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

neighbor_debug = 0
touch_debug = 0
disable_wdt = 1

hwrev = 5
num_cubes = 3
channel = [0x2]

date = datetime.date.today().isoformat()

chan_list = ""

# Generate string for channels

for idx,chan in enumerate(channel):
    
    if  (idx+1) == len(channel):
        chan_list += "0x%x" % chan
        
    else:
        chan_list += "0x%x, " % chan
        

print "\nCube Binary Generator\nCompiling firmare for channel %s ..." % chan_list

cores = multiprocessing.cpu_count()

# Retrive the Github Hash we've been using to determine versions

githash = subprocess.check_output(["git", "describe", "--tags"]).strip()

# Create the main folder this stuff will be stashed in

try:
    os.stat(githash)
except:
    os.mkdir(githash)

for chan in channel:

    # Changes the name of the files if they're debug versions
    if neighbor_debug or touch_debug:
        destination = os.path.join( githash , "DEBUG_rev_%d_0x%x_%s" % (hwrev,chan,githash))
    else:
        destination = os.path.join(githash, "rev_%d_chan_0x%x_%s" % (hwrev,chan,githash))
    
    # Checks to see if the destination folder exists. If not create it.
    try:
        os.stat(destination)
    except:
        os.mkdir(destination)
    
    # Now generate each cube firmware at each address needed.
    for addr in range(num_cubes):
        
        # Squeeky Clean!
        subprocess.check_call(["make", "clean"])
        
        myenv = dict(os.environ)
        
        # Sets the environment variables accordingly
        if neighbor_debug:
            flags = "-DCUBE_ADDR=0x%x -DCUBE_CHAN=0x%x -DHWREV=%d -DDEBUG_NBR=1" % (addr, chan, hwrev)
        elif touch_debug:
            flags = "-DCUBE_ADDR=0x%x -DCUBE_CHAN=0x%x -DHWREV=%d -DDEBUG_TOUCH=1" % (addr, chan, hwrev)
        elif disable_wdt:
            flags = "-DDISABLE_WDT=1 -DCUBE_ADDR=0x%x -DCUBE_CHAN=0x%x -DHWREV=%d" % (addr, chan, hwrev)
        else:
            flags = "-DCUBE_ADDR=0x%x -DCUBE_CHAN=0x%x -DHWREV=%d" % (addr, chan, hwrev)
        
        myenv["CFLAGS"] = flags
        
        subprocess.check_call(["make", "-j%d" % (cores), "cube.hex"], env=myenv)

        filename = "cube_chan_0x%x_addr_0x%x_%s.hex" % (chan, addr, githash)
        shutil.move("cube.hex", os.path.join(destination, filename))

print "\nGit Version: %s" % githash
