Factory Test Protocol for TC Cubes
==================================

To conserve code space, this protocol is super simple, and it builds upon layers that are already present in the cube firmware.

At regular intervals of ~6ms, the cube will begin the polling sequence below.
It continues, uninterrupted, until the test jig returns a NACK or equivalently  until the test jig is disconnected. Note that any NACK will terminate the entire sequence, and return to a normal sensor polling schedule.

1. Start condition, address byte 0xAA: Start a write to address 0x55

2. The entire ACK buffer is written, in the same format we send it over the radio.

3. Repeated start condition, address byte 0xAB: Start a read from address 0x55.

4. Read a three-byte packet. Defined formats:

    * [LL HH BB], with H < 4 -- Write 0xBB to VRAM address 0xHHLL

5. Repeat step 4 until NACK.

To Do
=====

Flash access! One problem here is that flash access currently must be
synchronous with the main graphics thread, but the above is all running
on the I2C state machine ISR. We could program flash via the decoder FIFO,
using commands like these:

    * [HH ff BB] -- Write 0xBB to Flash FIFO at offset (HH - 1) % size, then write HH to FIFO head pointer.
    * [ff ff 00] -- Special case of above; begin Flash FIFO reset sequence

But this still wouldn't provide any readback.

