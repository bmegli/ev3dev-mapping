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
 * This program is C# example for consuming output of xv11test with -raw argument
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
 * ./xv11test /dev/tty_in1 -raw | mono xv11pipe2csv.exe
 * 
 * See Usage() function for syntax details (or run the program without arguments)
 */


using System;
using System.IO;
using System.Runtime.InteropServices;

namespace xv11pipe
{
    class Program
    {
        static void Main(string[] args)
        {                                
            int laserReadingSize = Marshal.SizeOf(typeof(LaserFrame));
            int framesToRead = 90;
            int scanSize = framesToRead * laserReadingSize;

            byte[] buffer = new byte[scanSize];
            LaserFrame[] frames=new LaserFrame[framesToRead];

            if(args.Length!=0)
            {
                Usage();
                return;
            }

            ReadBinary(buffer);
            LaserFrameFromBinaryData(frames, buffer);
            LaserFramesToCSV(frames);
        }

        public static void ReadBinary(byte[] data)
        {
            int scanSize = data.Length;

            using (Stream stdin = Console.OpenStandardInput())
            {
                int bytesRead=0;

                do
                {
                    int read=stdin.Read(data, bytesRead, scanSize-bytesRead);
                    bytesRead += read;
                    if (read == 0)
                        throw new ArgumentException("Data size doesn't match supplied buffer");
                } while (bytesRead<scanSize);                
            }
        }

        public static void LaserFrameFromBinaryData(LaserFrame[] frames, byte[] data)
        {
            GCHandle handle = GCHandle.Alloc(data, GCHandleType.Pinned);
            int frameSize = Marshal.SizeOf(typeof(LaserFrame));

            if (frames.Length * Marshal.SizeOf(frames[0]) != data.Length)
                throw new ArgumentException("data size doesn't match supplied structure array size");

            try
            {
                IntPtr ptr = handle.AddrOfPinnedObject();
                
                for(int i=0;i<frames.Length;++i)
                    frames[i] = (LaserFrame)Marshal.PtrToStructure(ptr + i*frameSize, typeof(LaserFrame));
            }
            finally
            {
                handle.Free();
            }
        }
        public static void LaserFramesToCSV(LaserFrame[] frames)
        {
            for (int i = 0; i < 360; ++i)
            {
                Console.Out.Write(i);                
                if (i < 359)
                    Console.Out.Write(";");
            }
            Console.WriteLine();

            //here we are after synchronization so first 90 frames will be data from 0-359 degree scan
            //at any time you can also get the scan angle as (frames[i].index-0xA0)*4 + reading number (0-3)
            //you can get the current speed in rpm as frames[i].speed/64
            for (int i = 0; i < frames.Length; ++i)
            {
                if (i > 0 && i % 90 == 0)
                    Console.WriteLine();

                for (int j = 0; j < 4; ++j)
                {
                    if (frames[i].readings[j].invalid_data == 0)
                        Console.Write(frames[i].readings[j].distance);
                    else
                        Console.Write("0");
                    Console.Write(";");
                }
            }
        }

        public static void Usage()
        {
                Console.Out.WriteLine("This program is example for consuming output of xv11test with -raw argument." + Environment.NewLine);

                Console.Out.WriteLine("It will output semicolon separated distances for angles 0-359" + Environment.NewLine);

                Console.Out.WriteLine("Preconditions (for EV3/ev3dev):");
                Console.Out.WriteLine("-hardware connections (uart/motor)");
                Console.Out.WriteLine("-lego port put in other-uart mode");
                Console.Out.WriteLine("-lidar spinning CCW at around 200-300 rpm" + Environment.NewLine);

                Console.Out.WriteLine("Usage: ");
                Console.Out.WriteLine("-this program has no arguments ");
                Console.Out.WriteLine("-it expects synchronized binary lidar data on input");
                Console.Out.WriteLine("-pipe xv11test -raw output to xv11pipe2csv" + Environment.NewLine);

                Console.Out.WriteLine("Examples: ");
                Console.Out.WriteLine("./xv11test /dev/tty_in1 -raw | mono xv11pipe2csv.exe");
                Console.Out.WriteLine("./xv11test /dev/tty_in1 -raw | mono xv11pipe2csv.exe > distances.csv" + Environment.NewLine);

                Console.Out.WriteLine("More info:");
                Console.Out.WriteLine("http://www.ev3dev.org/docs/tutorials/using-xv11-lidar/");
            
        }
    }
}
