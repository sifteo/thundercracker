--[[
    Sifteo Thundercracker firmware unit tests

    M. Elizabeth Scott <beth@sifteo.com> 
    Copyright <c> 2011 Sifteo, Inc. All rights reserved.
]]--

require('scripts/luaunit')
require('scripts/vram')

TestGraphics = {}

    function TestGraphics:setUp()
        gx:init()
    end
    
    function TestGraphics:tearDown()
        gx:exit()
    end    

    function TestGraphics:test_solid()
        gx:setMode(VM_SOLID)
        gx:setColor(0, 0x0000)
        gx:drawAndAssert("solid-0000")
        gx:setColor(0, 0xFFFF)
        gx:drawAndAssert("solid-FFFF")
        gx:setColor(0, 0x8000)
        gx:drawAndAssert("solid-8000")
        gx:setColor(0, 0x000F)
        gx:drawAndAssert("solid-000F")
    end

LuaUnit:run()
