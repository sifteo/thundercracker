#!/bin/bash
tc-siftulator -n 3 -T -LR & 
sleep 0.1s
master-sim sandwichcraft.elf #| grep PAINT --line-buffered
