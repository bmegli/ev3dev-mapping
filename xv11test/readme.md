# XV11 LIDAR test

## Overview

This is code for XV11 lidar test. The test outputs semicolon separated distance data for angles 0-359.
The output can be redirected to file, transfered to PC and plotted.

The files xv11lidar.h and xv11lidar.c can also be used as a simple library to communicate with the lidar.

## LIDAR test

### Instructions Assumptions 
- hardware is connected as in http://www.ev3dev.org/docs/tutorials/using-xv11-lidar/
- lidar data connector is connected to port 1
- lidar motor interface is avaliable at `/sys/class/tacho-motor/motor0`

### Building the xv11test

Instructions are for compiling the code directly on EV3 (get the files to EV3 and use ssh)

- Get the build system (or just get the gcc and make package instead)
```bash
sudo apt-get update
sudo apt-get install build-essential
```
- Compile the code - enter directory and type `make`

### Running the test

- Put the port in `other-uart` mode
```bash
 echo other-uart > /sys/class/lego-port/port0/mode
```
- Spin the motor around 200-300 RPM CCW
```bash
echo 40 > /sys/class/tacho-motor/motor0/duty_cycle_sp
echo run-direct > /sys/class/tacho-motor/motor0/command
```
- Run xv11test
```bash
./xv11test /dev/tty_in1
```
- Run xv11test and redirect output to file
```bash
./xv11test /dev/tty_in1 > distances.csv
```
- Stop the motor
```bash 
echo stop > /sys/class/tacho-motor/motor0/command
```

### Results plot

The file `xv11plot.ods` is Open Document Spreadsheet with the formulas for converting xv11test angle/distance output to 2D points graph.
You can open `xv11plot.ods` in OpenOffice Calc, Microsoft Excel or any other software supporting Open Document Spreadsheets.

To make a plot:
- Get the xv11test output `distances.csv` file to your PC
- Open `xv11plot.ods` in software of your choice (e.g. OpenOffice Calc, Microsoft Excel, ...) 
- Copy your result from `distances.csv` to lines in `xv11plot.ods` where it says so

See your own plot and note how to convert LIDAR angle/distance output and apply geometric correction in the spreadsheet

### Example - 910 mm x 600 mm cardboard box 

![Alt text](/img/xv11plot.png "XV11 scan sample image")
