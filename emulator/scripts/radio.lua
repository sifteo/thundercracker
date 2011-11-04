--[[
    Radio utilities for the Sifteo Thundercracker simulator's scripting environment

    M. Elizabeth Scott <beth@sifteo.com> 
    Copyright <c> 2011 Sifteo, Inc. All rights reserved.
]]--

-- General Utilities

function packHex(hexStr)
    r = {}
    for hexByte in hexStr:gmatch("..") do
        r[1+#r] = string.char(tonumber(hexByte, 16))
    end
    return table.concat(r)
end

-- Radio Utilities

radio = {}

    function radio:tx(hexString)
        -- Transmit a hex packet, wait for it to be processed
        gx.cube:handleRadioPacket(packHex(hexString))
        gx.sys:vsleep(0.01)
    end
