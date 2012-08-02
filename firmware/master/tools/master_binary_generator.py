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

print "\n#### Master Binary Generator\n"

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

# Set dem environment variables
myenv = dict(os.environ)
myenv["BOOTLOADABLE"] = "1"

# Compile!
cores = "-j%d" % multiprocessing.cpu_count()
subprocess.call(["make", cores, "encrypted"], env=myenv)

# Do a little renaming
filename = "master_%s.sft" % (githash)
shutil.move("master-stm32.sft", os.path.join(destination, filename))
print "#### Moving %s to %s\n" % (filename, destination)

#Prints out version at the end for any excel copy paste action
print "\nGit Version: %s" % githash
