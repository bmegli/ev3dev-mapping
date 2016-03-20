# xv11pipe-loop

## Overview

This program is Java example for consuming unlimited output of xv11test with -raw argument

This program reads from standard input expecting synchronized raw binary lidar data

It will output continuously distance at LIDAR angle 0 or 'e' on reading error.
Note that this is almost but not exactly ahead of the LIDAR (geometric correction needs to be applied).
 
## Getting Started

### Instructions Assumptions 
- hardware is connected as in http://www.ev3dev.org/docs/tutorials/using-xv11-lidar/
- lidar data connector is connected to port 1
- lidar motor interface is avaliable at `/sys/class/tacho-motor/motor0`
- xv11test was compiled and is ready to use
- both xv11test and xv11pipe-loop.jar are in the same directory

### Installing Java on EV3

To install JRE along with command line tools for development:

```bash
sudo apt-get update
sudo apt-get install default-jdk
```

If you only intend to compile from your PC it's enough to get jre.

```bash
sudo apt-get update
sudo apt-get install default-jre
```

### Building xv11pipe-loop.jar

Instructions are for compiling the code on EV3

The makefile will also work on your PC provided that:
- you have `javac`, `jar` and `make` tools in your path  
- the jdk version on your PC matches the jre/jdk on EV3 (alternatively modify makefile to cross-compile)

#### Building on EV3

Assumption: jdk installed on EV3

- cd to directory with java examples (with this readme.md)
- run make
```bash
make
```

This will take a minute or two on EV3!

### Running the test

- Copy `xv11pipe-loop.jar` that you build to EV3 directory with `xv11test`
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
./xv11test /dev/tty_in1 -raw 15 0 | java -jar xv11pipe-loop.jar
```
- Stop the programs with ctrl+z
- Stop the motor
```bash 
echo stop > /sys/class/tacho-motor/motor0/command
```

