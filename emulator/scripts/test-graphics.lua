--[[
    Sifteo Thundercracker firmware unit tests

    M. Elizabeth Scott <beth@sifteo.com> 
    Copyright <c> 2011 Sifteo, Inc. All rights reserved.
]]--

require('scripts/luaunit')
require('scripts/vram')

TestGraphics = {}

    function TestGraphics:setUp()
        gx:setUp()
    end

    function TestGraphics:test0_solid()
        local colors = { 0x0000, 0xffff, 0x8000, 0x000f, 0x1234 }
        
        -- Plain solid colors
        gx:setMode(VM_SOLID)
        for k, color in pairs(colors) do
            gx:setColors{color}
            gx:drawAndAssert(string.format("solid-%04X", color))
        end
        
        -- Windowed rendering
        gx:setColors{0xF000}
        for k, win in pairs{ 1, 33, 128, 255 } do
            gx:setWindow(71, win)
            gx:drawAndAssert(string.format("solid-win-%d", win, color))
        end
    end
    
    function TestGraphics:test1_fb32()
        gx:setMode(VM_FB32)

        -- Horizontal gradient with a black outline, in a unique palette
        gx:xbFill(0, 512, 0)
        gx:setUniquePalette()
        for y = 1, 30, 1 do
            for x = 1, 30, 1 do
                gx:putPixelFB32(x, y, bit.band(x, 0xF))
            end
        end
        gx:drawAndAssert("fb32-outlined")
        
        -- Arbitrary pattern
        for i = 0, 511, 1 do
            gx.cube:xbPoke(i, i * 17)
        end
        gx:drawAndAssert("fb32-arbitrary")

        -- Diagonal gradient
        gx:xbFill(0, 512, 0)
        gx:setGrayscalePalette()
        for y = 0, 31, 1 do
            for x = 0, 31, 1 do
                gx:putPixelFB32(x, y, (x+y)*15/(31+31))
            end
        end
        gx:drawAndAssert("fb32-gradient")
        
        -- Windowed rendering
        
        gx:setColors{ 0x0F00 }
        for i, width in pairs{ 1, 4, 16, 17, 92, 255 } do
            gx:setMode(VM_SOLID)
            gx:setWindow(0, 128)
            gx:drawFrame()
            gx:setMode(VM_FB32)
            gx:setWindow(33, width)
            gx:drawAndAssert(string.format("fb32-win-%d", width))
        end        
    end

gx:init(true)
LuaUnit:run()
gx:exit()
