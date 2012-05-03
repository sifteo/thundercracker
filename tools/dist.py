#!/usr/bin/env python

import os, platform
import shutil, subprocess
from ZipDir import ZipDir

TC_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
DIST = os.path.join(TC_ROOT, "dist")


def platformBits():
    if os.name != "posix":
        return ""
    machine = os.uname()[4]
    if machine == "x86_64":
        return "64"
    elif machine in ('i386', 'i686'):
        return "32"
    else:
        raise OSError("Unrecognized machine in uname output")

def platformOS():
    if os.name == "nt":
        return "windows"

    if os.name == "posix":
        sysname = os.uname()[0]
        if sysname == "Linux":
            return "linux"
        if sysname == "Darwin":
            return "mac"

    raise OSError("Unrecognized OS type")

def sdkVersion():
    githash = subprocess.check_output(["git", "describe", "--tags"]).strip()
    return "sifteo-sdk-%s%s-%s" % (platformOS(), platformBits(), githash)


if __name__ == '__main__':

    print "copying files into place....."
    os.chdir(TC_ROOT)
    packagedir = sdkVersion()
    zipFilename = "%s.zip" % packagedir
    shutil.rmtree(packagedir, ignore_errors = True)

    # Basic ignoreables...
    patterns = [
        '.git', '.gitignore', '.DS_Store',
        '*.o', '*.d', '*.bak', '*.gen.h', '*.gen.cpp',
    ]

    # Platform-specific
    if platformOS() != "mac":
        patterns.append("sifteo-sdk-shell.command")
    if platformOS() != "windows":
        patterns.append("sifteo-sdk-shell.cmd")
    if platformOS() != "linux":
        patterns.append("sifteo-sdk-shell.sh")

    shutil.copytree("sdk", packagedir, True, ignore = shutil.ignore_patterns(*patterns))

    print "creating sdk archive '%s'....." % zipFilename
    ZipDir(packagedir, zipFilename)
    shutil.rmtree(packagedir, ignore_errors = True)

    print "done."
