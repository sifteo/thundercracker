set -e
make DEBUG=1 LLVM_LIB=~/src/llvm-3.0/Debug+Asserts/lib
llvm-as test.ll
echo ------------------------
./svmc -print-isel-input test.bc 
echo ------------------------
cat test.s
echo ------------------------
