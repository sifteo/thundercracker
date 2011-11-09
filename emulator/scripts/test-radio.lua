--[[
    Sifteo Thundercracker firmware unit tests

    M. Elizabeth Scott <beth@sifteo.com> 
    Copyright <c> 2011 Sifteo, Inc. All rights reserved.
]]--

require('scripts/luaunit')
require('scripts/vram')
require('scripts/radio')

TestRadio = {}

    function TestRadio:test_literal_14()
        -- Tests 14-bit literal codes
        
        radio:txn("c000")
        assertEquals(gx.cube:xwPeek(0), 0x0000)

        radio:txn("ffff")
        assertEquals(gx.cube:xwPeek(0), 0xfefe)

        radio:txn("c100")
        assertEquals(gx.cube:xwPeek(0), 0x0002)

        radio:txn("d000")
        assertEquals(gx.cube:xwPeek(0), 0x4000)

        radio:txn("c001")
        assertEquals(gx.cube:xwPeek(0), 0x0400)

        radio:txn("c010")
        assertEquals(gx.cube:xwPeek(0), 0x0020)

        radio:txn("efcd")
        assertEquals(gx.cube:xwPeek(0), gx:tileIndex(0x2dcf))

        -- Sequential addressing
        radio:txn("c000ffffc100d000c001c010efcd")
        assertEquals(gx.cube:xwPeek(0), 0x0000)
        assertEquals(gx.cube:xwPeek(1), 0xfefe)
        assertEquals(gx.cube:xwPeek(2), 0x0002)
        assertEquals(gx.cube:xwPeek(3), 0x4000)
        assertEquals(gx.cube:xwPeek(4), 0x0400)
        assertEquals(gx.cube:xwPeek(5), 0x0020)
        
        -- Explicit addressing
        radio:txn("31ffc001")
        assertEquals(gx.cube:xwPeek(0x1ff), 0x0400)
        radio:txn("31ffc010")
        assertEquals(gx.cube:xwPeek(0x1ff), 0x0020)
 
        -- Address wrap
        radio:txn("31ffc001c010")
        assertEquals(gx.cube:xwPeek(0x1ff), 0x0400)
        assertEquals(gx.cube:xwPeek(0), 0x0020)
    end
