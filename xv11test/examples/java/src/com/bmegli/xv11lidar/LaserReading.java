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

class LaserReading
{	
	private short bits;
	private short signal;
				
	public LaserReading() {
	}

	public void Fill(DataInput in) throws IOException
	{
		// bits and signal are in Little Endian so we can't use in.readUnsignedShort (Big Endian)
		bits = (short) ( in.readUnsignedByte() | ( in.readUnsignedByte() << 8) );
		signal =(short) ( in.readUnsignedByte() | ( in.readUnsignedByte() << 8) );		
	}
	
	public int getSignal()
	{
		return signal & 0xFFFF;
	}
	
	public int getDistance()
	{
		return bits & 0x3FFF;
	}
	
	public boolean isStrengthWarning()
	{
		return (bits & 0x4000) != 0;
	}
	
	public boolean isInvalidData()
	{
		return (bits & 0x8000) != 0;
	}	
}
