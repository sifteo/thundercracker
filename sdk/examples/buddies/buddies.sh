#!/bin/bash
../../../firmware/master/master-sim buddies.elf | sed '/stamp=0x/d;/Flashlayer/d'
