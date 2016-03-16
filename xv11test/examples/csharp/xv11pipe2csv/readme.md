# xv11pipe2csv

## Overview

This program is C# example for consuming output of xv11test with -raw argument

This program reads from standard input expecting synchronized raw binary lidar data

It will output semicolon separated distances for angles 0-359 
 
## Getting Started

### Instructions Assumptions 
- hardware is connected as in http://www.ev3dev.org/docs/tutorials/using-xv11-lidar/
- lidar data connector is connected to port 1
- lidar motor interface is avaliable at `/sys/class/tacho-motor/motor0`
- xv11test was compiled and is ready to use
- both xv11test and xv11pipe2csv.exe are in the same directory

### Building xv11pipe2csv.exe

Instructions are for compiling the code on PC (and later run it on EV3).

If you are wondering whether it is possible to build the .exe with Visual Studio on Windows, copy to EV3 and run on Mono the answer is yes - you can.

You have several options, pick one.

#### Building on Windows with nmake and csc

Assumption: you have Visual Studio (e.g. VS Express 2015 for Desktop) installed.

- open Developer Command Prompt for Visual Studio
- cd to directory with xv11pipe2csv source
- run nmake
```bash
nmake
```
#### Building on Linux with Mono and make

Assumption: you have Mono installed
This will also work on Windows from Mono Command Prompt provided that you have make in the path

- cd to directory with xv11pipe2csv source
- edit the Makefile and comment the line:

`CSHARP = csc #for Microsoft Visual C# Compiler (use from Developer Command Prompt for Visual Studio)`

uncomment the line:

`#CSHARP = mcs #for Mono, uncomment if needed (use from Mono Command Prompt)`

- run make
```bash
make
```

#### Building on Windows from Visual Studio

Assumption: you have Visual Studio (e.g. VS Express 2015 for Desktop) installed.

- create Visual C# Console Application
- remove all references except System
- add Program.cs and LaserReading.cs to the project
- Build the project

This is method that I use along with Post-build event copying output file to EV3 using pscp

#### Building by hand with Microsoft csc or Mono mcs

Assumption: csc or mcs in the path

- cd to directory with xv11pipe2csv source
- run csc or mcs

With Microsoft csc:

```bash
csc  Program.cs LaserReading.cs -out:xv11pipe2csv.exe
```

With Mono mcs:

```bash
mcs  Program.cs LaserReading.cs -out:xv11pipe2csv.exe
```

### Installing Mono on EV3

```bash
sudo apt-get update
sudo apt-get install mono-runtime
```

`mono-runtime` package is enough to run the examples.
If you run into missing assemblies problems (with own code) get `mono-complete` package:

```bash
sudo apt-get update
sudo apt-get install mono-complete
```

### Running the test

- Copy `xv11pipe2csv.exe` that you build to EV3 directory with `xv11test`
- Put the port in `other-uart` mode
```bash
 echo other-uart > /sys/class/lego-port/port0/mode
```
- Spin the motor around 200-300 RPM CCW
```bash
echo 40 > /sys/class/tacho-motor/motor0/duty_cycle_sp
echo run-direct > /sys/class/tacho-motor/motor0/command
```
- Pipe xv11test -raw output to xv11pipe2csv
```bash
./xv11test /dev/tty_in1 -raw | mono xv11pipe2csv.exe
```
- Pipe xv11test -raw output to xv11pipe2csv and redirect to file
```bash
./xv11test /dev/tty_in1 -raw | mono xv11pipe2csv.exe > distances.csv
```
- Stop the motor
```bash 
echo stop > /sys/class/tacho-motor/motor0/command
```

