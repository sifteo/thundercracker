#!/usr/bin/env python
#
# Syscall table generator.
# Reads "abi.h" from stdin, produces a function pointer table on stdout.
#
# M. Elizabeth Scott <beth@sifteo.com>
# Copyright <c> 2012 Sifteo, Inc. All rights reserved.
#

import sys, re

regex = re.compile(r"^.*(_SYS_\w+)\s*\(.*\)\s+_SC\((\d+)\)");
fallback = re.compile(r"_SC\((\d+)\)");
highestNum = 0
callMap = {}
typedef = "(SvmSyscall)"

for line in sys.stdin:
    m = regex.match(line)
    if m:
        name, num = m.group(1), int(m.group(2))
        highestNum = max(highestNum, num)
        if num in callMap:
            raise Exception("Duplicate syscall #%d" % num)
        callMap[num] = name
    
    elif fallback.search(line):
        raise Exception("Regex might have missed a syscall on line: %r" % line);

for i in range(highestNum+1):
    print "    /* %4d */ %s%s," % (i, typedef, callMap.get(i) or "0")
