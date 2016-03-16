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
  * This program is C example for consuming output of xv11test with -raw argument
  * 
  * Please note that in C you can just use xv11lidar.h and xv11lidar.c from xv11test
  * as a small C LIDAR library which is a better way
  * 
  * This program reads from standard input expecting synchronized raw binary lidar data
  * 
  * It will output semicolon separated distances for angles 0-359 
  *   
  * Preconditions (for EV3/ev3dev):
  * -hardware connections (uart/motor)
  * -lego port put in other-uart mode
  * -lidar spinning CCW at around 200-300 rpm
  * 
  * It should be invokded with xv11test pipeline (example):
  * 
  * ./xv11test /dev/tty_in1 -raw | ./xv11pipe2csv
  * 
  * See Usage() function for syntax details (or run the program without arguments)
  */

#include <stdio.h> //freopen, fprintf, perror
#include <stdint.h> //uint8_t, uint16_t

//single angle reading
struct laser_reading
{
	unsigned int distance : 14; //distance or error code when invalid_data flag is set
	unsigned int strength_warning : 1; //flag indicating that reported signal strength is lower then expected
	unsigned int invalid_data : 1; //flag indicating that distance could not be calculated	
	uint16_t signal_strength; //received signal strength 
} __attribute__((packed));

//single frame read from lidar
struct laser_frame
{
	uint8_t start; //fixed 0xFA can be used for synchronization
	uint8_t index; //(index-0xA0)*4 is the angle for readings[0] (+1,2,3 for consecutive readings)
	uint16_t speed; //divide by 64 to get speed in rpm
	struct laser_reading readings[4]; //readings for 4 consecutive angles
	uint16_t checksum; //if you need it in xv11lidar.c there is a function for calculating the checksum, compare with this value
	
} __attribute__((packed));

void LaserDataToCSV(struct laser_frame *frames,int frames_number);
void Usage();

const int FRAMES_TO_READ=90; //360 degree scan

int main(int argc, char **argv)
{
	struct laser_frame frames[FRAMES_TO_READ];
	size_t result;
	
	if(argc!=1)
	{
		Usage();
		return 0;
	}
	
	if(freopen(NULL, "rb",stdin)==NULL)
	{
		perror("unable to reopen stdin in binary mode\n");
		return 1;
	}
	
	//read in a loop if you want to do something continously with the data
	result = fread(frames, sizeof(struct laser_frame), FRAMES_TO_READ, stdin);
		
	if(result != FRAMES_TO_READ)
	{
		fprintf(stderr, "Error on reading data");
		return 1;
	}

	LaserDataToCSV(frames, FRAMES_TO_READ);

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
				printf("%u", frames[i].readings[j].distance);
			else
				printf("0");
			printf(";");
		}
	}
}

void Usage()
{
	printf("This program is example for consuming output of xv11test with -raw argument.\n\n");

	printf("It will output semicolon separated distances for angles 0-359\n\n");
		
	printf("Preconditions (for EV3/ev3dev):\n");
	printf("-hardware connections (uart/motor)\n");
	printf("-lego port put in other-uart mode\n");
	printf("-lidar spinning CCW at around 200-300 rpm\n\n");
		
	printf("Usage: \n");
	printf("-this program has no arguments \n");
	printf("-it expects synchronized binary lidar data on input\n");
	printf("-pipe xv11test output to xv11pipe \n\n");
			
	printf("Examples: \n");
	printf("./xv11test /dev/tty_in1 -raw | ./xv11pipe2csv \n");
	printf("./xv11test /dev/tty_in1 -raw | ./xv11pipe2csv > distances.csv \n\n");
	
	printf("More info:\n");
	printf("http://www.ev3dev.org/docs/tutorials/using-xv11-lidar/\n");
}