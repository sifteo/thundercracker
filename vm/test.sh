set -e
OPTS="-print-isel-input -view-isel-dags -show-mc-encoding"

make DEBUG=1 LLVM_LIB=~/src/llvm-3.0/Debug+Asserts/lib -j 4
clang -emit-llvm -m32 -ffreestanding -O2 -c test.c

echo ------------------------

rm -f test.o.s
./svmc -filetype=asm $OPTS test.o $* 2>&1 | tee test.out

DOTFILE=`grep \.dot test.out | tail -n 1 | cut -d "'" -f 2`
dot -Tpng $DOTFILE > test.png
#open test.png&

echo ------------------------

cat test.o.s

echo ------------------------

./svmc test.o $*
arm-none-eabi-objdump -d test.elf