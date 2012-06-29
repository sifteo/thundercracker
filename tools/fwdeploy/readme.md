
fwdeploy: Prepare firmware images for deployment
=================================================

## Notes

`fwdeploy` accepts as its input a binary fw image (not an elf!). To generate a .bin from your .elf:

    arm-none-eabi-objcopy -O binary myfirmware.elf myfirmware.bin

fwdeploy works in tandem with:
* `swiss fwload` takes the output of fwdeploy as its input, and transmits it to the master's bootloader over USB
* firmware/master/bootloader decrypts the data sent by `swiss fwload` and installs it to the master's program flash
