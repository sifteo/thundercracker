#!/bin/bash
../../../emulator/tc-siftulator.app/Contents/MacOS/tc-siftulator -n 3 -T &
#../../../emulator/tc-siftulator.app/Contents/MacOS/tc-siftulator -n 3 -f ../../../../firmware/cube/cube.ihx &
sleep 0.1s
../../../firmware/master/master-sim sandwichcraft.elf | grep -v "stamp=0x"
