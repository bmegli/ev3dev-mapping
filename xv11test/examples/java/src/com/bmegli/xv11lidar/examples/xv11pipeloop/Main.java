package com.bmegli.xv11lidar.examples.xv11pipeloop;

/*
 * XV11 LIDAR test utility example
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
 * This program is java example for consuming unlimited output of xv11test with -raw argument
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
 * It should be invoked with xv11test pipeline (example):
 * 
 * ./xv11test /dev/tty_in1 -raw 15 0 | java -jar xv11pipe-loop.jar
 * 
 * 
 */
 
import java.io.DataInputStream;
import com.bmegli.xv11lidar.LaserScanner;

public class Main
{
	public static void main(String[] args)
	{
		LaserScanner laser=new LaserScanner(new DataInputStream(System.in));
		int[] distances=new int[360];	
		
		while(laser.Read())
		{
			if( !laser.isInvalidData() )
				distances[laser.getAngle()]=laser.getDistance();
 						
			if(laser.getAngle()==0)
			{
				if( !laser.isInvalidData() )
					System.out.print( laser.getDistance() + " ");
				else
					System.out.print("e ");
			}						
		}			
	}

}
