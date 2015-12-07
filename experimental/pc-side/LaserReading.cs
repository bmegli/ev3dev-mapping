using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LaserScanner
{
    [System.Runtime.InteropServices.StructLayoutAttribute(System.Runtime.InteropServices.LayoutKind.Sequential, Pack =1)]
    public struct LaserReading
    {

        /// distance : 14
        ///strength_warning : 1
        ///invalid_data : 1
        public ushort bitvector1;

        /// unsigned short
        public ushort signal_strength;

        public ushort distance
        {
            get
            {
                return ((ushort)((this.bitvector1 & 16383u)));
            }
            set
            {
                this.bitvector1 = ((ushort)((value | this.bitvector1)));
            }
        }

        //just wondering if this bit is a warning or a bit of distance still
        public ushort unknown_warning
        {
            get
            {
                return ((ushort)(((this.bitvector1 & 8192u)
                            / 8192)));
            }       
        }

        public ushort strength_warning
        {
            get
            {
                return ((ushort)(((this.bitvector1 & 16384u)
                            / 16384)));
            }
            set
            {
                this.bitvector1 = ((ushort)(((value * 16384)
                            | this.bitvector1)));
            }
        }

        public ushort invalid_data
        {
            get
            {
                return ((ushort)(((this.bitvector1 & 32768u)
                            / 32768)));
            }
            set
            {
                this.bitvector1 = ((ushort)(((value * 32768)
                            | this.bitvector1)));
            }
        }
    }

    [System.Runtime.InteropServices.StructLayoutAttribute(System.Runtime.InteropServices.LayoutKind.Sequential, Pack = 1)]
    public struct laser_frame
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

    [System.Runtime.InteropServices.StructLayoutAttribute(System.Runtime.InteropServices.LayoutKind.Sequential, Pack = 1)]
    public struct RobotFrame
    {
        public const int LASER_FRAMES_PER_360 = 90;
        public const int LASER_FRAMES_PER_PACKET = 15;

        public ushort laser_angle;
        public ushort laser_speed;
        public ushort elapsed_ms;

        /// laser_reading[LASER_FRAMES_PER_PACKET*4]
        [System.Runtime.InteropServices.MarshalAsAttribute(System.Runtime.InteropServices.UnmanagedType.ByValArray, SizeConst = LASER_FRAMES_PER_PACKET*4, ArraySubType = System.Runtime.InteropServices.UnmanagedType.Struct)]
        public LaserReading[] laser_readings;

  

        /// short
        public short angle_start;
        public short angle_stop;

        public int left_start;
        public int right_start;
        public int left_stop;
        public int right_stop;
    }



    
    
}
