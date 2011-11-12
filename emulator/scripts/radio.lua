--[[
    Radio utilities for the Sifteo Thundercracker simulator's scripting environment

    M. Elizabeth Scott <beth@sifteo.com> 
    Copyright <c> 2011 Sifteo, Inc. All rights reserved.
]]--

-- General Utilities

local bit = require("bit")

function packHex(hexStr)
    -- Hex to string
    local r = {}
    for hexByte in hexStr:gmatch("..") do
        r[1+#r] = string.char(tonumber(hexByte, 16))
    end
    return table.concat(r)
end

function packHexN(hexStr)
    -- Hex to string, with nybble swapping
    local r = {}
    for hexByte in hexStr:gmatch("..") do
        local num = tonumber(hexByte, 16)
        num = bit.bor(bit.lshift(bit.band(num, 0x0F), 4),
                      bit.rshift(bit.band(num, 0xF0), 4))
        r[1+#r] = string.char(num)
    end
    return table.concat(r)
end

-- Radio Utilities

radio = {}

    function radio:tx(hexString)
        -- Transmit a hex packet, wait for it to be processed.

        gx.cube:handleRadioPacket(packHex(hexString))
        gx.sys:vsleep(0.01)
    end

    function radio:txn(hexString)
        -- Nybble-swapped version of tx(). Our overall protocol is
        -- nybble-based, so unless we're sending flash escape data it usually
        -- makes more sense to handle each nybble as little-endian rather than
        -- left-to-right (big endian).
       
        gx.cube:handleRadioPacket(packHexN(hexString))
        gx.sys:vsleep(0.01)
    end
