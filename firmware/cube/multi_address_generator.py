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

print "\nCube Binary Generator\nCompiling firmare 0x00 to 0x08...\n"
	
for addr in ["0x00","0x01","0x02","0x03","0x04","0x05","0x06","0x07","0x08"]:
	subprocess.check_call(["make","clean"])
	myenv = dict(os.environ)
	myenv["CFLAGS"] = "-DCUBE_ADDR="+addr
	subprocess.check_call(["make"], env=myenv)
	shutil.move("cube.ihx", "cube_"+date+"_addr_"+addr+".hex")
