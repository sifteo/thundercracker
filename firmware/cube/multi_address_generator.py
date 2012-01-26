#
# Python script to execute the compilation
# of TC cube firmware utilizing different hard coded
# addresses.
#
# Sifteo 2012 
#
# Jared Wolff - Sifteo Hardware Engineering

import sys
import os
import shutil
import datetime
import platform
import subprocess

date = datetime.date.today().isoformat()

print "\nCube Binary Generator\nCompiling firmare 0x00 to 0x02...\n"

for chan in ["0x00","0x10","0x20"]:
	
	for addr in ["0x00","0x01","0x02"]:
		subprocess.check_call(["make","clean"])
		myenv = dict(os.environ)
		myenv["CFLAGS"] = "-DCUBE_ADDR="+addr+" -DCUBE_CHAN="+chan
		subprocess.check_call(["make"], env=myenv)
		shutil.move("cube.ihx", "cube_"+date+"_chan_"+chan+"_addr_"+addr+".hex")
