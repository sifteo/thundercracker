--[[
    Sifteo Thundercracker firmware unit tests

    Micah Elizabeth Scott <micah@misc.name> 
    Copyright <c> 2011 Sifteo, Inc.
   
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.
]]--

require('luaunit')
require('vram')
require('radio')

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

    function TestRadio:test_hop()
        -- Test the "radio hop" command, for switching address/channel/ID

        -- Set a known address and channel
        radio:tx("7af4123456789a")
        assertEquals(gx.cube:getRadioAddress(), "74/123456789a")

        -- Empty hop (nop hop)
        radio:tx("7a")
        assertEquals(gx.cube:getRadioAddress(), "74/123456789a")

        -- Change channel only
        radio:tx("7a42")
        assertEquals(gx.cube:getRadioAddress(), "42/123456789a")

        -- Incomplete address (change channel only)
        radio:tx("7a43aa")
        assertEquals(gx.cube:getRadioAddress(), "43/123456789a")
        radio:tx("7a44aabb")
        assertEquals(gx.cube:getRadioAddress(), "44/123456789a")
        radio:tx("7a45aabbcc")
        assertEquals(gx.cube:getRadioAddress(), "45/123456789a")
        radio:tx("7a46aabbccdd")
        assertEquals(gx.cube:getRadioAddress(), "46/123456789a")

        -- Change channel and address
        radio:tx("7a47aabbccddee")
        assertEquals(gx.cube:getRadioAddress(), "47/aabbccddee")

        -- This changes channel, address, and neighbor ID, but we have
        -- no way to read the neighbor ID from Lua.
        radio:tx("7a48abcdface5599")
        assertEquals(gx.cube:getRadioAddress(), "48/abcdface55")

        -- And finally, an overly long packet. This is room for future expansion,
        -- additional bytes are currently ignored.
        radio:tx("7a494567face5500aabbccddeeff")
        assertEquals(gx.cube:getRadioAddress(), "49/4567face55")
    end

    function TestRadio:test_nap()
        -- Test the "radio nap" command, which puts the radio to sleep for
        -- a specified duration (in 16-bit CLKLF units)

        assertEquals(radio:isListening(), true)

        radio:tx("7b0000")
        assertEquals(math.abs(radio:expectWake() - 2.0) < 0.2, true)

        radio:tx("7b0080")
        assertEquals(math.abs(radio:expectWake() - 1.0) < 0.2, true)

        radio:tx("7b0040")
        assertEquals(math.abs(radio:expectWake() - 0.5) < 0.2, true)

        radio:tx("7b0500")
        assertEquals(radio:expectWake() < 0.2, true)
    end

    function TestRadio:test_idle_addr()
        -- Our protocol defines an algorithm for generating the default
        -- radio address and channel from the programmed HWID. Here we
        -- forcibly set several different HWIDs, and verify the resulting
        -- radio address.

        radio:setHWID("01f263338b1351cf")
        assertEquals(gx.cube:getRadioAddress(), "13/c885851d27")

        radio:setHWID("01b636d36540699d")
        assertEquals(gx.cube:getRadioAddress(), "12/edecde5b7c")

        radio:setHWID("00b636d36540699d")
        assertEquals(gx.cube:getRadioAddress(), "16/671f4bcd7d")

        radio:setHWID("04b636d36540699d")
        assertEquals(gx.cube:getRadioAddress(), "4b/79fe93e32f")
    end
