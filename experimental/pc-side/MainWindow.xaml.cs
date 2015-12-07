using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Net.Sockets;
using System.Net;
using System.Runtime.InteropServices;
using System.Threading;

namespace LaserScanner
{
    delegate void RefresherHandler(object sender, RobotFrame frame);

    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        #region External
        [DllImport("kernel32.dll", EntryPoint = "RtlMoveMemory")]
        public static extern void CopyMemory(IntPtr destination, IntPtr source, uint length);
        #endregion

        private const int MAX_PATH_SEGMENTS = 500;
        private const int UDP_PORT = 5679;
        private const double POINT_RADIUS = 1.0;
        
        private const double CENTER_RADIUS = 16.0;
        private const double POINTER_LENGTH = 16.0;

        bool runReceiverThread = true;
        private Thread receiverThread;
        UdpClient udpSocket = new UdpClient(UDP_PORT);

        private Ellipse center = new Ellipse();
        private Line pointer = new Line();
        private Ellipse[] meters=new Ellipse[6];
        private Ellipse[] points = new Ellipse[360];


        //   private List<Ellipse> map = new List<Ellipse>();

        private Point[] map_update = new Point[RobotFrame.LASER_FRAMES_PER_PACKET * 4];
        private int map_point_to_update = 0;
        private const int MAP_WIDTH = 1920*2;
        private const int MAP_HEIGHT = 1080*2;
        private WriteableBitmap map = new WriteableBitmap(MAP_WIDTH, MAP_HEIGHT, 96, 96, PixelFormats.Bgra32, null);
        
        private const double map_center_x = MAP_WIDTH / 2;
        private const double map_center_y = MAP_HEIGHT / 2;
        
        private Path robotPath = new Path();
        private GeometryGroup robotPathRepresentation = new GeometryGroup();

        private Point positionEstimate = new Point(0, 0);
        private RobotFrame[] frames = new RobotFrame[RobotFrame.LASER_FRAMES_PER_360 / RobotFrame.LASER_FRAMES_PER_PACKET];
    
        private CircularBuffer speedBuffer = new CircularBuffer(6);

        private Point viewOffset = new Point(0, 0);
        public Point ViewOffset
        {
            get { return viewOffset; }
            set
            {
                viewOffset = value;
                double centerx = laserCanvas.ActualWidth / 2.0;
                double centery = laserCanvas.ActualHeight / 2.0;

                Canvas.SetLeft(mapImage, centerx - MAP_WIDTH / 2+viewOffset.X);
                Canvas.SetTop(mapImage, centery - MAP_HEIGHT / 2+viewOffset.Y);                
            }
        }

        public MainWindow()
        {
            InitializeComponent();
            InitializeScan();        
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            StartReceiver();
        }

        private void Window_Closed(object sender, EventArgs e)
        {
            runReceiverThread = false;
            udpSocket.Close();
        }

        void UpdateMap()
        {
            map.Lock();

            IntPtr data = map.BackBuffer;
            int i,x,y, points_updated=0;
            uint color = 0xFFFFFFFF;
            int minx = int.MaxValue, maxx = 0, miny = int.MaxValue, maxy = 0;       
                     
            unsafe
            {
                uint* pdata = (uint*)data.ToPointer();
   
                for(int p=0;p<map_point_to_update;++p)
                {
                    x = (int)map_update[p].X;
                    y = (int)map_update[p].Y;
                    
                    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT)
                        continue;

                    i = MAP_WIDTH * y + x;
                    pdata[i] = color;
                    if (x >= maxx)
                        maxx = x;
                    if (x <= minx)
                        minx = x;
                    if (y >= maxy)
                        maxy = y;
                    if (y <= miny)
                        miny = y;
                    ++points_updated;
                }
     
            }
            if(points_updated>0)
                map.AddDirtyRect(new Int32Rect(minx, miny, maxx-minx+1, maxy-miny+1));

