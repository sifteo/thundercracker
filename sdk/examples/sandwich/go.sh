#!/bin/bash
#make
../../../emulator/tc-siftulator.app/Contents/MacOS/tc-siftulator -n 3 -T &
#../../../emulator/tc-siftulator.app/Contents/MacOS/tc-siftulator -n 3 -f ../../../../firmware/cube/cube.ihx &
sleep 1s
#./sandwichcraft.sim
../../../firmware/master/master-sim sandwichcraft.elf
