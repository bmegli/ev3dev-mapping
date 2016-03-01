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

#include <stdio.h>

#include "xv11lidar.h"

void LaserDataToCSV(struct laser_frame *frames,int frames_number);
void Usage();

int main(int argc, char **argv)
{	
	const int capture_frames=90; //90 frames each with 4 angles read - 360 degree read
	struct xv11lidar_data xv11_data;
	struct laser_frame frames[capture_frames];
	int status;

	if(argc!=2)	
	{
		Usage();
		return 0;
	}	
	const char *tty=argv[1];
	
	status=InitLaser(&xv11_data, tty, capture_frames);
	
	if(status != SUCCESS)	
	{
		fprintf(stderr, "Laser initialization failed with status %d\n", status);
		return status;
	}
			
	status=ReadLaser(&xv11_data, frames);
	
	if(status != SUCCESS)
		fprintf(stderr, "Laser read failed with status %d\n", status);
	
	status=CloseLaser(&xv11_data);
	
	if(status != SUCCESS)
		fprintf(stderr, "Laser cleanup failed with status %d\n", status);
		
	LaserDataToCSV(frames, capture_frames);

	return 0;
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
				printf("%d", frames[i].readings[j].distance);
			else
				printf("0");
			printf(";");
		}
	}
}

void Usage()
{
	printf("This program is for testing XV11 lidar\n");
	printf("It outputs semicolon separated list of distances (or zeroes on invalid_data) for angles 0-359 \n");
	printf("It was created for EV3 with ev3dev OS but can be used anywhere as long as you have tty connection with the lidar\n");
	printf("Preconditions (for EV3/ev3dev):\n");
	printf("-hardware connections (uart/motor)\n");
	printf("-lego port put in other-uart mode\n");
	printf("-lidar spinning CCW at around 200-300 rpm\n");
	printf("\n");
	printf("Usage: \n");
	printf("xv11test tty_device \n");
	printf("\n");
	printf("Example: \n");
	printf("xv11test /dev/tty_in1 \n");
	printf("xv11test /dev/tty_in1 > distances.csv\n");
}