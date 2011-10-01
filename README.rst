THUNDERCRACKER
==============

This is the software toolchain for Sifteo's 2nd generation product.


Parts
-----

emulator
  Accurate simulation models for the cubes and master.

firmware
  Firmware source for cubes and master.

sdk
  Development kit for game software, running on the master.

stir
  Our asset preparation tool, the Sifteo Tiled Image Reducer.

deps
  Outside dependencies, included for convenience.

attic
  Old or experimental code, not part of the main toolchain.

hw
  Hardware schematics and layout


Build
-----

Running "make" in this top-level directory will build all Thundercracker
software, including both native and ARM builds of the master firmware.

Various dependencies are required:

1. gcc and g++, for compiling native binaries
2. SDL, for the simulated graphics output in simcube
3. ncurses or pdcurses, for the debug console in simcube
4. SDCC, a microcontroller cross-compiler used to build the cube firmware
5. gcc for ARM (arm-none-eabi-gcc), used to build master-cube firmware

Optional dependencies:

1. OpenOCD, for installing and debugging master firmware
2. The Python Imaging Library, used by the tilerom generator and elsewhere

Most of these dependencies are very easy to come by, and your favorite
Linux distro or Mac OS package manager has them already. The ARM cross
compiler is usually more annoying to obtain. On Linux and Windows, the
CodeSourcery C++ distribution is preferred. On Mac OS or Linux, the following
script will automatically build a compatible toolchain for your machie:

   https://github.com/jsnyder/arm-eabi-toolchain


