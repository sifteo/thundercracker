--[[
    Sifteo Thundercracker firmware unit tests

    M. Elizabeth Scott <beth@sifteo.com> 
    Copyright <c> 2011 Sifteo, Inc. All rights reserved.
]]--

require('scripts/luaunit')
require('scripts/vram')

TestGraphics = {}

    function TestGraphics:test_solid()
        local colors = { 0x0000, 0xffff, 0x8000, 0x000f, 0x1234 }
        gx:setMode(VM_SOLID)
        for k, color in pairs(colors) do
            gx:setColors{color}
            gx:drawAndAssert(string.format("solid-%04X", color))
        end
    end
    
    function TestGraphics:test_fb32()
        gx:setMode(VM_FB32)
        gx:setUniquePalette()
        
        -- Arbitrary pattern
        for i = 0, 511, 1 do
            gx.cube:xbPoke(i, i * 17)
        end
        gx:drawAndAssert("fb32-arbitrary")
    end

gx:init()
LuaUnit:run()
gx:exit()
