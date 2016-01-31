# ev3dev-mapping

Comes "as is".

## How it works

Inits 2 large motors, laser motor, CruizCore gyroscope, laser port, laser tty
Synchronizes with the laser, sets terminal to supply some multiplicity of laser packets.
In a loop reads from laser, motors, gyroscope, timestamps data and sends through UDP for futher processing to PC

Ip addresses are hardocoded. Laser duty cycle is argument to program (positive or negative integer).

## Notes On Building

1. Can be build with command:

`g++ main.cpp ev3dev.cpp -DEV3 -O2 -Wall -march=armv5 -std=c++11 -D_GLIBCXX_USE_NANOSLEEP`

- DEV3 is for symbol defined in main.cpp
- O2 is for optimization
- Wall is all warnings
- march=armv5 is for crosscompiling for EV3
- std=c++11 is for C++11 standard, requried by ev3dev library
- D_GLIBCXX_USE_NANOSLEEP, I don't even remember now


* ev3dev.h and ev3dev.cpp were compatible with ev3-ev3dev-jessie-2015-09-13. For more recent kernel you may have to overwrite them with newer versions.
* Some paths to devices may not be valid in future ev3dev releases.