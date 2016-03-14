/*
 * XV11 LIDAR test utility
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
 
 /*
  * This program is for testing XV11 lidar.
  * 
  * It will output:
  * -semicolon separated distances for angles 0-359 (default)
  * -synchronized binary lidar data (-raw argument)
  * 
  * It was created for EV3 with ev3dev OS...
  * ... but will work anywhere (unix, tty lidar connection required)
  * 
  * Preconditions (for EV3/ev3dev):
  * -hardware connections (uart/motor)
  * -lego port put in other-uart mode
  * -lidar spinning CCW at around 200-300 rpm
  * 
  * See Usage() function for syntax details (or run the program without arguments)
  */
	
#include "xv11lidar.h"
#include <unistd.h> //write
#include <stdio.h> //printf, fprintf
#include <string.h> //strncmp
#include <stdlib.h> //strtol

void TextLaserData(struct xv11lidar_data *xv11_data, struct laser_frame *frames, int frames_number);
void BinaryLaserData(struct xv11lidar_data *xv11_data, struct laser_frame *frames, int frames_number, int frames_limit);

void LaserDataToCSV(struct laser_frame *frames,int frames_number);
int ProcessInput(int argc, char **argv, int *raw_mode, int *frames_per_read, int *frames_limit);
void Usage();

int main(int argc, char **argv)
{	
	int capture_frames,status, raw_mode, frames_limit; 
	struct xv11lidar_data xv11_data;
	struct laser_frame *frames;

	//Arguments processing (tty, raw_mode, number of frames to capture, frames limit)

	if(ProcessInput(argc, argv, &raw_mode, &capture_frames, &frames_limit) == 0)	
	{
		Usage();
		return 0;
	}	
	
	const char *tty=argv[1];
	
	//Initialization (memory, laser)
	
	frames = (struct laser_frame*)malloc(capture_frames * sizeof(struct laser_frame));

	if( frames==0 )
	{
		fprintf(stderr, "Unable to allocate memory for lidar data");
		return 1;
	}
	
	status=InitLaser(&xv11_data, tty, capture_frames);
	
	if(status != SUCCESS)	
	{
		fprintf(stderr, "Laser initialization failed with status %d\n", status);
		free(frames);
		return status;
	}
			
	// Actual work (read from lidar, output data)		
			
	if(raw_mode==0) 
		TextLaserData(&xv11_data, frames, capture_frames);
	else 
		BinaryLaserData(&xv11_data, frames, capture_frames, frames_limit);
	
	// Cleanup (laser, memory)
	status=CloseLaser(&xv11_data);
	
	if(status != SUCCESS)
		fprintf(stderr, "Laser cleanup failed with status %d\n", status);

	free(frames);
		
	return 0;
}

void TextLaserData(struct xv11lidar_data *xv11_data, struct laser_frame *frames, int frames_number)
{
	int status;
	status = ReadLaser(xv11_data, frames);

	if(status != SUCCESS)
		fprintf(stderr, "Laser read failed with status %d\n", status);
	else
		LaserDataToCSV(frames, frames_number);	
}
void BinaryLaserData(struct xv11lidar_data *xv11_data, struct laser_frame *frames, int frames_number,int frames_limit)
{
	int status, data_written;
	const int data_to_write=frames_number*sizeof(struct laser_frame);
	const char *frames_char_data=(char *)frames;
	long int frames_processed=0;
		
	while(1)
	{
		status = ReadLaser(xv11_data, frames);
		if(status != SUCCESS)
		{
			fprintf(stderr, "Laser read failed with status %d\n", status);
			break;
		}
		data_written=0;
		while(data_written<data_to_write)
		{
			status=write(STDOUT_FILENO, frames_char_data+data_written, data_to_write-data_written);
			if(status<0)
			{
				perror("Unable to write lidar data to stdout\n");
				return;
			}
			data_written+=status;
		}	
		
		frames_processed += frames_number;
		if(frames_limit == frames_processed)
			return;
	}
}


