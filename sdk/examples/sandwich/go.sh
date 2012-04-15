#!/bin/bash
../../../emulator/tc-siftulator.app/Contents/MacOS/tc-siftulator -n 6 -T -LR &
sleep 0.1s
../../../firmware/master/master-sim sandwichcraft.elf
