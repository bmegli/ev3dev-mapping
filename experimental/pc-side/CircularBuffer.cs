using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LaserScanner
{
    public class CircularBuffer
    {
        private double[] data;
        private int nextFree;
        private double sum;

        public CircularBuffer(int length)
        {
            data = new double[length];
            nextFree = 0;
            sum = 0;
        }

        public void Add(double x)
        {
            sum -= data[nextFree];
            data[nextFree] = x;
            sum += x;
            nextFree = (nextFree + 1) % data.Length;
        }
        public double Average
        {
            get {return sum/data.Length;}
        }
    }
}
