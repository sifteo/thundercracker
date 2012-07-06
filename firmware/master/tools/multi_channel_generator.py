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
import time


channel = [0x09, 0x0b, 0x0d, 0x0f, 0x11, 0x13, 0x15, 0x17, 0x19, 0x1b, 0x1d, 0x1f, 0x20, 0x23, 0x25, 0x27, 0x29, 0x2b, 0x31]

# channel = [ 0x1d ]

date = datetime.date.today().isoformat()

chan_list = ""

for idx,chan in enumerate(channel):
    
    if  (idx+1) == len(channel):
        chan_list += "0x%x" % chan
        
    else:
        chan_list += "0x%x, " % chan
        

print "\n####Master Binary Generator\n####Compiling firmare for channel %s ...\n" % chan_list

githash = subprocess.check_output(["git", "describe", "--tags"]).strip()

subprocess.check_call(["make", "clean"])

# Grab the current dir

DIR = os.path.dirname(__file__)

CUBE_PATH = os.path.join(DIR, "../common/cube.cpp")

for chan in channel:
    
    destination = githash
    
    # Checks if our destination exists
    
    try:
        os.stat(destination)
    except:
        os.mkdir(destination)

    # Touches cube.cpp every time to ensure we're recompiling with a new channel
    print "\n####Touching: %s" % CUBE_PATH 
    subprocess.call(["touch", "-afm", CUBE_PATH ])
    subprocess.call(["ls", '-l', CUBE_PATH ])
    
    myenv = dict(os.environ)
    
    # Set dem environment variables
    myenv["BOOTLOADABLE"] = "1"
    myenv["MASTER_RF_CHAN"] = "%d" % chan
    
    # Compile!
    subprocess.call(["make", "encrypted"], env=myenv)

    # Do a little renaming
    filename = "master_chan_0x%x_%s.sft" % (chan, githash)
    shutil.move("master-stm32.sft", os.path.join(destination, filename))
    print "#### Moving %s to %s " % (filename, destination)
    
    #Added a delay. Here because of fast compilation.
    time.sleep(1)

#Prints out version at the end for any excel copy paste action
print "\nGit Version: %s" % githash