void LaserDataToCSV(struct laser_frame *frames,int frames_number)
{
	int i,j;
	for(i=0;i<360;++i)
	{
		printf("%d",i);
		if(i<359)
			printf(";");
	}
	printf("\n");
	
	//here we are after synchronization so first 90 frames will be data from 0-359 degree scan
	//at any time you can also get the scan angle as (frames[i].index-0xA0)*4 + reading number (0-3)
	//you can get the current speed in rpm as frames[i].speed/64
	for(i=0;i<frames_number;++i)
	{
		if(i>0 && i % 90==0)
			printf("\n");
			
		for(j=0;j<4;++j)
		{
			if(frames[i].readings[j].invalid_data==0)
				printf("%u", frames[i].readings[j].distance);
			else
				printf("0");
			printf(";");
		}
	}
}

int ProcessInput(int argc, char **argv, int *raw_mode, int *frames_per_read, int *out_frames_limit)
{
	long int frames=15;
	long int frames_limit=90;
	
	if(argc==2) //character mode
	{
		*raw_mode=0;
		*frames_per_read=90;
		return 1;
	}
	if(argc>=3 && argc<=5) //raw mode
	{
		if( strncmp("-raw", argv[2],4) !=0 )
			return 0;
		*raw_mode=1;
		
		if(argc>=4)
		{
			frames=strtol(argv[3], NULL, 0);
			if(frames<=0)
			{
				fprintf(stderr, "frames_per_read argument has to be positive\n\n");
				return 0;
			}			
		}
		
		*frames_per_read=(int) frames;
		
		if(argc>=5)
		{
			frames_limit=strtol(argv[4], NULL, 0);
			if(frames_limit<0)
			{
				fprintf(stderr, "frames_limit argument has to be non-negative\n\n");
				return 0;
			}		
			if(frames_limit % frames != 0)
			{
				fprintf(stderr, "frames_limit has to be multiplicity of frames_per_read\n\n");
				return 0;
			}
	
		}
		
		*out_frames_limit=(int) frames_limit;
		
		return 1;
	}
	return 0;	
}


void Usage()
{
	printf("This program is for testing XV11 lidar.\n\n");

	printf("It will output:\n");
	printf("-semicolon separated distances for angles 0-359 (default) \n");
	printf("-synchronized binary lidar data (-raw argument)\n\n");
	
	printf("It was created for EV3 with ev3dev OS...\n");
	printf("...but will work on any POSIX system (tty lidar connection required)\n\n");
	
	printf("Preconditions (for EV3/ev3dev):\n");
	printf("-hardware connections (uart/motor)\n");
	printf("-lego port put in other-uart mode\n");
	printf("-lidar spinning CCW at around 200-300 rpm\n\n");
	
	printf("Usage: \n");
	printf("xv11test tty_device \n");
	printf("xv11test tty_device -raw [frames_per_read] [frames_limit] \n\n");

	printf("The first call outputs once character distance data for angle 0-359\n");
	printf("The second call outputs binary lidar data in requested chunks and limit\n");
	printf("The default frames_per_read for binary data is 15 (60 degrees scan)\n");
	printf("The default frames_limit for binary data is 90 (360 degree scan)\n");
	printf("Use 0 for frames_limit to get continous binary output\n\n");
	
	printf("Examples: \n");
	printf("xv11test /dev/tty_in1 \n");
	printf("xv11test /dev/tty_in1 > distances.csv\n");
	printf("xv11test /dev/tty_in1 -raw | program_consuming_90_frames_of_binary_data\n");
	printf("xv11test /dev/tty_in1 -raw 15 | program_consuming_90_frames_of_binary_data\n");
	printf("xv11test /dev/tty_in1 -raw 15 90 | program_consuming_90_frames_of_binary_data\n");
	printf("xv11test /dev/tty_in1 -raw 15 90 > binary_file_with_90_frames.bin\n");
	printf("xv11test /dev/tty_in1 -raw 15 0 | program_consuming_continous_binary_data\n\n");	

	printf("More info:\n");
	printf("http://www.ev3dev.org/docs/tutorials/using-xv11-lidar/\n");
}