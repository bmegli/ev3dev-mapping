# XV11 LIDAR test

This is code for XV11 lidar test. The test outputs semicolon separated distance data for angles 0-359.
The output can be redirected to file, transfered to PC and plotted.

The files xv11lidar.h and xv11lidar.c can be used as simple library to communicate with the lidar.

Assumptions made in instructions below:
- hardware is connected as in http://www.ev3dev.org/docs/tutorials/using-xv11-lidar/
- lidar data connector is connected to port 1
- lidar motor interface is avaliable at /sys/class/tacho-motor/motor0 

Instructions are for compiling the code directly on EV3 (get the files to EV3 and use ssh)

1. Compile the code - enter directory and type `make`
2. Put the port in `other-uart` mode
`echo other-uart > /sys/class/lego-port/port0/mode`
3. Spin the motor around 200-300 RPM CCW
`echo 40 /sys/class/tacho-motor/motor0/duty_cycle_sp`
`echo run-direct /sys/class/tacho-motor/motor0/command`
4. Run xv11test
`./xv11test /dev/tty_in1`
5. Run xv11test and redirect output to file
`./xv11test /dev/tty_in1 > distances.csv`
6. Stop the motor 
`echo stop /sys/class/tacho-motor/motor0/command`
7. You can get the output csv file to OpenOffice Calc/Excel or anything you want and plot the result 