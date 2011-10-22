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

    function TestGraphics:test_solid()
        local colors = { 0x0000, 0xffff, 0x8000, 0x000f, 0x1234 }
        
        -- Plain solid colors
        gx:setMode(VM_SOLID)
        for k, color in pairs(colors) do
            gx:setColors{color}
            gx:drawAndAssert(string.format("solid-%04X", color))
        end
        
        -- Windowed rendering
        gx:setColors{0xF000}
        for k, win in pairs{ 1, 33, 128, 255, 0 } do
            gx:setWindow(71, win)
            gx:drawAndAssert(string.format("solid-win-%d", win, color))
        end
    end
    
    function TestGraphics:test_fb32()
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
        gx:drawAndAssertWithWindow(VM_FB32, "fb32", {1, 4, 16, 17, 92, 255, 0})
    end

    function TestGraphics:test_fb64()
        gx:setMode(VM_FB64)

        -- Checkerboard, with a dark outline
        gx:xbFill(0, 512, 0)
        gx:setColors{gx:RGB565(0.5,0.5,0.5), gx:RGB565(1,1,0)}
        for y = 1, 62, 1 do
            for x = 1, 62, 1 do
                gx:putPixelFB64(x, y, bit.band(x+y, 1))
            end
        end
        gx:drawAndAssert("fb64-outlined")
        
        -- Binary counting pattern, reflected down the middle
        for i = 0, 255, 1 do
            gx.cube:xbPoke(i, i)
        end
        for i = 256, 511, 1 do
            gx.cube:xbPoke(i, 511-i)
        end
        gx:drawAndAssert("fb64-binary")

        -- Windowed rendering
        gx:drawAndAssertWithWindow(VM_FB64, "fb64", {1, 4, 16, 17, 92, 255, 0})       
    end
    
    function TestGraphics:test_fb128()
        gx:setMode(VM_FB128)

        -- Checkerboard, with a dark outline
        gx:xbFill(0, 0x300, 0)
        gx:setColors{gx:RGB565(0.5,0.5,0.5), gx:RGB565(0,1,0)}
        for x = 1, 126, 1 do
            for y = 1, 46, 1 do
                gx:putPixelFB128(x, y, bit.band(x+y, 1))
            end
        end
        gx:drawAndAssert("fb128-outlined")

        -- Binary counting pattern
        for i = 0, 0x2ff, 1 do
            gx.cube:xbPoke(i, bit.bxor(i, bit.rshift(i, 8)))
        end
        gx:drawAndAssert("fb128-binary")

        -- Windowed rendering
        gx:drawAndAssertWithWindow(VM_FB128, "fb128", {1, 2, 20, 48, 48*2, 255, 0})
    end
    
    function TestGraphics:test_rom()
        gx:setMode(VM_BG0_ROM)
        gx:drawROMPattern()
        gx:drawAndAssertWithBG0Pan("rom")
        gx:drawAndAssertWithWindow(VM_BG0_ROM, "rom", {1, 4, 16, 17, 92, 255, 0})       
    end
    
    function TestGraphics:test_rotation()
        -- Uses ROM mode, since that gives us an interesting high-res pattern to test with
        
        gx:setColors{0x1234}
        gx:panBG0(0,0)
        gx:drawROMPattern()
        
        for flags = 0, 7, 1 do        
            gx:setMode(VM_SOLID)
            gx:setWindow(0, 128)
            gx:drawFrame()
            gx:setMode(VM_BG0_ROM)
            gx:setWindow(15, 48)
            
            gx:xbReplace(VA_FLAGS, 5, 3, flags)
            gx:drawAndAssert(string.format("rotate-%d", flags))
        end
    end
    
    function TestGraphics:test_bg0()
        gx:setMode(VM_BG0)
        gx:drawBG0Pattern()
        gx:drawAndAssertWithBG0Pan("bg0")
        gx:drawAndAssertWithWindow(VM_BG0, "bg0", {1, 4, 16, 17, 92, 255, 0})       
    end
    
    function TestGraphics:test_bg2()
        -- Borrow the BG0 test pattern. It'll be reinterpreted as a 16x16 grid
        gx:setMode(VM_BG2)
        gx:drawBG0Pattern()
        
        -- Identity matrix for now
        gx:pokeWords(VA_BG2_BORDER, {0x1234})
        gx:setMatrixBG2(matrix:new(3, 'I'))
        gx:drawAndAssert("bg2-identity")
        
        -- With this matrix, do a windowing test.
        -- Note that the wrap tests (255, 0) will end up wrapping to the
        -- border color, not back to the tile array.
        gx:drawAndAssertWithWindow(VM_BG2, "bg2", {1, 4, 16, 17, 92, 255, 0})       
        gx:setWindow(0, 128)
       
        -- Integer scaling, without rotation or shearing
        local iScales = {0, 0.25, 0.5, 1, 2, 4, 8};
        for i, sx in pairs(iScales) do
            for i, sy in pairs(iScales) do
                gx:setMatrixBG2(gx:scaling(sx, sy))
                gx:drawAndAssert(string.format("bg2-scale-%04x-%04x", sx * 256, sy * 256))
            end
        end
        
        -- Panning
        for i = 1, 16 do
            gx:setMatrixBG2(gx:scaling(0.25, 0.25) * gx:translation(i, 2*i))
            gx:drawAndAssert(string.format("bg2-pan-%d", i))
        end

        -- Rotation about the center of the screen
        for i = 0, 360, 5 do
            gx:setMatrixBG2(gx:translation(64,64) * gx:rotation(i) * gx:translation(-64,-64))
            gx:drawAndAssert(string.format("bg2-rotate-%d", i))
        end
    end
    
    function TestGraphics:test_bg1_no_bits()
        -- Using BG0_BG1 mode, but only testing BG0 functionality and windowing.
        -- The whole BG1 bitmap is zero.
        -- This uses the same reference images as the bare BG0 test.

        gx:setMode(VM_BG0_BG1)
        gx:drawBG0Pattern()
        gx:xbFill(VA_BG1_BITMAP, 32, 0)
        
        -- Try it with several different bg1 panning offsets.
        -- Shouldn't make a difference. Definitely shouldn't crash.
        for i = 0,7 do

            -- This pattern tests the full 8-bit range, plus it tests all 8 pixel alignments
            local pan = i * 255 / 7
            gx:panBG1(pan, pan)

            gx:setWindow(0, 128)
            gx:drawAndAssertWithBG0Pan("bg0")
            gx:drawAndAssertWithWindow(VM_BG0_BG1, "bg0", {1, 4, 16, 17, 92, 255, 0})       
        end
    end
    
    function TestGraphics:test_bg1_clear()
        -- Another BG0_BG1 test which attempts to look exactly like BG1. This time,
        -- we do it by drawing stripes of fully transparent tiles. This tests the slow
        -- transparency path, and our mode changes back and forth to the fast path.
        
        gx:setMode(VM_BG0_BG1)
        gx:drawBG0Pattern()
        gx:xbFill(VA_BG1_BITMAP, 32, 0xA5)

        local clear = gx:drawTransparentTile()
        for i = 0,143 do
            gx:putTileBG1(i, clear)
        end 
        
        for i = 0,7 do
            local pan = i * 255 / 7
            gx:panBG1(pan, pan)
            gx:drawAndAssertWithBG0Pan("bg0")
        end
    end

    function TestGraphics:test_bg1_bits()
        -- Test the bitmap layout in BG0. This draws labelled BG1 tiles on top of
        -- a blank bg0 background. The bitmap is set up in a unique test pattern.
        -- Then, we pan BG1 throughout its range while BG0 holds still.
        
        gx:setMode(VM_BG0_BG1)
        gx:setWindow(0, 128)
        gx:panBG0(0, 0)
        gx:panBG1(0, 0)
        
        -- BG0 is not chroma keyed, this just gives us a funny green tile.
        local clear = gx:drawTransparentTile()
        for x = 0, 17 do
            for y = 0, 17 do
                gx:putTileBG0(x, y, clear)
            end
        end

        -- Fill BG1's tiles with unique images
        for i = 0,143 do
            gx:putTileBG1(i, gx:drawUniqueTile(i))
        end
        
        -- Now plot some interesting un-scrolled patterns...
        
        gx:pokeWords(VA_BG1_BITMAP, {
            0x0FF8, 0x0FF8, 0x0FF8, 0x0FF8, 
            0x0FF8, 0x0FF8, 0x0FF8, 0x0FF8, 
            0x0FF8, 0x0FF8, 0x0FF8, 0x0FF8, 
            0x0FF8, 0x0FF8, 0x0FF8, 0x0FF8, 
        })
        gx:drawAndAssert("bg1-bits-9x16")
        
        -- Do a windowing test while we're here.. since this uses all
        -- 144 tiles, we want to make sure there isn't an xram overflow
        -- even if we have a big window.
        gx:drawAndAssertWithWindow(VM_BG0_BG1, "bg1-bits-9x16", {1, 4, 16, 17, 92, 255, 0})    
        gx:setWindow(0, 128)
        
        -- Back to our regularly scheduled bit patterns...
        
        gx:pokeWords(VA_BG1_BITMAP, {
            0x0000, 0x7FFE, 0x4002, 0x4002,
            0x4002, 0x4002, 0x4002, 0x4002,
            0x4002, 0x4002, 0x4002, 0x4002,
            0x4002, 0x4002, 0x7FFE, 0x0000,
        })
        gx:drawAndAssert("bg1-bits-border1")

        gx:pokeWords(VA_BG1_BITMAP, {
            0xFFFF, 0x8001, 0x8001, 0x8001,
            0x8001, 0x8001, 0x8001, 0x8001,
            0x8001, 0x8001, 0x8001, 0x8001,
            0x8001, 0x8001, 0x8001, 0xFFFF,
        })
        gx:drawAndAssert("bg1-bits-border0")

        -- Now do a panning test. We'll do this with a diagonal pattern,
        -- since it's pretty interesting.

        for y = 2, 13 do
            for x = 2, y do
                gx:putBitBG1(x, y, 1)
            end
        end        
        gx:drawAndAssertWithBG1Pan("bg1-bits-diag")
    end
    
    function TestGraphics:test_bg1_chroma()
        -- Test every combination of chroma keying patterns in BG1, while
        -- panning BG0 underneath.

        gx:setMode(VM_BG0_BG1)
        gx:drawBG0Pattern()
        
        -- These 64 tiles contain every unique 8-bit pattern of
        -- transparent vs. opaque bits.
        for i = 0,63 do
            gx:putTileBG1(i, gx:drawChromaKeyPatternTile(i))
        end
                
        -- Splat these tiles all over. Sometimes consecutive,
        -- sometimes not.
        
        gx:pokeWords(VA_BG1_BITMAP, {
            0xaaaa, 0x5555, 0x0000, 0x0000, 
            0x0000, 0x0ff0, 0x0ff0, 0xffff,
            0x0000, 0x0000, 0x0000, 0x0000,
            0x0000, 0x0000, 0xaaaa, 0x5555,
        })
        
        gx:panBG1(0, 0)
        gx:drawAndAssertWithBG0Pan("bg1-chroma")
        
        gx:panBG0(0, 0)
        gx:drawAndAssertWithBG1Pan("bg1-chroma")
    end

    function TestGraphics:test_bg1_spr_no_bits()
        -- Like test_bg1_no_bits, but in a sprite-capable mode
        -- that has all sprites disabled. The windowing test here
        -- is especially important, since bg0_spr_bg1 uses a different
        -- Y coordinate loop than the other modes.
    
        gx:setMode(VM_BG0_SPR_BG1)
        gx:drawBG0Pattern()
        gx:xbFill(VA_BG1_BITMAP, 32, 0)
        
        -- Zero mask, junk in all other sprite params
        for i = 0,7 do
            gx.cube:xbPoke(VA_SPR + i*6 + 2, 0)  -- mask_x
            gx.cube:xbPoke(VA_SPR + i*6 + 3, 0)  -- mask_y
        end
        
        for i = 0,7 do
            local pan = i * 255 / 7
            gx:panBG1(pan, pan)
            gx:setWindow(0, 128)
            gx:drawAndAssertWithBG0Pan("bg0")
            gx:drawAndAssertWithWindow(VM_BG0_SPR_BG1, "bg0", {1, 4, 16, 17, 92, 255, 0})       
        end
    end
    
    function TestGraphics:test_bg1_spr_clear()
        -- Like bg1_clear, but in a sprite-capable mode that
        -- has all sprites disabled.
    
        gx:setMode(VM_BG0_SPR_BG1)
        gx:drawBG0Pattern()
        gx:xbFill(VA_BG1_BITMAP, 32, 0xA5)

        -- Zero mask, junk in all other sprite params
        for i = 0,7 do
            gx.cube:xbPoke(VA_SPR + i*6 + 2, 0)  -- mask_x
            gx.cube:xbPoke(VA_SPR + i*6 + 3, 0)  -- mask_y
        end
        
        local clear = gx:drawTransparentTile()
        for i = 0,143 do
            gx:putTileBG1(i, clear)
        end 
        
        for i = 0,7 do
            local pan = i * 255 / 7
            gx:panBG1(pan, pan)
            gx:drawAndAssertWithBG0Pan("bg0")
        end
    end
    
    function TestGraphics:test_bg1_spr_chroma()
        -- Like bg1_chroma, but in a sprite-capable mode that
        -- has all sprites disabled. Should look identical to
        -- bg1_chroma, so we use the same screenshots.
        
        gx:setMode(VM_BG0_SPR_BG1)
        gx:drawBG0Pattern()
        
        -- Zero mask, junk in all other sprite params
        for i = 0,7 do
            gx.cube:xbPoke(VA_SPR + i*6 + 2, 0)  -- mask_x
            gx.cube:xbPoke(VA_SPR + i*6 + 3, 0)  -- mask_y
        end
        
        for i = 0,63 do
            gx:putTileBG1(i, gx:drawChromaKeyPatternTile(i))
        end
        
        gx:pokeWords(VA_BG1_BITMAP, {
            0xaaaa, 0x5555, 0x0000, 0x0000, 
            0x0000, 0x0ff0, 0x0ff0, 0xffff,
            0x0000, 0x0000, 0x0000, 0x0000,
            0x0000, 0x0000, 0xaaaa, 0x5555,
        })
        
        gx:panBG1(0, 0)
        gx:drawAndAssertWithBG0Pan("bg1-chroma")
        
        gx:panBG0(0, 0)
        gx:drawAndAssertWithBG1Pan("bg1-chroma")
    end