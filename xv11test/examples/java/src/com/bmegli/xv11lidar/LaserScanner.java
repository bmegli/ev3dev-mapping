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

public class LaserScanner
{
	private DataInput in;
	private LaserFrame frame=new LaserFrame(); 
	private int reading=3;
	
	public LaserScanner(DataInput input) 
	{
		in=input;
	}
		
	public boolean Read() 
	{
		try
		{
			if(reading>=3 || reading<0)
			{
				frame.Fill(in);
				reading=0;
				return true;
			}
		}
		catch(IOException e)
		{
			return false;
		}
			
		++reading;
		return true;
		
	}
		
	public int getAngle()
	{
		return  frame.getAngle()+reading;
	}
	
	public int getSpeedRPMFixedPoint()
	{
		return frame.getSpeedRPMFixedPoint();
	}
	
	public float getSpeedRPM()
	{
		return frame.getSpeedRPM();
	}
		
	public int getChecksum()
	{
		return frame.getChecksum();
	}
	
	public int getSignal()
	{
		return frame.getSignal(reading);
	}
	
	public int getDistance()
	{
		return frame.getDistance(reading);
	}
	
	public boolean isStrengthWarning()
	{
		return frame.isStrengthWarning(reading);
	}
	
	public boolean isInvalidData()
	{
		return frame.isInvalidData(reading);
	}

}
