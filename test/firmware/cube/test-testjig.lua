--[[
    Sifteo Thundercracker firmware unit tests

    M. Elizabeth Scott <beth@sifteo.com> 
    Copyright <c> 2012 Sifteo, Inc. All rights reserved.
]]--

require('luaunit')
require('vram')
require('radio')

TestTestjig = {}

    function TestTestjig:test_vram_pokes()
        gx.cube:testSetEnabled(true)
        gx.cube:testWriteVRAM(0, 0x12)
        gx.cube:testWriteVRAM(1, 0x34)
        gx.cube:testWriteVRAM(100, 0x55)
        gx.cube:testWriteVRAM(101, 0xaa)
        gx.sys:vsleep(0.3)
        gx.cube:testSetEnabled(false)

        assertEquals(gx.cube:xwPeek(0), 0x3412)
        assertEquals(gx.cube:xwPeek(50), 0xaa55)
    end

    function TestTestjig:test_hwid()
        -- Capture an ACK packet
        gx.cube:testSetEnabled(true)
        gx.sys:vsleep(0.3)
        local ack = gx.cube:testGetACK()
        gx.cube:testSetEnabled(false)

        -- Read the HWID from NVM
        local hwid = ""
        for i = 0, 7 do
            hwid = hwid .. string.char(gx.cube:nbPeek(i))
        end

        assertEquals(string.sub(ack, -8), hwid)
    end
