set -e

CLANG=../deps/osx/x86_64/llvm-3.0/bin/clang
DIS=../deps/osx/x86_64/llvm-3.0/bin/llvm-dis

make DEBUG=1 LLVM_LIB=~/src/llvm-3.0/Debug+Asserts/lib -j 4

$CLANG -emit-llvm -m32 -fno-stack-protector -ffreestanding \
    -fno-exceptions -fno-threadsafe-statics -fno-rtti -O2 -c \
    -I ../sdk/include test.cpp
    
$DIS test.o

echo ------------------------

rm -f test.s
./svmc -filetype=asm -show-mc-encoding test.o $* 2>&1 | tee test.out
DOTFILE=`grep \.dot test.out | tail -n 1 | cut -d "'" -f 2`
dot -Tpng $DOTFILE > test.png < /dev/null
open test.png&

echo ------------------------

cat test.s

echo ------------------------

./svmc test.o $*
arm-none-eabi-readelf -a test.elf
arm-none-eabi-objdump -D -M force-thumb test.elf