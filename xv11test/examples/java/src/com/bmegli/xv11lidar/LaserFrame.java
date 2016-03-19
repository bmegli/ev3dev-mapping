package com.bmegli.xv11lidar;

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

import java.io.DataInput;
import java.io.IOException;
	
class LaserFrame
{
	private byte start;
	private byte index;
	private short speed;
	private LaserReading[] readings= {new LaserReading(), new LaserReading(), new LaserReading(), new LaserReading()};
	private short checksum;
	
	public void Fill(DataInput in) throws IOException
	{
		// speed and checksum are in Little Endian so we can't use in.readUnsignedShort (Big Endian)
		start = (byte)in.readUnsignedByte();
		index = (byte)in.readUnsignedByte();
					
		speed= (short) ( in.readUnsignedByte() | ( in.readUnsignedByte() << 8) ); 
			
		for(int i=0;i<4;++i)
			readings[i].Fill(in);
				
		checksum = (short) ( in.readUnsignedByte() | ( in.readUnsignedByte() << 8) );	
	}
			
	public LaserFrame() {		
	}

	public int getStart()
	{
		return start & 0xFF;
	}
	public int getAngle()
	{
		return  ((index & 0xFF)-0xA0)*4;
	}
	
	public int getSpeedRPMFixedPoint()
	{
		return speed & 0xFFFF;
	}
	
	public float getSpeedRPM()
	{
		return (speed & 0xFFFF) / 64.0f;
	}
				
	public int getChecksum()
	{
		return checksum & 0xFFFF;
	}
	
	public int getSignal(int r)
	{
		return readings[r].getSignal();
	}
	
	public int getDistance(int r)
	{
		return readings[r].getDistance();
	}
	
	public boolean isStrengthWarning(int r)
	{
		return readings[r].isStrengthWarning();
	}
	
	public boolean isInvalidData(int r)
	{
		return readings[r].isInvalidData();
	}	
}
