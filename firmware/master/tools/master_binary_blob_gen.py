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

########################################################
## Defines
########################################################
CLEAN = True

DIR = os.path.dirname(os.path.realpath(__file__))
TC_DIR = os.path.normpath(os.path.join(DIR, "..", "..", ".."))
FW_DIR = os.path.join(TC_DIR, "firmware/master/")
FWDEPLOY_DIR = os.path.join(TC_DIR, "tools/fwdeploy/")
MASTER_UNVERSIONED = "master-stm32.bin"
LAUNCHER_UNVERSIONED = "launcher.elf"
HW_VERSIONS = {2:"BOARD_TC_MASTER_REV2", 4:"BOARD_TC_MASTER_REV3"}

####################################
## Checks to see if directory exists.
## Deletes old one if it does exist
####################################
def check_and_mkdir(path):
    # ensure our destination exists
    try:
        print "#### Making directory: %s" % path
        os.mkdir(path)
    except:
        pass

def check_remove_and_mkdir(path):

    try:
        print "#### Emptying directory: %s" % path
        os.stat(path)
        shutil.rmtree(path)
    except:
        pass

    os.mkdir(path)

####################################
## Check to make sure we're in the SDK shell
####################################
def ensureRunningFromSDKShell():
    if  os.getenv("SDK_DIR") == None:
        error("SDK directory not defined!")

####################################
## Error dialog
####################################
def error(detail):
    print "########################################################"
    print "## ERROR: ", detail
    print "########################################################"
    sys.exit(1)

########################################################################
## Copies raw filename renamed to the versioned_filename at the target path
########################################################################
def copy_to_dir(raw_filename,versioned_filename,target_path):
    shutil.copy(raw_filename, os.path.join(target_path, versioned_filename))
    print "#### Moving %s to %s" % ( versioned_filename, target_path )

def change_dir(target_dir):
    print "#### Changing directories to %s" % (target_dir)
    os.chdir(target_dir)

def build_launcher():

    ensureRunningFromSDKShell()

    if CLEAN:
        subprocess.check_call(["make", "clean"])

    subprocess.check_call(["make","RELEASE=1"])

def build_firmware(version):
    #run make clean on master firmware directory
    if CLEAN:
      subprocess.check_call(["make", "clean"])

    # Set dem environment variables
    myenv = dict(os.environ)
    myenv["BOOTLOADABLE"] = "1"
    myenv["BOARD"] = HW_VERSIONS[version]

    # Compile!
    cores = "-j%d" % multiprocessing.cpu_count()
    subprocess.call(["make", cores], env=myenv)

def deploy_firmware_blob(target_filename,build_hash,githash):

    arguements = [];

    arguements.append(os.path.join(FWDEPLOY_DIR, "fwdeploy"))
    arguements.append(target_filename)
    arguements.append("--fw-version")
    arguements.append(githash)

    for key in build_hash:
        arguements.append("--fw")
        arguements.append(str(key))
        arguements.append(build_hash[key])

    # print "Arguements %s" % arguements

    subprocess.check_call(arguements)


##############################
## Our main execution function
##############################
def run(secondary_path, launcher_build_status):
    print "\n#### Master Binary Generator"

    #grab githash
    githash = subprocess.check_output(["git", "describe", "--tags"]).strip()

    #set up directory pointers.
    latest_dir = os.path.join(DIR, "..", "latest")
    launcher_dir = os.path.join(TC_DIR, "launcher")
    target_dir = os.path.join(DIR, "..", githash)
    fw_build_dir = FW_DIR

    #one time operations if secondary_path is defined
    if secondary_path != False:
      remote_target_dir = os.path.join(secondary_path,githash)
      remote_latest_dir = os.path.join(secondary_path,"latest")
    else:
      remote_target_dir = None
      remote_latest_dir = None

    #launcher filename
    launcher_filename = "launcher_%s.elf" % (githash)

    #set the target filename
    target_filename = "master_%s.usft" % (githash)

    # one time operations
    date = datetime.date.today().isoformat()

    # Changing directory to firmware dir
    change_dir(fw_build_dir)

    # Make sure our target directories are generated!
    check_and_mkdir(target_dir)
    check_remove_and_mkdir(latest_dir)

    ############################
    #### Process base firmware
    ############################

    version_hash = {}

    # iterate over all the versions and build them
    for hw_version in HW_VERSIONS:
        #master firmware filename

        print "#### Generating Firmware for Version %d" % hw_version
        version_hash[hw_version] = "master_%s_hw_ver_%d.bin" % (githash,hw_version)
        build_firmware(hw_version)

        # Rename and keep in master directory
        copy_to_dir(MASTER_UNVERSIONED,version_hash[hw_version],target_dir)

    change_dir(target_dir)

    # lets make a new blob
    deploy_firmware_blob(target_filename,version_hash,githash)

    # copy that blob to the latest dir
    copy_to_dir(target_filename,target_filename,latest_dir)

    if secondary_path != False:

        check_remove_and_mkdir(remote_latest_dir)
        check_and_mkdir(remote_target_dir)

        copy_to_dir(target_filename,target_filename, remote_latest_dir)
        copy_to_dir(target_filename,target_filename, remote_target_dir)

    ############################
    #### Process launcher
    ############################

    if launcher_build_status:

        change_dir(launcher_dir)

        build_launcher()

        copy_to_dir(LAUNCHER_UNVERSIONED,launcher_filename,latest_dir)
        copy_to_dir(LAUNCHER_UNVERSIONED,launcher_filename,target_dir)

        if secondary_path != False:
            copy_to_dir(LAUNCHER_UNVERSIONED,launcher_filename,remote_latest_dir)
            copy_to_dir(LAUNCHER_UNVERSIONED,launcher_filename,remote_target_dir)

    else:
        print "#### Launcher not built. Use --with-launcher to build launcher."

    #Prints out version at the end for any excel copy paste action
    print "#### Git Version: %s" % githash

if __name__ == '__main__':

    if len(sys.argv) > 5 or len(sys.argv) < 2:
        print >> sys.stderr, "usage: python master_binary_blob_gen.py --secondary-path <secondary path> --with-launcher"
        sys.exit(1)

    secondary_path = False
    with_launcher = False

    for idx,arg in enumerate(sys.argv):
        if arg == "--with-launcher":
            with_launcher = True
        elif arg == "--secondary-path":
            secondary_path = sys.argv[idx+1]

    run(secondary_path, with_launcher)
