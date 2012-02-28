Debug Cube Firmware In The Emulator
===================================

1. Compile the cube firmware (cd firmware/cube && make) or otherwise obtain a cube.ihx/cube.hex file.

2. Run the simulator with a firmware file specified (selects emulation rather than static binary translation) and with debug mode enabled:

 tc-siftulator.exe -f /path/to/cube.hex -d

3. You should see a text-mode debug UI. On Mac this will show up in the terminal you launched siftulator from, on Windows it'll be a separate window. Also, if you have multiple cubes, only cube #0 will be in debug mode. It will be identified by a red "Debugging" label in the UI.

4. You may want to press "+" once, to put the emulator in to the highest-speed mode. This disables some of the screen redraws in order to go a bit faster. Still expect the sim to run significantly slower than in binary-translated mode.

5. Use the "v" key to switch views, to the memory editor.

6. Tab over to the "XDATA" region. This entire 1kB region maps 1:1 with the VRAM layout documented in abi.h

7. You can browse and edit this region in real-time, or you can save a snapshot of it to disk with "s". Note that if you want to change memory and see the effects right away, you may need to manually put the cube into continuous-rendering mode. You can do this by writing an "8" into the very last byte of memory.
