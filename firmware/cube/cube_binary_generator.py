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
disable_wdt = 0
disable_sleep = 0

hwrev = 6

date = datetime.date.today().isoformat()

print "\nCube Binary Generator\n"

cores = multiprocessing.cpu_count()

# Retrive the Github Hash we've been using to determine versions

githash = subprocess.check_output(["git", "describe", "--tags"]).strip()

# Create the main folder this stuff will be stashed in

try:
    os.stat(githash)
except:
    os.mkdir(githash)

destination = githash

# Checks to see if the destination folder exists. If not create it.
try:
    os.stat(destination)
except:
    os.mkdir(destination)

# Squeeky Clean!
subprocess.check_call(["make", "clean"])

myenv = dict(os.environ)

flags = "-DHWREV=%d" % (hwrev)
# Sets the environment variables accordingly
if neighbor_debug:
    flags += " -DDEBUG_NBR=1"
if touch_debug:
    flags += " -DDEBUG_TOUCH=1"
if disable_wdt:
    flags += " -DDISABLE_WDT=1"
if disable_sleep:
    flags += " -DDISABLE_SLEEP=1"
    
myenv["CFLAGS"] = flags

subprocess.check_call(["make", "-j%d" % (cores), "cube.hex"], env=myenv)

filename = "cube_%s.hex" % (githash)
shutil.move("cube.hex", os.path.join(destination, filename))

print "\nGit Version: %s" % githash
