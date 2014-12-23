#!/usr/bin/env python
#
# SVM syscall table generator.
#
# Reads "abi.h" from stdin, produces on stdout a file with macros which
# name the symbols associated with each syscall function. This is used by
# the SVM target to generate calls to runtime functions that the compiler
# uses internally, such as our software floating point library.
#
# Micah Elizabeth Scott <micah@misc.name>
# Copyright <c> 2012 Sifteo, Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

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
