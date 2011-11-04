--[[
    Sifteo Thundercracker firmware unit tests

    M. Elizabeth Scott <beth@sifteo.com> 
    Copyright <c> 2011 Sifteo, Inc. All rights reserved.
]]--

require('scripts/luaunit')
require('scripts/vram')
require('scripts/radio')

TestRadio = {}

    function TestRadio:test_literal()
        radio:tx("0c00")
        assertEquals(gx.cube:xwPeek(0), 0x0000)
    end
