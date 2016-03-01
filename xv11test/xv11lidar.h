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

#pragma once

#include <stdint.h> //uint8_t, uint16_t
#include <termios.h> //termios

//return value for all the functions
enum xv11_status {SUCCESS=0, SYNCHRONIZATION_ERROR, TTY_ERROR, MEMORY_ERROR};

//internal data used by the functions
struct xv11lidar_data
{
	int fd;
	struct termios old_io;
	int laser_frames_per_read;
	uint8_t *data;
};

/*	For complete information on LIDAR data format see: 
 *	http://xv11hacking.wikispaces.com/LIDAR+Sensor
 *  LIDAR returns data in frames of 4 consecutive readings (angles)
*/
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

/* Sets terminal to the appropriate mode and synchrnonizes with the device
 * If this function fails there is no need to call CloseLaser
 * 
* parameters:
* lidar_data - internal data 
* tty - the path to lidar tty
* laser_frames_per_read - configure tty to rad that much frames per read, each frame has 4 degree scan
* 
* returns:
* xv11_status.SUCCESS (0) on success
* xv11_status.TTY_ERROR (2) on failure
* xv11_status.MEMORY_ERROR (3) on memory allocation failure
* 
* preconditions:
* -lidar spinning CCW at around 300 RPM
* -lidar uart reachable at tty 
* -lego port set to other-uart mode
*
*/
int InitLaser(struct xv11lidar_data *lidar_data, const char *tty, int laser_frames_per_read);

/* Cleans up (file descriptors, tty is set back to inital configuration)
* parameters:
* lidar_data - internal data
* 
* returns:
* xv11_status.SUCCESS (0) on success
* xv11_status.TTY_ERROR (2) on failure
* 
* preconditions:
* -lidar initialized InitLaser
*/
int CloseLaser(struct xv11lidar_data *lidar_data);

/* Reads from the lidar tty the number of frames configured during initialization
 * parameters:
 * lidar_data - internal data
 * frame_data - a pointer to array that is big enough to store laser_frames_per_read configured during InitLaser
 * 
 * returns:
 * xv11_status.SUCCESS (0) on success
 * xv11_status.SYNCHRONIZATION_ERROR (1) on laser synchronization failure
 * xv11_status.TTY_ERROR (2) on failure 
 * 
 * Currently in case of failure the laser has to be reninitialized (CloseLaser followed by InitLaser)
 * 
 * preconditions:
 * -lidar initialized with InitLaser
 */
int ReadLaser(struct xv11lidar_data *lidar_data, struct laser_frame *frame_data);