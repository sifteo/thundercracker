Experimental simulator for an 8051-based cube ("microcube")
===========================================================

This is a hardware simulator that aims to do a cycle-accurate
simulation of the subset of hardware that we're using. If everything
is working as intended, the frame rate and other stats given by the
simulator should exactly match real hardware.

What does it simulate?
----------------------

* An 8051 CPU core (nRF24LE1 cycle timings)
* A shared 8-bit parallel bus
* Two 8-bit address latches
* A NOR Flash memory, with 21-bit address and 8-bit data
* An LCD controller (SPFD5414)

Prerequisites
-------------

- make
- gcc
- libSDL, for the LCD output
- ncurses, for the debug console
- sdcc, for compiling the firmware
- Optional, only needed if you want to regenerate the asset image:
  Python, and PIL (Python Imaging Library)

Tested on Linux and Mac OS X.

On Mac OS, I used MacPorts to install the dependencies:
  sudo port install py27-pil sdcc ncurses libsdl

Usage
-----

After "make" finishes, you should have a "cube51-sim" binary, some
.ihx firmware images in the "firmware" directory, and a populated
"flash.bin" asset image.

Run::

  ./cube51-sim firmware/demo.ihx

This will load the compiled firmware into the simulator, and start it
running. The 'h' key will open help for the console, so you can see
the other supported keys.

Note that the firmware starts running immediately, since this is typically
what you'll want. If you want to inspect something or add a breakpoint
before starting execution, you can stop the simulation then reset it.

Assumptions
-----------

1. It is cheaper to use the built-in Nordic CPU than a separate CPU.

2. It is cheaper to use a small 8-bit CPU with little RAM, than a
   32-bit ARM with enough RAM for a framebuffer.

3. We can write compelling games without rendering any vector graphics
   at runtime.

4. We can write compelling games using tile and sprite support only,
   in a graphics engine that is similar to an NES, Super Nintendo, or
   Game Boy Advance.

5. Our LCD controller natively supports 16-bit pixel data

6. Our LCD controller does *not* natively support 8-bit pixel data.
   (If it did, we could use smaller asset data in NOR flash.)

7. NOR flash in the 1-16 MBit range is cheap enough not to raise the
   overall cost of this design beyond that of a larger CPU plus a
   different memory technology.

8. Parallel NOR flash is power-efficient enough to use in this application

9. We can write compelling games using no more than 16 MBits of
   *uncompressed* tile/sprite/image data per cube.

probably others too.
