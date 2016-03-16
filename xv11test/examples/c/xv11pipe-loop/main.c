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
  * This program is C example for consuming unlimited output of xv11test with -raw argument
  * 
  * Please note that in C you can just use xv11lidar.h and xv11lidar.c from xv11test
  * as a small C LIDAR library which is a better way
  * 
  * This program reads from standard input expecting synchronized raw binary lidar data
  * 
  * It will output continously distance at LIDAR angle 0 or 'e' on reading error.
  * Note that this is almost but not exactly ahead of the LIDAR (geometric correction needs to be applied)
  *   
  * Preconditions (for EV3/ev3dev):
  * -hardware connections (uart/motor)
  * -lego port put in other-uart mode
  * -lidar spinning CCW at around 200-300 rpm
  * 
  * It should be invokded with xv11test pipeline (example):
  * 
  * ./xv11test /dev/tty_in1 -raw 15 0 | ./xv11pipe-loop
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

int ScanLoop();
void Usage();

const int FRAMES_TO_READ=15; //read data by 60 degrees

int main(int argc, char **argv)
{		
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
	
	return ScanLoop();
}

// This functions keeps updated table of distances and prints distance at angle 0 (ahead) continously
int ScanLoop()
{
	struct laser_frame frames[FRAMES_TO_READ];
	int distances[360]={0}; 
	int i,j;
	size_t result;
	
	while(1)
	{
		result = fread(frames, sizeof(struct laser_frame), FRAMES_TO_READ, stdin);

		if(result == 0)
		{
			printf("End of input\n");
			return 0;
		}
		else if(result != FRAMES_TO_READ)
		{
			fprintf(stderr, "Error on reading data");
			return 1;
		}
		
		for(i=0;i<FRAMES_TO_READ;++i)
		{
			int angle=(frames[i].index-0xA0)*4;
	
			if(angle<0 || angle>356)
			{
				fprintf(stderr, "Frame index out of bounds. CRC failure?\n");
				continue;
			}
			for(j=0;j<4;++j)
			{
				if(frames[i].readings[j].invalid_data == 0)					
					distances[angle+j]=frames[i].readings[j].distance;
				
				if(angle+j==0)
				{
					if(frames[i].readings[j].invalid_data == 0)
						printf("%d ", distances[0]);
					else
						printf("e ");
					fflush(stdout);
				}
			}			
		}
	}	
}

void Usage()
{
	printf("This program is C example for consuming unlimited output of xv11test with -raw argument.\n\n");

	printf("It will output continously distance at LIDAR angle 0 or 'e' on reading error.\n\n");
		
	printf("Preconditions (for EV3/ev3dev):\n");
	printf("-hardware connections (uart/motor)\n");
	printf("-lego port put in other-uart mode\n");
	printf("-lidar spinning CCW at around 200-300 rpm\n\n");
		
	printf("Usage: \n");
	printf("-this program has no arguments \n");
	printf("-it expects synchronized binary lidar data on input\n");
	printf("-pipe xv11test -raw x 0 output to xv11pipe-loop \n\n");
			
	printf("Examples: \n");
	printf("./xv11test /dev/tty_in1 -raw 15 0 | ./xv11pipe-loop \n\n");
	
	printf("More info:\n");
	printf("http://www.ev3dev.org/docs/tutorials/using-xv11-lidar/\n");
}