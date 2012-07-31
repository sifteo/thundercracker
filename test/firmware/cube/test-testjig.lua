--[[
    Sifteo Thundercracker firmware unit tests

    M. Elizabeth Scott <beth@sifteo.com> 
    Copyright <c> 2012 Sifteo, Inc. All rights reserved.
]]--

require('luaunit')
require('vram')
require('testjig')
require('flash-crc')

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
        assertEquals(jig:peekHWID(), jig:getHWID())
    end

    function TestTestjig:test_flash_reset()
        jig:flashReset()
    end

    function TestTestjig:test_neighbor_id()
        -- Make sure we can set the cube's neighbor ID over the testjig

        -- We start out with neighbors disabled
        assertEquals(gx.cube:getNeighborID(), 0)

        gx.cube:testWrite(packHex('fce5'))
        gx.sys:vsleep(0.1)
        assertEquals(gx.cube:getNeighborID(), 0xe5)

        gx.cube:testWrite(packHex('fce0'))
        gx.sys:vsleep(0.1)
        assertEquals(gx.cube:getNeighborID(), 0xe0)

        gx.cube:testWrite(packHex('fcff'))
        gx.sys:vsleep(0.1)
        assertEquals(gx.cube:getNeighborID(), 0xff)

        gx.cube:testWrite(packHex('fc00'))
        gx.sys:vsleep(0.1)
        assertEquals(gx.cube:getNeighborID(), 0x00)
    end

    function TestTestjig:test_flash_program()
        -- Queue up a bunch of flash loadstream data. This must be smaller
        -- than the FIFO buffer (63 bytes) plus it must be small enough to
        -- send over I2C during a single sensor polling interval.

        jig:flashReset()
        jig:programFlashAndWait(
            'e10000' ..     -- Address 0x0000
            '00abcd' ..     -- LUT1    [0] = 0xabcd
            '01fdff' ..     -- LUT1    [1] = 0xfdff
            '40' ..         -- TILE_P0 [0]
            '41'            -- TILE_P0 [1]
        )

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

        jig:flashReset()
        jig:programFlashAndWait(
            -- Write 0xFFFF at 0x0000 to force an auto-erase
            -- without actually programming any zero bits.

            'e10000' ..     -- Address 0x0000
            '00ffff' ..     -- LUT1    [0] = 0xffff
            '01f0f1' ..     -- LUT1    [1] = 0xf0f1
            '40' ..         -- TILE_P0 [0]

            -- Write a test pattern to the second tile

            'e10200' ..     -- Address 0x0002
            '41'            -- TILE_P0 [1]
        )

        -- Check memory contents

        for i = 0, 63 do
            assertEquals(gx.cube:fwPeek(i), 0xffff)
        end
        for i = 64, 127 do
            assertEquals(gx.cube:fwPeek(i), 0xf0f1)
        end

        -- Same thing, again.

        jig:programFlashAndWait(
            'e10200' ..     -- Address 0x0002
            '41'            -- TILE_P0 [1]
        )

        -- Check memory contents

        for i = 0, 63 do
            assertEquals(gx.cube:fwPeek(i), 0xffff)
        end
        for i = 64, 127 do
            assertEquals(gx.cube:fwPeek(i), 0xf0f1)
        end
    end

    function TestTestjig:test_flash_crc()

        -- Write some simple tiles, plus erase the first block
        jig:flashReset()
        jig:programFlashAndWait(
            'e10000' ..     -- Address 00:00
            '00abcd' ..     -- LUT1    [0] = 0xabcd
            '01fdff' ..     -- LUT1    [1] = 0xfdff
            '40' ..         -- TILE_P0 [0]
            '41'            -- TILE_P0 [1]
        )

        -- First, verify the CRC of some erased memory (Tiles 16-31)
        jig:programFlashAndWait(
            'e12000' ..     -- Address 00:20
            'e24201'        -- CRC, one block
        )
        assertEquals(unpackHex(radio:expectQuery(0x42)),
                     unpackHex(flashCRC(gx.cube, 0x20 * 0x40, 1)))

        -- Now go back and write some 1bpp tile data
        jig:programFlashAndWait(
            'e12000' ..     -- Address 00:20
            '60' ..         -- TILE_P1_R4 (count=1)
            '01234567' ..   -- 16 nybbles of pixel data
            '89abcdef' ..
            ('e0'):rep(10)  -- 10 pad bytes (12 past origin of quarter-tile)
        )

        -- Verify the same CRC again
        jig:programFlashAndWait(
            'e12000' ..     -- Address 00:20
            'e21901'        -- CRC, one block
        )
        assertEquals(unpackHex(radio:expectQuery(0x19)),
                     unpackHex(flashCRC(gx.cube, 0x20 * 0x40, 1)))

        -- Now verify a longer CRC
        jig:programFlashAndWait(
            'e10000' ..     -- Address 00:00
            'e21904'        -- CRC, four blocks
        )
        local expected = unpackHex(flashCRC(gx.cube, 0, 4))
        assertEquals(unpackHex(radio:expectQuery(0x19)), expected)

        -- Try the 'check query' command, to verify this without using the radio.
        -- Do it twice, to make sure it doesn't hang the FIFO nor corrupt the query buffer.
        -- Also make sure we can check just the header byte.
        jig:programFlashAndWait(
            'e31199' .. expected ..
            'e31199' .. expected ..
            'e30199'
        )

        -- Now a negative test. Make sure we hang the FIFO when the query buffer doesn't match.
        -- The 'check query' command itself should succeed... but any future commands will time
        -- out until the flash state machine is reset.

        -- Nop, just to verify the FIFO is still alive
        jig:programFlashAndWait('e0')

        -- This command succeeds, but it leaves the FIFO hung
        jig:programFlashAndWait('e31199' .. expected:sub(1,30) .. '00')

        -- Verify the FIFO is now hung
        assertError(jig.programFlashAndWait, jig, 'e0')
    end

    