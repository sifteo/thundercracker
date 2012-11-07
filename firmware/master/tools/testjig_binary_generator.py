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


def run(secondary_path): 
    print "\n#### Testjig Binary Generator\n"

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

    try:
        os.stat("latest")
        print "#### Removing latest symlink."
        os.remove("latest")
    except:
        pass

    def removeRelPath(p):
        print "Removing", p
        try:
            os.remove(os.path.join(DIR, p))
        except:
            pass

    # Set dem environment variables
    myenv = dict(os.environ)

    # Compile!
    cores = "-j%d" % multiprocessing.cpu_count()
    subprocess.call(["make", cores], env=myenv)

    # Do a little renaming
    filename = "testjig_%s.hex" % (githash)
    shutil.copy("testjig.hex", os.path.join(destination, filename))
    print "#### Moving %s to %s" % (filename, destination)
    os.symlink( destination, "latest" )
    print "#### Making symlink to %s" % ( "latest" )

    #Prints out version at the end for any excel copy paste action
    print "#### Git Version: %s" % githash
    
if __name__ == '__main__':

    if len(sys.argv) > 2:
        print >> sys.stderr, "usage: python master_binary_generator.py <secondary_path>"
        sys.exit(1)

    if len(sys.argv) == 2 :
        secondary_path = sys.argv[1]
    else:
        secondary_path = False

    run(secondary_path)