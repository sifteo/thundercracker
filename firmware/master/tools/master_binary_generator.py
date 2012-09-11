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

    try:
        os.stat("latest")
        print "#### Removing latest dir."
        shutil.rmtree("latest")
        os.mkdir("latest")
    except:
        os.mkdir("latest")

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
    shutil.copy("master-stm32.sft", os.path.join(destination, filename))
    print "#### Moving %s to %s" % (filename, destination)
    shutil.copy("master-stm32.sft", os.path.join("latest", filename))
    print "#### Moving %s to %s" % ( destination,"latest" )

    filename = "master_%s.elf" % (githash)
    shutil.copy("master-stm32.elf", os.path.join(destination, filename))
    print "#### Moving %s to %s" % (filename, destination)
    shutil.copy("master-stm32.elf", os.path.join("latest", filename))
    print "#### Moving %s to %s" % ( destination,"latest" )
    
    if secondary_path != False:
        remote_path = os.path.join(secondary_path,githash)
        latest_remote_path = os.path.join(secondary_path,"latest")

        try:
            os.stat(os.path.join(secondary_path,"latest"))
            print "#### Removing remote latest dir."
            shutil.rmtree(os.path.join(secondary_path,"latest"))
            os.mkdir(os.path.join(secondary_path,"latest"))
        except:
            os.mkdir(os.path.join(secondary_path,"latest"))

        try:
            os.stat(remote_path)
        except:
            os.mkdir(remote_path)
        shutil.copy("master-stm32.sft", os.path.join(remote_path, filename))
        print "#### Moving %s to %s" % (filename, remote_path)

        shutil.copy("master-stm32.sft", os.path.join(latest_remote_path, filename))
        print "#### Moving %s to %s" % (filename, latest_remote_path)

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