Factory Test Protocol for TC Cubes
==================================

To conserve code space, this protocol is super simple, and it builds upon
layers that are already present in the cube firmware.

At regular intervals of ~6ms, the cube will begin the polling sequence below.
It continues, uninterrupted, until the test jig returns a NACK or equivalently
until the test jig is disconnected. Note that any NACK will terminate the
entire sequence, and return to a normal sensor polling schedule.

1. Start condition, address byte 0xAA: Start a write to address 0x55

2. The full ACK packet (including HWID) is written, in the same format we send it over the radio.

3. Repeated start condition, address byte 0xAB: Start a read from address 0x55.

4. Read a packet. Defined formats:

    * [HH LL BB], with H < 4 -- Write 0xBB to VRAM address 0xHHLL
    * [fc BB] -- Set neighbor ID to 0xBB (Should be OR'ed with 0xE0 already)
    * [fd BB] -- Write 0xBB to Flash FIFO
    * [fe] -- Begin Flash FIFO reset sequence
    * [ff] -- No more packets. Stop polling.

5. Repeat step 4 until we get the no-more-packets packet.

Note that the normal accelerometer polling will preempt this process, so
there's really only time for ~30 packets until your 6ms window is up.

Sample log
==========

A cube firmware trace log, grepped for "I2C: BUS", while running the LuaUnit
test "TestTestjig".

Here you can see the ACK packet being read, followed by two VRAM write
packets. Then there's an 0xFF terminator, followed by a redundant terminator
(since the cube firmware needs at least one byte of advance notice to end the
packet)::

    [00 t=6421607] I2C: BUS start
    [00 t=6421607] I2C: BUS write(1) aa
    [00 t=6423273] I2C: BUS write(1) 02
    [00 t=6424778] I2C: BUS write(1) 00
    [00 t=6426282] I2C: BUS write(1) ff
    [00 t=6427786] I2C: BUS write(1) 00
    [00 t=6429290] I2C: BUS write(1) 00
    [00 t=6430795] I2C: BUS write(1) 00
    [00 t=6432301] I2C: BUS write(1) 00
    [00 t=6433805] I2C: BUS write(1) 00
    [00 t=6435311] I2C: BUS write(1) 00
    [00 t=6436817] I2C: BUS write(1) 60
    [00 t=6438323] I2C: BUS write(1) 87
    [00 t=6439818] I2C: BUS start
    [00 t=6439818] I2C: BUS write(1) ab
    [00 t=6442894] I2C: BUS read(1) 00
    [00 t=6444334] I2C: BUS read(1) 00
    [00 t=6445774] I2C: BUS read(1) 12
    [00 t=6447214] I2C: BUS read(1) 00
    [00 t=6448654] I2C: BUS read(1) 01
    [00 t=6450094] I2C: BUS read(1) 34
    [00 t=6451534] I2C: BUS read(1) ff
    [00 t=6452974] I2C: BUS read(0) ff
    [00 t=6452974] I2C: BUS stop


To Do
=====

Flash access! One problem here is that flash access currently must be
synchronous with the main graphics thread, but the above is all running
on the I2C state machine ISR. We could program flash via the decoder FIFO,
using commands like these:


But this still wouldn't provide any readback.

