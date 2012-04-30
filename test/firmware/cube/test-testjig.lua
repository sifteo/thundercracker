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
        gx.sys:vsleep(0.3)
        gx.cube:testSetEnabled(false)

        assertEquals(gx.cube:xwPeek(0), 0x3412)
    end
