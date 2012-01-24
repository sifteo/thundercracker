set -e
make DEBUG=1 LLVM_LIB=~/src/llvm-3.0/Debug+Asserts/lib
llvm-as test.ll
./svmc -print-isel-input test.bc 
