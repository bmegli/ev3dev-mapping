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
#include <stdio.h> //perror
#include <unistd.h> //read, close
#include <errno.h> //errno
#include <string.h> //memcpy

/*
void DieNow(const char *s)
{
    perror(s);
    exit(1);
}
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

int SynchronizeLaser(int fd, int laser_frames_per_read)
{		
	uint8_t c=0;
	int i;

	if(tcflush(fd, TCIOFLUSH)!=0)
		return TTY_ERROR;
		//DieNow("Laser synchronization failed (tcflush)");
	
	while(1)
	{
		if (read(fd,&c,1)>0)
		{
			if(c==0xFA)
			{
				//wait for angle 0
				if (read(fd,&c,1)>0 && c!=0xA0)
					continue;
				
				//discard 360 degree scan (90 frames with 22 bytes)
				for(i=0;i<20 + 22*89;++i) 
					if (read(fd,&c,1)<0)
						return TTY_ERROR;
						//DieNow("Laser synchroznization failed (21 bytes discard)\n");
		
				struct termios io;
				if(tcgetattr(fd, &io) < 0)
					return TTY_ERROR;
					//DieNow("tcgetattr(fd, &io)");
			
				io.c_cc[VMIN]=sizeof(struct laser_frame)*laser_frames_per_read; 
				if(tcsetattr(fd, TCSANOW, &io) < 0)
					return TTY_ERROR;
					//DieNow("tcsetattr(fd, TCSANOW, &io)");
		
				break;
			}	
		}
		else
			return TTY_ERROR;
	}
	return SUCCESS;
}

int InitLaser(struct xv11lidar_data *lidar_data, const char *tty, int laser_frames_per_read)
{
	int fd=0;
	struct termios io;

	if ((fd=open(tty, O_RDONLY))==-1)
		return TTY_ERROR;
		//DieNow("open(tty, O_RDONLY)");
	
	if(tcgetattr(fd, &lidar_data->old_io) < 0)
		return TTY_ERROR;
		//DieNow("tcgetattr(fd, &old_io)");
			
	io.c_iflag=io.c_oflag=io.c_lflag=0;
	io.c_cflag=CS8|CREAD|CLOCAL; //8 bit characters
	
	io.c_cc[VMIN]=1; //one input byte enough
	io.c_cc[VTIME]=0; //no timer
	
	if(cfsetispeed(&io, B115200) < 0 || cfsetospeed(&io, B115200) < 0)
		return TTY_ERROR;
		//DieNow("cfsetispeed(&io, B115200) < 0 || cfsetospeed(&io, B115200) < 0");
 
	if(tcsetattr(fd, TCSAFLUSH, &io) < 0)
		return TTY_ERROR;
		//DieNow("tcsetattr(fd, TCSAFLUSH, &io)");
		
	lidar_data->fd=fd;
	lidar_data->laser_frames_per_read=laser_frames_per_read;
	return SynchronizeLaser(fd, laser_frames_per_read);
}

int CloseLaser(const struct xv11lidar_data *lidar_data)
{
	if(tcsetattr(lidar_data->fd, TCSAFLUSH, &lidar_data->old_io) < 0)
		return TTY_ERROR;
		
	close(lidar_data->fd);
	
	return SUCCESS;
}

int ReadLaser(const struct xv11lidar_data *lidar_data, struct laser_frame *frame_data)
{
	const size_t total_read_size=sizeof(struct laser_frame)*lidar_data->laser_frames_per_read;
	uint8_t data[total_read_size]; 
	size_t bytes_read=0;
	int status=0;
	int i;
	
	while( bytes_read < total_read_size )
	{
		if( (status=read(lidar_data->fd,data+bytes_read,total_read_size-bytes_read))<0 )
		{
			//if(errno==EINTR) //interrupted by signal
				//return; //0
			return TTY_ERROR;
			//DieNow("status=read(fd,data+bytes_read,total_read_size-bytes_read))<0");
		}
		else if(status==0)
			return TTY_ERROR;
			//DieNow("status==0, read no bytes?\n");
			
		bytes_read+=status;
	}

	
	memcpy(frame_data, data, total_read_size);
	
	for(i=0;i<lidar_data->laser_frames_per_read;++i)
	{
		if(Checksum(data+i*sizeof(struct laser_frame))!=frame_data[i].checksum)
		{
			fprintf(stderr, " CRCFAIL ");			
		}
		if(frame_data[i].start!=0xFA)
			return SYNCHRONIZATION_ERROR;
			//DieNow("Synchronization lost!\n");
	}
	return SUCCESS; 
}