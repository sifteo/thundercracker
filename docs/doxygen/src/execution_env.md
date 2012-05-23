Execution Environment     {#execution_env}
=====================

# Overview
Sifteo applications are deployed as ELF binaries, a [standard file format](http://en.wikipedia.org/wiki/Executable_and_Linkable_Format), and run in a specialized sandbox called @b SVM.

# Resource Usage Limits
Sifteo applications run on the Sifteo Base, a (very) small mobile device with limited resources.

## Read-Write Data (RAM)
Applications have a total of @b 32K RAM available, including static allocations and the stack. The system does not provide any built-in dynamic memory allocation facility.

### Stack
The size of the stack is equal to the total RAM available (32K) minus the total size of your application's static allocations.

In the event that you overflow the stack, the SVM runtime faults and execution of your application is stopped. Overflows are detected in the emulator, making it easy to detect these errors during development.

@note To get a sense of your stack usage, pass the `--svm-stack` option to `siftulator`. During execution it will log a message to the console each time it observes a new low water mark for the stack, indicating its total size.

## Read-Only Data (ROM)

The maximum size of a binary on the Sifteo platform is @b 16MB. Currently, this is also the maximum amount of external storage available on the Sifteo Base, so it's not practical to assume that a shipping application can consume all 16MB.

The .elf binary for your application contains all of the code, data, and assets required to run your game, so the total size of the .elf binary is the effective size of your application.

@note If your application is linked for Debug (the default), the .elf binary will also include debug information which does not get installed to hardware. To see the final size of your application, make sure you are linking a release build with "make clean && make RELEASE=1".