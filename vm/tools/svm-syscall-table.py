#!/usr/bin/env python
#
# SVM syscall table generator.
#
# Reads "abi.h" from stdin, produces on stdout a file with macros which
# name the symbols associated with each syscall function. This is used by
# the SVM target to generate calls to runtime functions that the compiler
# uses internally, such as our software floating point library.
#
# M. Elizabeth Scott <beth@sifteo.com>
# Copyright <c> 2012 Sifteo, Inc. All rights reserved.
#

import sys, re

regex = re.compile(r"^.*_SYS_(\w+)\s*\(.*\)\s+_SC\((\d+)\)");
fallback = re.compile(r"_SC\((\d+)\)");
callMap = {}

for line in sys.stdin:
    m = regex.match(line)
    if m:
        name, num = m.group(1), int(m.group(2))
        if num in callMap:
            raise Exception("Duplicate syscall #%d" % num)
        callMap[num] = name
    
    elif fallback.search(line):
        raise Exception("Regex might have missed a syscall on line: %r" % line);

callList = callMap.items()
callList.sort()

for num, name in callList:
    print '#define SVMRT_%-25s "_SYS_%d"' % (name, num)
