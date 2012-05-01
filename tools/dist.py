#!/usr/bin/env python

import os, platform
import shutil, subprocess
from ZipDir import ZipDir

TC_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
DIST = os.path.join(TC_ROOT, "dist")

def sdkVersion():
    githash = subprocess.check_output(["git", "describe", "--tags"]).strip()
    return "sifteo-sdk-%s" % githash


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
    if os.name != 'posix':
        patterns.append('sifteo-sdk-shell.command')
    if os.name != 'nt':
        patterns.append('sifteo-sdk-shell.cmd')

    shutil.copytree("sdk", packagedir, True, ignore = shutil.ignore_patterns(*patterns))

    print "creating sdk archive '%s'....." % zipFilename
    ZipDir(packagedir, zipFilename)
    shutil.rmtree(packagedir, ignore_errors = True)

    print "done."
