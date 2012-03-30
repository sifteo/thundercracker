#!/bin/sh
# Sample shell script to build with debug enabled.

exec make DEBUG=1 LLVM_LIB=~/src/llvm-3.0/Debug+Asserts/lib -j 4
