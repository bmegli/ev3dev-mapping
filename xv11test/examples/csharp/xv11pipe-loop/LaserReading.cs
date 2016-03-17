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

using System;

namespace xv11pipeloop
{
    [System.Runtime.InteropServices.StructLayoutAttribute(System.Runtime.InteropServices.LayoutKind.Sequential, Pack =1)]
    public struct LaserReading
    {

        /// distance : 14
        ///strength_warning : 1
        ///invalid_data : 1
        public ushort bitvector;

        /// unsigned short
        public ushort signal_strength;

        public ushort distance
        {
            get
            {
                return ((ushort)((this.bitvector & 16383u)));
            }
            set
            {
                this.bitvector = ((ushort)((value | this.bitvector)));
            }
        }

        public ushort strength_warning
        {
            get
            {
                return ((ushort)(((this.bitvector & 16384u)
                            / 16384)));
            }
            set
            {
                this.bitvector = ((ushort)(((value * 16384)
                            | this.bitvector)));
            }
        }

        public ushort invalid_data
        {
            get
            {
                return ((ushort)(((this.bitvector & 32768u)
                            / 32768)));
            }
            set
            {
                this.bitvector = ((ushort)(((value * 32768)
                            | this.bitvector)));
            }
        }
    }

    [System.Runtime.InteropServices.StructLayoutAttribute(System.Runtime.InteropServices.LayoutKind.Sequential, Pack = 1)]
    public struct LaserFrame
    {

        /// BYTE->unsigned char
        public byte start;

        /// BYTE->unsigned char
        public byte index;

        /// unsigned short
        public ushort speed;

        /// laser_reading[4]
        [System.Runtime.InteropServices.MarshalAsAttribute(System.Runtime.InteropServices.UnmanagedType.ByValArray, SizeConst = 4, ArraySubType = System.Runtime.InteropServices.UnmanagedType.Struct)]
        public LaserReading[] readings;

        /// unsigned short
        public ushort checksum;
    }
}
