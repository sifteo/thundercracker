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

channel = [0x09, 0x0b, 0x0d, 0x0f, 0x11, 0x13, 0x15, 0x17, 0x19, 0x1b, 0x1d, 0x1f, 0x20, 0x23, 0x25, 0x27, 0x29, 0x2b, 0x31]
# channel = [ 0x1d ]

print "\n#### Master Binary Generator\n#### Compiling firmare for channels %s\n" % channel

# one time operations
date = datetime.date.today().isoformat()
githash = subprocess.check_output(["git", "describe", "--tags"]).strip()
subprocess.check_call(["make", "clean"])
DIR = os.path.dirname(__file__)

# ensure our destination exists
destination = githash
try:
    os.stat(destination)
except:
    os.mkdir(destination)

def removeRelPath(p):
    print "Removing", p
    try:
        os.remove(os.path.join(DIR, p))
    except:
        pass

for chan in channel:
    # remove files we need to recompile for each channel configuration
    removeRelPath("../master-stm32.elf")
    removeRelPath("../stm32/main.stm32.o")
    removeRelPath("../common/cube.stm32.o")

    # Set dem environment variables
    myenv = dict(os.environ)
    myenv["BOOTLOADABLE"] = "1"
    myenv["MASTER_RF_CHAN"] = "%d" % chan

    # Compile!
    cores = "-j%d" % multiprocessing.cpu_count()
    subprocess.call(["make", cores, "encrypted"], env=myenv)

    # Do a little renaming
    filename = "master_chan_0x%x_%s.sft" % (chan, githash)
    shutil.move("master-stm32.sft", os.path.join(destination, filename))
    print "#### Moving %s to %s\n" % (filename, destination)

#Prints out version at the end for any excel copy paste action
print "\nGit Version: %s" % githash
