I am assuming here that the lidar is connected to port 1

1. Compile the code - enter directory and type `make`
2. Put the port in `other-uart` mode
`echo other-uart > /sys/class/lego-port/port0/mode`
3. Spin the motor around 200-300 RPM CCW
4. Run xv11test
`./xv11test /dev/tty_in1`
5. Run xv11test and redirect output to file
`./xv11test /dev/tty_in1 > distances.csv`
6. You can get the output csv file to OpenOffice Calc/Excel or anything you want and plot the result 