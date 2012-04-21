#!/usr/bin/env python

import os, platform
import shutil, subprocess

TC_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
DIST = os.path.join(TC_ROOT, "dist")

def sdkVersion():
    githash = subprocess.check_output(["git", "describe", "--tags"]).strip()
    return "sifteo-sdk-%s" % githash


if __name__ == '__main__':

    print "copying files into place....."
    os.chdir(TC_ROOT)
    packagedir = sdkVersion()
    shutil.rmtree(packagedir, ignore_errors = True)

    # don'y copy sifteo games - this won't be needed once they're in their own repo
    shutil.copytree("sdk", packagedir, ignore = shutil.ignore_patterns(
        '.git', '.gitignore', 'buddies', 'chroma', 'ninja_slide', 'peano', 'sandwich', 'word')
    )

    print "creating sdk archive....."
    shutil.make_archive(packagedir, "zip", packagedir)
    shutil.rmtree(packagedir, ignore_errors = True)

    print "done."
