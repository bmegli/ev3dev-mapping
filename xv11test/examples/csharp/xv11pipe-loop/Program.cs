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
 * This program is C# example for consuming unlimited output of xv11test with -raw argument
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
 * ./xv11test /dev/tty_in1 -raw 15 0 | mono xv11pipe-loop.exe
 * 
 * See Usage() function for syntax details (or run the program without arguments)
 */


using System;
using System.IO;
using System.Runtime.InteropServices;

namespace xv11pipeloop
{
    class Program
    {
        static void Main(string[] args)
        {
            if (args.Length != 0)
            {
                Usage();
                return;
            }

            int framesToRead = 15;
            int scanSize = framesToRead * Marshal.SizeOf(typeof(LaserFrame));
  
            byte[] buffer = new byte[scanSize];
            LaserFrame[] frames = new LaserFrame[framesToRead];
            int[] distances = new int[360];

            Stream stdin = Console.OpenStandardInput();

            while( ReadBinary(stdin, buffer) )
            {
                LaserFrameFromBinaryData(frames, buffer);

                foreach(LaserFrame frame in frames)
                {
                    int angle = (frame.index - 0xA0) * 4;
                    if (angle < 0 || angle > 356)
                    {
                        Console.Error.WriteLine("Frame index out of bounds. CRC failure?");
                        continue;
                    }
                    for(int i=0;i<4;++i)
                    {
                        if (frame.readings[i].invalid_data == 0)
                            distances[angle + i] = frame.readings[i].distance;

                        if(angle + i==0)
                        {
                            if (frame.readings[i].invalid_data == 0)
                                Console.Out.Write(distances[0] + " ");
                            else
                                Console.Out.Write("e ");
                        }
                    }
                }
            }

            Console.Out.WriteLine("End of input");
        }

        public static bool ReadBinary(Stream stdin, byte[] data)
        {
            int scanSize = data.Length;

            int bytesRead = 0;

            do
            {
                int read = stdin.Read(data, bytesRead, scanSize - bytesRead);
                bytesRead += read;
                if (read == 0)
                    return false;
            } while (bytesRead < scanSize);
            return true;     
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

                for (int i = 0; i < frames.Length; ++i)
                    frames[i] = (LaserFrame)Marshal.PtrToStructure(ptr + i * frameSize, typeof(LaserFrame));
            }
            finally
            {
                handle.Free();
            }
        }
    

        public static void Usage()
        {
            Console.Out.WriteLine("This program is C# example for consuming unlimited output of xv11test with -raw argument" + Environment.NewLine);

            Console.Out.WriteLine("It will output continously distance at LIDAR angle 0 or 'e' on reading error" + Environment.NewLine);

            Console.Out.WriteLine("Preconditions (for EV3/ev3dev):");
            Console.Out.WriteLine("-hardware connections (uart/motor)");
            Console.Out.WriteLine("-lego port put in other-uart mode");
            Console.Out.WriteLine("-lidar spinning CCW at around 200-300 rpm" + Environment.NewLine);

            Console.Out.WriteLine("Usage: ");
            Console.Out.WriteLine("-this program has no arguments ");
            Console.Out.WriteLine("-it expects synchronized binary lidar data on input");
            Console.Out.WriteLine("-pipe xv11test -raw xx 0 output to xv11pipe-loop" + Environment.NewLine);

            Console.Out.WriteLine("Examples: ");
            Console.Out.WriteLine("./xv11test /dev/tty_in1 -raw 15 0 | mono xv11pipe-loop.exe" + Environment.NewLine);

            Console.Out.WriteLine("More info:");
            Console.Out.WriteLine("http://www.ev3dev.org/docs/tutorials/using-xv11-lidar/");

        }
    }
}
