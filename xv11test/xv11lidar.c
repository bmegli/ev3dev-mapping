/*
 * XV11 LIDAR communication library
 *
 * Copyright (C) 2016 Bartosz Meglicki <meglickib@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "xv11lidar.h"

#include <termios.h> //struct termios, tcgetattr, tcsetattr, cfsetispeed, tcflush
#include <fcntl.h> //file open flags
#include <stdlib.h> //exit
#include <stdio.h> //fprintf
#include <unistd.h> //read, close
#include <errno.h> //errno
#include <string.h> //memcpy
#include <limits.h> //UCHAR_MAX

/*
 *  This function calculates the checksum from the first 20 bytes of frame returned by the LIDAR
 *  You can expose this function (in xv11lidar.h) and compare its value with laser_frame.checksum
 *  Currently the ReadLaser implementation only outputs CRCFAIL
 *  to stderr when the checksum doesn't match the expected value
 *  A lot of checksum failures can indicate problem with reading the data (imperfect soldering, etc)
 *  or that the synchronization was lost and we are not reading frames correctly.
 *  Synchronization could be lost for example when not reading the data long and buffer overflow happens
 */
uint16_t Checksum(const uint8_t data[20])
{
	uint32_t chk32=0;
	uint16_t word;
	int i;
	
	for(i=0;i<10;++i)
	{
		word=data[2*i] + (data[2*i+1] << 8);
		chk32 = (chk32 << 1) + word;
	}
	
	uint32_t checksum=(chk32 & 0x7FFF) + (chk32 >> 15);
	return checksum & 0x7FFF;
}

/*
 * Flushes the TTY buffer so that we synchronize on new data
 * Waits for 0xFA byte (starting laser_frame) followed by 0xA0 which is angle 0-3 frame
 * Discards the rest 20 bytes of frame and the next 89 frames so that the next read will 
 * start reading at frame with 0-3 angles. 
 * Finally sets the termios to return the data in largest possible chunks (termios.c_cc[VMIN])
 */
int SynchronizeLaser(int fd, int laser_frames_per_read)
{		
	uint8_t c=0;
	int i;
		
	//flush the current TTY data so that buffers are clean
	//The LIDAR may have been spinning for a while
	if(tcflush(fd, TCIOFLUSH)!=0)
		return TTY_ERROR;

	while(1)
	{
		if (read(fd,&c,1)>0)
		{
			//wait for frame start
			if(c==0xFA)
			{
				//wait for angle 0
				if (read(fd,&c,1)>0 && c!=0xA0)
					continue;
				
				//discard 360 degree scan (next 20 bytes of frame 0 and 89 frames with 22 bytes)
				for(i=0;i<20 + 22*89;++i) 
					if (read(fd,&c,1)<0)
						return TTY_ERROR;						
					
				//get the termios and set it to return data in largest possible chunks
				struct termios io;
				if(tcgetattr(fd, &io) < 0)				
					return TTY_ERROR;
							
				if(laser_frames_per_read*sizeof(struct laser_frame) <= UCHAR_MAX)					
					io.c_cc[VMIN]=laser_frames_per_read*sizeof(struct laser_frame); 
				else
					io.c_cc[VMIN]=11*sizeof(struct laser_frame); //11*22=242 which is the largest possible value <= UCHAR_MAX 	
					
				if(tcsetattr(fd, TCSANOW, &io) < 0)
					return TTY_ERROR;
		
				break;
			}	
		}
		else
			return TTY_ERROR;
	}
	return SUCCESS;
}

/*
 * Open the terminal
 * Save its original settings in lidar_data->old_io
 * Set terminal for raw byte input single byte at a time at 115200 speed
 * Synchronize with the laser
 * Alloc internal buffer for laser readings
 */
int InitLaser(struct xv11lidar_data *lidar_data, const char *tty, int laser_frames_per_read)
{
	int error;
	struct termios io;
	lidar_data->data=NULL;

	if ((lidar_data->fd=open(tty, O_RDONLY))==-1)
		return TTY_ERROR;
	
	if(tcgetattr(lidar_data->fd, &lidar_data->old_io) < 0)
	{
		close(lidar_data->fd);
		return TTY_ERROR;		
	}
			
	io.c_iflag=io.c_oflag=io.c_lflag=0;
	io.c_cflag=CS8|CREAD|CLOCAL; //8 bit characters
	
	io.c_cc[VMIN]=1; //one input byte enough
	io.c_cc[VTIME]=0; //no timer
	
	if(cfsetispeed(&io, B115200) < 0 || cfsetospeed(&io, B115200) < 0)
	{
		close(lidar_data->fd);
		return TTY_ERROR;		
	}

	if(tcsetattr(lidar_data->fd, TCSAFLUSH, &io) < 0)
	{
		close(lidar_data->fd);
		return TTY_ERROR;
	}
				
	lidar_data->laser_frames_per_read=laser_frames_per_read;
		
	error=SynchronizeLaser(lidar_data->fd, laser_frames_per_read);
	if(error!=SUCCESS)
		close(lidar_data->fd);
	else if( (lidar_data->data=(uint8_t*)malloc(laser_frames_per_read*sizeof(struct laser_frame))) == 0)
	{
		close(lidar_data->fd);
		return MEMORY_ERROR;
	}
	return error;
}

/*
 * Restore original terminal settings
 * Free the allocated buffer for laser readings
 * Close the terminal
 */
int CloseLaser(struct xv11lidar_data *lidar_data)
{
	int error=SUCCESS;
	
	if(tcsetattr(lidar_data->fd, TCSAFLUSH, &lidar_data->old_io) < 0)
		error=TTY_ERROR;
	
	free(lidar_data->data);
	close(lidar_data->fd);
	
	return error;
}

/*
 * Read from LIDAR until requested number of frames is read or error occurs
 * 
 */
int ReadLaser(struct xv11lidar_data *lidar_data, struct laser_frame *frame_data)
{
	const size_t total_read_size=sizeof(struct laser_frame)*lidar_data->laser_frames_per_read;
	uint8_t *data=lidar_data->data; 
	size_t bytes_read=0;
	int status=0;
	int i;
	
	while( bytes_read < total_read_size )
	{
		if( (status=read(lidar_data->fd,data+bytes_read,total_read_size-bytes_read))<0 )
			return TTY_ERROR;
		else if(status==0)
			return TTY_ERROR;
			
		bytes_read+=status;
	}

	memcpy(frame_data, data, total_read_size);
	
	for(i=0;i<lidar_data->laser_frames_per_read;++i)
	{
		if(Checksum(data+i*sizeof(struct laser_frame))!=frame_data[i].checksum)
			fprintf(stderr, " CRCFAIL ");			
		
		if(frame_data[i].start!=0xFA)
			return SYNCHRONIZATION_ERROR;
	}
	return SUCCESS; 
}