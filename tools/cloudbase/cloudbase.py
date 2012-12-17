#!/usr/bin/env python

# to install elftools: `pip install pyelftools`

import sys, elftools

from elftools.elf.elffile import ELFFile

def installableELFSize(filepath):
    with open(filepath, 'rb') as f:
        elffile = ELFFile(f)
        sz = 0

        for segment in elffile.iter_segments():
            sz = max(sz, segment['p_offset'] + segment['p_filesz'])
        return sz


if __name__ == '__main__':

    path = sys.argv[1]
    print("installable size:", installableELFSize(path))
