# xv11pipe-loop

## Overview

This program is C example for consuming unlimited output of xv11test with -raw argument

Please note that in C you can just use xv11lidar.h and xv11lidar.c from xv11test as a small C LIDAR library which is a better way.

This program reads from standard input expecting synchronized raw binary lidar data.

It will output continuously distance at LIDAR angle 0 or 'e' on reading error.
Note that this is almost but not exactly ahead of the LIDAR (geometric correction needs to be applied).
 
## Getting Started

### Instructions Assumptions 
- hardware is connected as in http://www.ev3dev.org/docs/tutorials/using-xv11-lidar/
- lidar data connector is connected to port 1
- lidar motor interface is avaliable at `/sys/class/tacho-motor/motor0`
- xv11test was compiled and is ready to use
- both xv11test and xv11pipe-loop are in the same directory

### Building xv11pipe-loop

Instructions are for compiling the code directly on EV3 (get the files to EV3 and use ssh)

- Get the build system (or just get the gcc and make package instead)
```bash
sudo apt-get update
sudo apt-get install build-essential
```
- Compile the code - enter directory and type `make`

### Running the test

- Copy `xv11pipe-loop` that you build to EV3 directory with `xv11test`
- Put the port in `other-uart` mode
```bash
 echo other-uart > /sys/class/lego-port/port0/mode
```
- Spin the motor around 200-300 RPM CCW
```bash
echo 40 > /sys/class/tacho-motor/motor0/duty_cycle_sp
echo run-direct > /sys/class/tacho-motor/motor0/command
```
- Pipe xv11test -raw xx 0 output to xv11pipe-loop
```bash
./xv11test /dev/tty_in1 -raw 15 0 | ./xv11pipe-loop
```
- Stop the programs with ctrl + c
- Stop the motor
```bash 
echo stop > /sys/class/tacho-motor/motor0/command
```
