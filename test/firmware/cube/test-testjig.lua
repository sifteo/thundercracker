--[[
    Sifteo Thundercracker firmware unit tests

    M. Elizabeth Scott <beth@sifteo.com> 
    Copyright <c> 2012 Sifteo, Inc. All rights reserved.
]]--

require('luaunit')
require('vram')
require('radio')

TestTestjig = {}

    function TestTestjig:setUp()
        gx.cube:testSetEnabled(true)

        -- Give us time to capture an initial ACK packet.
        gx.sys:vsleep(0.3)
    end

    function TestTestjig:tearDown()
        gx.cube:testSetEnabled(false)
    end

    function TestTestjig:test_vram_pokes()
        gx.cube:testWrite(packHex(
            '000012' ..     -- Write at 0x0000
            '000134' ..     -- Write at 0x0001
            'f86455' ..     -- Write at 0xf864 (wrap to 0x0064)
            'f865aa'        -- Write at 0xf865 (wrap to 0x0065)
        ))
        gx.sys:vsleep(0.3)
        assertEquals(gx.cube:xwPeek(0), 0x3412)
        assertEquals(gx.cube:xwPeek(50), 0xaa55)
    end

    function TestTestjig:test_hwid()
        -- Read the HWID from NVM
        local hwid = ""
        for i = 0, 7 do
            hwid = hwid .. string.char(gx.cube:nbPeek(i))
        end

        local ack = gx.cube:testGetACK()
        assertEquals(string.sub(ack, -8), hwid)
    end

    function TestTestjig:test_flash_reset()
        -- Capture a baseline ACK packet
        local fifoAck1 = string.byte(string.sub(gx.cube:testGetACK(), 9, 9))

        -- Start a flash reset, and give that time to finish
        gx.cube:testWrite(packHex('fe'))
        gx.sys:vsleep(0.3)

        -- Make sure the reset was acknowledged
        local fifoAck2 = string.byte(string.sub(gx.cube:testGetACK(), 9, 9))
        assertEquals(fifoAck2, bit.band(fifoAck1 + 1, 0xFF))
    end

    function TestTestjig:test_flash_program()
        -- Start a flash reset, and give that time to finish
        gx.cube:testWrite(packHex('fe'))
        gx.sys:vsleep(0.3)
        local fifoAck1 = string.byte(string.sub(gx.cube:testGetACK(), 9, 9))

        -- Queue up a bunch of flash loadstream data. This must be smaller
        -- than the FIFO buffer (63 bytes) plus it must be small enough to
        -- send over I2C during a single sensor polling interval.
        
        gx.cube:testWrite(packHex(
            'fde1fd00fd00' ..       -- Address 0x0000
            'fd00fdabfdcd' ..       -- LUT1    [0] = 0xabcd
            'fd01fdfdfdff' ..       -- LUT1    [1] = 0xfdff
            'fd40' ..               -- TILE_P0 [0]
            'fd41'                  -- TILE_P0 [1]
        ))

        gx.sys:vsleep(0.5)

        -- Check for the expected number of byte ACKs
        local fifoAck2 = string.byte(string.sub(gx.cube:testGetACK(), 9, 9))
        assertEquals(fifoAck2, bit.band(fifoAck1 + 11, 0xFF))

        -- Check contents of flash memory
        for i = 0, 63 do
            assertEquals(gx.cube:fwPeek(i), 0xabcd)
        end
        for i = 64, 127 do
            assertEquals(gx.cube:fwPeek(i), 0xfdff)
        end
    end

    function TestTestjig:test_flash_verify()
        -- Rewrite the same tiles multiple times, ensure that the cube
        -- doesn't get stuck.

        local fifoAck1 = string.byte(string.sub(gx.cube:testGetACK(), 9, 9))
        
        -- Flash reset
        gx.cube:testWrite(packHex('fe'))
        gx.sys:vsleep(0.3)

        gx.cube:testWrite(packHex(
            -- Write 0xFFFF at 0x0000 to force an auto-erase
            -- without actually programming any zero bits.

            'fde1fd00fd00' ..       -- Address 0x0000
            'fd00fdfffdff' ..       -- LUT1    [0] = 0xffff
            'fd01fdf0fdf1' ..       -- LUT1    [1] = 0xf0f1
            'fd40' ..               -- TILE_P0 [0]

            -- Write a test pattern to the second tile

            'fde1fd02fd00' ..       -- Address 0x0002
            'fd41'                  -- TILE_P0 [1]
        ))

        -- Check memory contents
        gx.sys:vsleep(0.5)
        for i = 0, 63 do
            assertEquals(gx.cube:fwPeek(i), 0xffff)
        end
        for i = 64, 127 do
            assertEquals(gx.cube:fwPeek(i), 0xf0f1)
        end

        gx.cube:testWrite(packHex(
            -- Same thing, again.
            'fde1fd02fd00' ..       -- Address 0x0002
            'fd41'                  -- TILE_P0 [0]
        ))

        -- Check memory contents
        gx.sys:vsleep(0.5)
        for i = 0, 63 do
            assertEquals(gx.cube:fwPeek(i), 0xffff)
        end
        for i = 64, 127 do
            assertEquals(gx.cube:fwPeek(i), 0xf0f1)
        end

        -- Check for the expected number of byte ACKs
        local fifoAck2 = string.byte(string.sub(gx.cube:testGetACK(), 9, 9))
        assertEquals(fifoAck2, bit.band(fifoAck1 + 19, 0xFF))
    end

