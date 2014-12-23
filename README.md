Thundercracker
==============

This repository contains the software toolchain and firmware for Sifteo's 2nd generation product, a tangible video game system based on a wireless network of STM32 and nRF24LE1 microcontrollers.

![Photo of Sifteo cubes](https://raw.github.com/sifteo/thundercracker/master/docs/sifteo.jpg)

Foreword
--------

*by [Micah Scott](https://github.com/scanlime), firmware engineer formerly at Sifteo.*

The code in this repository was our attempt to do something impossible. From late 2011 through early 2013, a very small team of engineers did their best to create a new kind of video game system. To keep costs down we needed to use the smallest and lowest-power chips we could. I took inspiration from the video game consoles and personal computers of the 1980s and 1990s, but we used modern compiler and compression technologies to get the most from our 8-bit CPUs and kilobytes of RAM.

 * [Article: *How we built a Super Nintendo out of a wireless keyboard*](http://www.adafruit.com/blog/2012/12/05/how-we-built-a-super-nintendo-out-of-a-wireless-keyboard-sifteo-sifteo/)

The system's graphics and interaction come from a wireless network of "cubes" built using the same technologies that power other systems of wireless sensor nodes. Our cubes, however, needed to generate fluid real-time graphics with a minimum of CPU and battery power. As a result, the cube firmware became a highly optimized 2D graphics rendering machine. The simple hardware technique of sharing a single parallel bus between the CPU, LCD, and Flash memory enabled much faster graphics than a microcontroller could produce using a purely software approach.

This strange and unique graphics engine required a whole host of new tools to develop: a cycle-accurate CPU emulator and hardware emulator, a static binary translator and static verification tools, new compression protocols and new unit testing infrastructure.

 * [Sample code from deep inside the cube's graphics engine](https://github.com/sifteo/thundercracker/blob/master/firmware/cube/src/graphics_bg1_line.c)

These "cubes" act somewhat like a simple remote-controlled GPU. The game itself runs on a more powerful microcontroller, the "master" block. This is twice the size of a cube, with more battery power available. The master block runs games inside a new kind of virtual machine sandbox. Its simple operating system handles radio communications, storage, music, USB communications.

We wanted to create a robust "app" sandbox for running games, but we couldn't afford the price of hardware memory protection or the risk of allowing game code to have direct access to hardware that may be damaged by incorrect use. Our solution was strange and powerful. We called it "SVM". Most of a game's instructions ran natively at full speed, with some common operations implemented using privileged system calls. I wrote an LLVM backend to generate these specially formatted instructions and pack them into small blocks that could be cached and demand-paged in software from external serial flash memory.

 * [Sifteo VM Instruction Set Architecture document](https://github.com/sifteo/thundercracker/blob/master/vm/doc/sifteo-vm-isa.txt)

[Liam Staskawicz](https://github.com/liamstask) and I designed and built this monumental architecture. For better or worse, we were very careful about which other projects we depended on. Both of us come from an Open Source background and feel just a bit awful any time we have to use ambiguously-licensed code from some silicon vendor or another. So, when we built Thundercracker, we put in the extra time to build it all on a solid foundation. We created our own bootloader, our own platform support libraries, our own system simulator. We built on other open source tools, but in a way that we hoped could be eventually returned to the community.

That time has come. It saddens us that Sifteo Cubes were not the commercial success we hoped for, but at the same time I feel very grateful that we can share some of the amazing work we did to make the product real. I hope this code brings you some amusement at least, if not some utility.

Many of the components in this codebase are designed with reusability in mind. We've written a filesystem, operating system, several compression algorithms, graphics engine, simulator, linker, drivers for the nRF24L01 and nRF8001 radios, and so much more.

We can't provide any official support for this code, but I still care and I'll try to answer questions when I can. Who knows, perhaps Sifteo Cubes will live on as an Arduino shield or a swarm of remote control cars or maybe just a really esoteric easter egg.


Getting Started
---------------

If you aren't already familiar with the public documentation for the Sifteo SDK, best to check that out first. The Doxygen sources live in the `docs` directory, or you can view it online:

* [Sifteo SDK documentation](https://sifteo.github.io/thundercracker)

Most of the tools necessary to build the SDK are included in this repository, in the `deps` directory. There are some exceptions, like Doxygen. On Mac OS, you can install this with "[brew](http://brew.sh/) install doxygen".

In the top-level Thundercracker directory, type `make`. This will build the firmware, compiler, asset preparation tools, documentation, and unit tests. Most of the results will be in the `sdk` directory.


Parts
-----

* `deps` – Outside dependencies, included for convenience.
* `docs` – Build files for the SDK documentation, as well as other miscellaneous platform documentation.
* `emulator` – The Thundercracker hardware emulator. Includes an accurate hardware simulation of the cubes, and the necessary glue to execute "sim" builds of master firmware in lockstep with this hardware simulation. Also includes a unit testing framework.
* `extras` – Various userspace programs which are neither full games nor included in the SDK as "example" code. This directory can be used for internal tools and toys which aren't quite up to our standards for inclusion in the SDK.
* `firmware` – Firmware source for cubes and master.
* `launcher` – Source for the "launcher" app, the shell which contains the game selector menu and other non-game functionality that's packaged with the system. This is compiled with the SDK as an ELF binary.
* `sdk` – Development kit for game software, running on the master. This directory is intended to contain only redistributable components. During build, pre-packaged binaries, built binaries, and built docs are copied here.
* `stir` – Our asset preparation tool, the Sifteo Tiled Image Reducer.
* `test` – Unit tests for firmware, SDK, and simulator.
* `tools` – Internal tools for SDK packaging, factory tests, etc.
* `vm` – Tools and documentation for the virtual machine sandbox that games execute in. Includes "slinky", the Sifteo linker and code generator for LLVM.


Operating System
----------------

The code here should all run on Windows, Mac OS X, or Linux. Right now
the Linux port is infrequently maintained, but in theory it should
still work. In all cases, the build is Makefile based, and we compile
with some flavor of GCC.


Build
-----

Running "make" in this top-level directory will by default build all
firmware as well as the SDK.

Various dependencies are required:

1. A build environment; make, shell, etc. Use MSYS on Windows.
2. GCC, for building native binaries. Should come with (1).
3. SDCC, a microcontroller cross-compiler used to build the cube firmware
4. gcc for ARM (arm-none-eabi-gcc), used to build master-cube firmware.
5. Python, for some of the code generation tools
6. Doxygen, to build the SDK documentation
7. UPX (Linux only) for packing executables

Optional dependencies:

1. OpenOCD, for installing and debugging master firmware
2. The Python Imaging Library, used by other code generation tools

Linux only:

- The UUID Library for the generation of Universally Unique Identifiers
- Libusb allows accessing to USB devices on most operating systems
- Mesa is an open-source OpenGL, it's used in the simulator
- GLU (OpenGL Utility Library), allows interfaces for building mipmaps
- Libasound, a library to use ALSA, Advanced Linux Sound Architecture
- The ia32-libs package is needed in 64 bits computers

On most debian-like distros, you still need the arm toolchain (see below)
but the following command should install all of the useful deps (if you
already have one it will be skipped)::

    sudo apt-get install -y g++ doxygen upx-ucl python-imaging openocd \
                 uuid-dev libusb-1.0-0-dev mesa-common-dev \
                 libglu1-mesa-dev libasound2-dev ia32-libs

ARM toolchain
-------------

On all platforms, the ARM GCC Embedded distribution is preferred.
Binaries are available at:

   https://launchpad.net/gcc-arm-embedded

Environment Variables
---------------------

BOOTLOADABLE
  Generate a binary compatible with the USB bootloader (Not JTAG)

BOARD
  Define the board to target. If unset, uses the default board in stm32/board.h

HAVE_NRF8001
  If the current BOARD has an optional nRF8001 Bluetooth Low Energy controller,
  this enables support for it.

