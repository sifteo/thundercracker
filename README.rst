THUNDERCRACKER
==============

This is the software toolchain for Sifteo's 2nd generation product.


Parts
-----

emulator
  The Thundercracker hardware emulator. Includes an accurate
  hardware simulation of the cubes, and the necessary glue to
  execute "sim" builds of master firmware in lockstep with this
  hardware simulation. Also includes a unit testing framework.
  
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

  
Operating System
----------------
  
The code here should all run on Windows, Mac OS X, or Linux. Right now
the Linux port is infrequently maintained, but in theory it should
still work. In all cases, the build is Makefile based, and we compile
with some flavor of GCC.


Build
-----

Running "make" in this top-level directory will by default build all
Thundercracker software that isn't cross-compiled.

To build the optional cross-compiled firmwares, set the environment
variables BUILD_8051 and/or BUILD_STM32.

Various dependencies are required:

1. A build environment; make, shell, etc. Use MSYS on Windows.
2. GCC, for building native binaries. Should come with (1).

Optional dependencies:

1. SDCC, a microcontroller cross-compiler used to build the cube firmware
2. gcc for ARM (arm-none-eabi-gcc), used to build master-cube firmware.
3. OpenOCD, for installing and debugging master firmware
4. Python, for some of the code generation tools
5. The Python Imaging Library, used by other code generation tools

Most of these dependencies are very easy to come by, and your favorite
Linux distro or Mac OS package manager has them already. The ARM cross
compiler is usually more annoying to obtain. On Linux and Windows, the
CodeSourcery C++ distribution is preferred. On Mac OS or Linux, the following
script will automatically build a compatible toolchain for your machie:

   https://github.com/jsnyder/arm-eabi-toolchain


Running Tests
-------------

The simulator has Lua scripting capabilities, and we have a testing
framework written in Lua which currently covers the graphics code in
our cube firmware. To run all existing tests, from the "emulator"
directory:

  tc-siftulator [-f ../firmware/cube.hex] -e scripts/tests.lua
  
You'll see the tests run headless, with pass/fail information on the
console. To run tests with the GUI frontend enabled, set the
USE_FRONTEND environment variable. To specify a particular test to
run, set the TEST environment var to the name of the test, in
"TestClass:test_function" format.