            map.Unlock();      
        }

        public void InitializeScan()
        {
            mapImage.Width = MAP_WIDTH;
            mapImage.Height = MAP_HEIGHT;
            mapImage.Source = map;

            for (int i = 0; i < points.Length; ++i)
            {
                points[i] = new Ellipse();
                points[i].Width = 2*POINT_RADIUS;
                points[i].Height = 2*POINT_RADIUS;
                points[i].Stroke = Brushes.Green;
                laserCanvas.Children.Add(points[i]);                   
            }
            center.Height = 2* CENTER_RADIUS;
            center.Width =  2 * CENTER_RADIUS;
            center.Stroke = Brushes.Blue;
            laserCanvas.Children.Add(center);

            for (int i = 0; i < meters.Length; ++i)
            {
                meters[i] = new Ellipse();
                meters[i].Width=meters[i].Height = (i + 1) * 200;
                meters[i].Stroke = Brushes.DarkGray;
                meters[i].StrokeThickness = 0.1;
                laserCanvas.Children.Add(meters[i]);   
            }

            pointer.Stroke = Brushes.Yellow;
            laserCanvas.Children.Add(pointer);
            
            robotPath.Stroke = Brushes.Gray;
            robotPath.StrokeThickness = 0.5;

            robotPath.Data = robotPathRepresentation;
            robotPath.Visibility = Visibility.Collapsed;

            laserCanvas.Children.Add(robotPath);                   
        }

 
        public void RefreshReadings(object sender, RobotFrame frame)
        {
            map_point_to_update = 0;
            const double deg2rad = Math.PI / 180.0;

            double centerx = laserCanvas.ActualWidth / 2.0;
            double centery = laserCanvas.ActualHeight / 2.0;
       
            double middlex = centerx - POINT_RADIUS;
            double middley = centery - POINT_RADIUS;

            laserRPMTextBlock.Text = frame.laser_speed.ToString();
            double packet_time_sec = (double)frame.elapsed_ms / 100000.0;

            const double ENCODER_SCALE_FACTOR = Math.PI * 4.32 / 360.0;
            double ldiff = frame.left_stop - frame.left_start;
            double rdiff = frame.right_stop - frame.right_start;
            double displacement_cm = (ldiff + rdiff) * ENCODER_SCALE_FACTOR / 2.0;

            speedBuffer.Add(displacement_cm / packet_time_sec);
            speedTextBlock.Text = (-speedBuffer.Average).ToString("F2");

            double gyro_start = frame.angle_start / 100.0;
            double gyro_stop = frame.angle_stop / 100.0;
            double angle_difference = (gyro_stop - gyro_start);
            if (Math.Abs(angle_difference) > 180.0)
                angle_difference = Math.Sign(angle_difference) * 360.0 - angle_difference;

            angleTextBlock.Text = gyro_stop.ToString();
            double gyro_rad = gyro_stop * deg2rad;
            double gyro_average_rad = (gyro_start +  angle_difference/ 2.0) * deg2rad;

            double max_mapping_distance = 10000;
            if (maxMappingDistanceDoubleUpDown.Value.HasValue)
                max_mapping_distance = maxMappingDistanceDoubleUpDown.Value.Value;

            int point_index;
  
            LaserReading[] readings = frame.laser_readings;

            for (int i = 0; i < readings.Length; ++i)
            {
                point_index = i + frame.laser_angle * 4;

                if (readings[i].unknown_warning==1)
                    Console.Out.WriteLine("Uknown warning flag triggered! ");

                if (readings[i].invalid_data == 1)
                {
                    points[point_index].Visibility = Visibility.Hidden;
                    continue;//readings[i].distance = 160;
                }
                else
                    points[point_index].Visibility = Visibility.Visible;

                points[point_index].Stroke = DeterminePointColor(readings[i]);
             
                double angle = point_index;

               
                double angle_offset = gyro_start + i * angle_difference / (readings.Length - 1);

                if (absoluteRotationCheckBox.IsChecked == true)
                    angle -= angle_offset;             

                double rad = deg2rad * angle;
                double y, x;
                if (correctGeometryCheckBox.IsChecked == false)
                {
                    y = -Math.Cos(rad) * readings[i].distance / 10.0;
                    x = -Math.Sin(rad) * readings[i].distance / 10.0;
                }
                else //(correctGeometryCheckBox.IsChecked == true)
                {
                    double yp = -Math.Cos(rad) * readings[i].distance;
                    double xp = -Math.Sin(rad) * readings[i].distance;

                    double alpha = 180 - 82 + angle;
                    double alpharad = deg2rad * alpha;
                    double b = 25;

                    x = (xp + b * Math.Sin(alpharad)) / 10 ;
                    y = (yp + b * Math.Cos(alpharad)) / 10 ;
                }

                if(absolutePositionCheckBox.IsChecked==true)
                {
                    double angle_offset_rad = deg2rad * angle_offset;          
                    double displacement_offset = displacement_cm * (double)i /(double)(readings.Length - 1);

                    x += positionEstimate.X  - displacement_offset * Math.Sin(gyro_average_rad);
                    y += positionEstimate.Y  + displacement_offset * Math.Cos(gyro_average_rad);

                    if (mapEstimateCheckBox.IsChecked == true && ((double)readings[i].distance/10.0)<max_mapping_distance)
                    {                       
                        map_update[map_point_to_update].X = x+map_center_x;
                        map_update[map_point_to_update].Y = y+map_center_y;
                        ++map_point_to_update;
                    }

                }

                Canvas.SetLeft(points[point_index], x + middlex + viewOffset.X);
                Canvas.SetTop(points[point_index], y + middley + viewOffset.Y);
                
            }
            RefreshPath(displacement_cm, gyro_average_rad, gyro_stop, centerx, centery);
            RefreshPointer(centerx, centery, gyro_rad, speedBuffer.Average);

            if (mapEstimateCheckBox.IsChecked == true && map_point_to_update > 0)
            {
                if(mapMovingCheckBox.IsChecked==true || displacement_cm==0 && gyro_start==gyro_stop)
                    UpdateMap();               
            }
        }

        public void RefreshPointer(double cx, double cy, double gyro_rad, double speed)
        {
            double centerx = cx;
            double centery = cy;

            if (absolutePositionCheckBox.IsChecked == true)
            {
                centerx += positionEstimate.X + viewOffset.X;
                centery += positionEstimate.Y + viewOffset.Y;
            }

            Canvas.SetLeft(center, centerx - CENTER_RADIUS);
            Canvas.SetTop(center, centery - CENTER_RADIUS);

            pointer.X1 = pointer.X2 = centerx;
            pointer.Y1 = centery;
            pointer.Y2 = centery - POINTER_LENGTH;


            if (absoluteRotationCheckBox.IsChecked == true)
            {
                pointer.X2 = centerx + POINTER_LENGTH * Math.Sin(gyro_rad);
                pointer.Y2 = centery - POINTER_LENGTH * Math.Cos(gyro_rad);
            }
        }

        void RefreshPath(double displacement, double gyro_rad, double gyro_angle, double centerx, double centery)
        {
            Point oldPosition = positionEstimate;
            positionEstimate = new Point(oldPosition.X - displacement * Math.Sin(gyro_rad), oldPosition.Y + displacement * Math.Cos(gyro_rad));

            /*
            if (robotPathRepresentation.Children.Count > MAX_PATH_SEGMENTS)
                robotPathRepresentation.Children.RemoveAt(0);

            robotPathRepresentation.Children.Add(new LineGeometry(oldPosition, positionEstimate));

            if (absoluteRotationCheckBox.IsChecked == true)
                robotPath.RenderTransform = new TranslateTransform(-positionEstimate.X + centerx, -positionEstimate.Y + centery);
            else
            {
                TransformGroup tg = new TransformGroup();
                tg.Children.Add(new TranslateTransform(-positionEstimate.X + centerx, -positionEstimate.Y + centery));
                tg.Children.Add(new RotateTransform(-gyro_angle, centerx, centery));
                robotPath.RenderTransform = tg;
            }
            */

        }

        public Brush DeterminePointColor(LaserReading r)
        {
            if (r.strength_warning == 1)
                return Brushes.Orange;

            if (r.invalid_data == 1)
                return Brushes.Red;

            return Brushes.Green;
        }

        private void laserCanvas_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            double centerx = laserCanvas.ActualWidth / 2.0;
            double centery = laserCanvas.ActualHeight / 2.0;

            Canvas.SetLeft(mapImage, centerx - MAP_WIDTH / 2 + viewOffset.X);
            Canvas.SetTop(mapImage, centery - MAP_HEIGHT / 2 + viewOffset.Y);
    

            for (int i = 0; i < meters.Length; ++i)
            {
                Canvas.SetLeft(meters[i], centerx - 100 * (i + 1));
                Canvas.SetTop(meters[i], centery - 100 * (i + 1));
            }



        }


        private void robotPathCheckBox_Checked(object sender, RoutedEventArgs e)
        {
            if (robotPathCheckBox.IsChecked == true)
                robotPath.Visibility = Visibility.Visible;
            else
                robotPath.Visibility = Visibility.Collapsed;
        }

        private void absolutePositionCheckBox_Checked(object sender, RoutedEventArgs e)
        {
            positionEstimate = new Point(0, 0);
        }

        #region Receiver

        void StartReceiver()
        {
            if (receiverThread != null)
            {
                MessageBox.Show("already running");
                return;
            }
            receiverThread = new Thread(new ThreadStart(ReceiverThread));
            runReceiverThread = true;
            receiverThread.Start();
        }


        public void ReceiverThread()
        {
            IPEndPoint remote = new IPEndPoint(IPAddress.Any, UDP_PORT);
            byte[] data = null;

            while (runReceiverThread)
            {
                try
                {
                    data = udpSocket.Receive(ref remote);
                    if (remote.Port != UDP_PORT)
                        continue;

                }
                catch (System.Net.Sockets.SocketException e)
                {
                    continue;
                    //socket closed, finish execution if run is false
                }


                if (data.Length != Marshal.SizeOf(frames[0]))
                {
                    MessageBox.Show(".Net frame=" + Marshal.SizeOf(frames[0]).ToString() + " received=" + data.Length.ToString());
                    continue;
                }

                GCHandle handle = GCHandle.Alloc(data, GCHandleType.Pinned);
                RobotFrame frame = new RobotFrame();

                try
                {
                    IntPtr ptr = handle.AddrOfPinnedObject();
                    frame = (RobotFrame)Marshal.PtrToStructure(ptr, typeof(RobotFrame));
                }
                finally
                {
                    handle.Free();
                }

                laserCanvas.Dispatcher.BeginInvoke(new RefresherHandler(RefreshReadings), null, frame);
            }
        }
        #endregion

        unsafe private void resetButton_Click(object sender, RoutedEventArgs e)
        {
            uint[] blank = new uint[MAP_WIDTH * MAP_HEIGHT];
            fixed(uint *b=blank)
            {
                CopyMemory(map.BackBuffer, (IntPtr)b, MAP_WIDTH * MAP_HEIGHT * 4);
            }
          
            map.Lock();
            map.AddDirtyRect(new Int32Rect(0, 0, map.PixelWidth, map.PixelHeight));
            map.Unlock();
           


        }

        private bool mouseDragging = false;
        private Point draggingStart=new Point(0,0);
        private Point viewStart;

        private void laserCanvas_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            if(!mouseDragging && e.ButtonState==MouseButtonState.Pressed)
            {
                mouseDragging = true;
                draggingStart = e.GetPosition(laserCanvas);
                viewStart = ViewOffset;
            } 
        }

        private void laserCanvas_MouseMove(object sender, MouseEventArgs e)
        {
            if(mouseDragging)
            {
                Vector diff = Point.Subtract(e.GetPosition(laserCanvas), draggingStart);
                Point newViewOffset = Point.Add(viewStart, diff);
                ViewOffset = newViewOffset;
            }
        }

        private void laserCanvas_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {        
             mouseDragging = false;
        }

        private void laserCanvas_MouseLeave(object sender, MouseEventArgs e)
        {
            mouseDragging = false;
        }

        private void mapEstimateCheckBox_Checked(object sender, RoutedEventArgs e)
        {
            mapImage.Visibility = (mapEstimateCheckBox.IsChecked == true) ? Visibility.Visible : Visibility.Collapsed;
        }
    }
}
