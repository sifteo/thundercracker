--[[
    TestJig utilities for the Sifteo Thundercracker simulator's scripting environment

    M. Elizabeth Scott <beth@sifteo.com> 
    Copyright <c> 2012 Sifteo, Inc. All rights reserved.
]]--

local bit = require("bit")

require("radio")
require("vram")

jig = {}

    function jig:getHWID()
        -- Extract HWID from an ACK packet
        return string.sub(gx.cube:testGetACK(), -8)
    end

    function jig:peekHWID()
        -- Read the HWID from NVM
        local hwid = ""
        for i = 0, 7 do
            hwid = hwid .. string.char(gx.cube:nbPeek(i))
        end
        return hwid
    end

    function jig:getFlashACK()
        -- Return the last flash ACK counter byte received by the testjig
        return string.byte(string.sub(gx.cube:testGetACK(), 9, 9))
    end

    function jig:programFlashAndWait(hexCommands, acks)
        acks = acks or (string.len(hexCommands) / 4)
        local baseline = self:getFlashACK()
        local deadline = gx.sys:vclock() + 15

        gx.cube:testWrite(packHex(hexCommands))

        repeat
            gx.yield()
            if gx.sys:vclock() > deadline then
                error(string.format(
                    "Timeout while waiting for flash programming. " ..
                    "baseline=%02x acks=%02x current=%02x",
                    baseline, acks, self:getFlashACK()))
            end
        until bit.band(baseline + acks, 0xFF) == self:getFlashACK()
    end

    function jig:flashReset()
        jig:programFlashAndWait('fe', 1)
    end
