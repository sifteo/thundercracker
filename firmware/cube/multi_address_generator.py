#
# Python script to execute the compilation
# of TC cube firmware utilizing different hard coded
# addresses.
#
# Sifteo 2012 
#
# Jared Wolff - Sifteo Hardware Engineering

import sys, os, platform
import shutil, datetime
import subprocess, multiprocessing

date = datetime.date.today().isoformat()
destination = "cube_firmware_%s" % date

print "\nCube Binary Generator\nCompiling firmare 0x00 to 0x02...\n"

try:
    os.stat(destination)
except:
    os.mkdir(destination)

cores = multiprocessing.cpu_count()

for chan in [0x02, 0x10, 0x20]:
    for addr in [0x00, 0x01, 0x02]:
        subprocess.check_call(["make", "clean"])

        myenv = dict(os.environ)
        hwrev = 2
        flags = "-DCUBE_ADDR=0x%x -DCUBE_CHAN=0x%x -DHWREV=%d" % (addr, chan, hwrev)
        myenv["CFLAGS"] = flags

        subprocess.check_call(["make", "-j%d" % (cores)], env=myenv)

        filename = "cube_chan_0x%x_addr_0x%x.hex" % (chan, addr)
        shutil.move("cube.hex", os.path.join(destination, filename))
