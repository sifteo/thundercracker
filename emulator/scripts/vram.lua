--[[
    VRAM utilities for the Sifteo Thundercracker simulator's scripting environment

    M. Elizabeth Scott <beth@sifteo.com> 
    Copyright <c> 2011 Sifteo, Inc. All rights reserved.
]]--

LCD_WIDTH  = 128
LCD_HEIGHT = 128
LCD_PIXELS = 128*128

VRAM_BYTES = 1024
VRAM_WORDS = 512

VRAM_BG0_WIDTH     = 18      -- Width/height of BG0 tile grid
VRAM_BG1_WIDTH     = 16      -- Width/height of BG1 bitmap
VRAM_BG1_TILES     = 144     -- Total number of opaque tiles in BG1
VRAM_BG2_WIDTH     = 16      -- Width/height of BG2 tile grid
VRAM_SPRITES       = 8       -- Maximum number of linear sprites
CHROMA_KEY         = 0x4f    -- Chroma key MSB

-- Bits for 'flags'

VF_TOGGLE          = 0x02    -- Toggle bit, to trigger a new frame render
VF_SYNC            = 0x04    -- Sync with LCD vertical refresh
VF_CONTINUOUS      = 0x08    -- Render continuously, without waiting for toggle
VF_RESERVED        = 0x10
VF_XY_SWAP         = 0x20    -- Swap X and Y axes during render
VF_X_FLIP          = 0x40    -- Flip X axis during render
VF_Y_FLIP          = 0x80    -- Flip Y axis during render

-- Values for 'mode'

VM_MASK            = 0x3c    -- Mask of valid bits in VM_MASK

VM_POWERDOWN       = 0x00    -- Power saving mode, LCD is off
VM_BG0_ROM         = 0x04    -- BG0, with tile data from internal ROM
VM_SOLID           = 0x08    -- Solid color, from colormap[0]
VM_FB32            = 0x0c    -- 32x32 pixel 16-color framebuffer
VM_FB64            = 0x10    -- 64x64 pixel 2-color framebuffer
VM_FB128           = 0x14    -- 128x48 pixel 2-color framebuffer
VM_BG0             = 0x18    -- Background BG0: 18x18 grid
VM_BG0_BG1         = 0x1c    -- BG0, plus overlay BG1: 16x16 bitmap + 144 indices
VM_BG0_SPR_BG1     = 0x20    -- BG0, multiple linear sprites, then BG1
VM_BG2             = 0x24    -- Background BG2: 16x16 grid with affine transform

-- Important VRAM addresses

VA_BG0_TILES       = 0x000
VA_BG2_TILES       = 0x000
VA_BG2_AFFINE      = 0x200
VA_BG2_BORDER      = 0x20c
VA_BG1_TILES       = 0x288
VA_COLORMAP        = 0x300
VA_BG1_BITMAP      = 0x3a8
VA_SPR             = 0x3c8
VA_BG1_X           = 0x3f8
VA_BG1_Y           = 0x3f9
VA_BG0_X           = 0x3fa
VA_BG0_Y           = 0x3fb
VA_FIRST_LINE      = 0x3fc
VA_NUM_LINES       = 0x3fd
VA_MODE            = 0x3fe
VA_FLAGS           = 0x3ff

-- General Utilities

bit = require("bit")
matrix = require("scripts/matrix")

function clamp(x, lower, upper)
    return math.min(math.max(x, lower), upper)
end

-- Graphics Utilities

gx = {}

    function gx:init(useFrontend)
        -- Use one cube, and let the firmware boot.
        
        gx.sys = System()
        gx.cube = Cube(0)               
        gx.sys:setOptions{numCubes=1, noThrottle=true}
        gx.sys:init()
        
        if useFrontend then
            gx.fe = Frontend()
            gx.fe:init()
        end
        
        -- If we had a trace file (-t command line option), enable tracing
        -- before we start the simulated CPU.
        gx.sys:setTraceMode(true)

        -- Starts the simulation thread
        gx.sys:start()
        
        -- Must be enough time for the cube to boot, worst-case
        gx.sys:vsleep(0.3)
        
        -- Make sure the cube has rendered its idle frame
        pixelCount = gx.cube:lcdPixelCount()
        assertEquals(pixelCount >= LCD_PIXELS, true)
        
        -- Make sure the cube has stopped drawing
        gx.sys:vsleep(0.1)
        assertEquals(gx.cube:lcdPixelCount(), pixelCount)
    end
    
    function gx:setUp()
        -- Setup to be done before each test.
        -- Seed the PRNG, and wipe VRAM. Reset the exception counter.
        
        math.randomseed(0)
        gx:wipe()
        gx:setWindow(0, LCD_HEIGHT)
        gx:setRotation(0)
        gx.lastExceptionCount = 0
    end
    
    function gx:exit()
        if gx.fe then
            gx.fe:exit()
        end
        gx.sys:exit()
    end
    
    function gx:wipe()
        -- Put pseudorandom junk in all of mode-specific VRAM.
        -- So, everything except windowing, mode, and flags.
        
        for i = 0, VRAM_WORDS - 3, 1 do
            gx.cube:xwPoke(i, math.random(0, 0xFFFF))
        end
    end
    
    function gx:drawFrame()
        -- Draw a single frame
        
        -- Make sure we are not in continuous rendering mode
        assertEquals(bit.band(gx.cube:xbPeek(VA_FLAGS), VF_CONTINUOUS), 0)
        
        -- Trigger this next frame
        local pixelCount = gx.cube:lcdPixelCount()
        gx.cube:xbPoke(VA_FLAGS, bit.bxor(gx.cube:xbPeek(VA_FLAGS), VF_TOGGLE))

        -- Wait for it to fully complete, or we time out
        
        local timestamp = gx.sys:vclock()
        local pixelsWritten
        repeat
            if gx.fe then
                -- Using a GUI; draw a new frame there
                if not gx.fe:runFrame() then
                    gx.fe:exit()
                    gx.fe = nil
                end
            else
                -- No frontend, just sleep
                gx.sys:vsleep(0.01)   
            end
            
            -- Did the cube hit any exceptions?        
            local newExceptionCount = gx.cube:exceptionCount()
            local exceptions = bit.band(newExceptionCount - gx.lastExceptionCount, 0xFFFFFFFF)
            if exceptions > 0 then
                gx.lastExceptionCount = newExceptionCount
                error("Cube CPU exception!")
            end
            
            -- Wait for the expected number of pixels, with a timeout
            pixelsWritten = bit.band(gx.cube:lcdPixelCount() - pixelCount, 0xFFFFFFFF)
            if pixelsWritten > gx.expectedPixelCount then
                error(string.format("Cube wrote too many pixels (wrote %d, expected %d due to %d-line window)",
                                    pixelsWritten, gx.expectedPixelCount, gx.cube:xbPeek(VA_NUM_LINES)))
            end
            if gx.sys:vclock() - timestamp > 10.0 then
                error("Timed out waiting for a frame to render")
            end
        until pixelsWritten == gx.expectedPixelCount
        
    end
    
    function gx:assertScreenshot(name)
        -- Assert that a screenshot matches the current LCD contents.
        -- If not, we save a copy of the actual LCD screen, and error() out.
        
        local fullPath = string.format("scripts/screenshots/%s.png", name)
        local x, y, lcdColor, refColor;
        
        local status, err = pcall(function()
            x, y, lcdColor, refColor = gx.cube:testScreenshot(fullPath)
        end)

        if not status then
            -- We failed to load the reference screenshot! Odds are, this means
            -- the developer just added a new test and hasn't made a reference
            -- image yet. We'll still want to error out here, but after loudly
            -- explaining what we're doing, and creating a reference image.
            
            gx.cube:saveScreenshot(fullPath)
            if not os.getenv("GENERATE_REFERENCES") then
                error(string.format("%s\n\n" ..
                                    "** Since we failed to open the reference image, assuming that\n" ..
                                    "** this is a brand new test. The current output was just saved to:\n" ..
                                    "**\n" ..
                                    "**    %s\n" ..
                                    "**\n" ..
                                    "** If this was not what you intended, please delete that file!\n" ..
                                    "** If you want to do this automatically for all tests with missing\n" ..
                                    "** reference images, set GENERATE_REFERENCES environment variable.\n",
                                    err, fullPath))
            end
            return
        end
                                
        if x then
            local failedPath = string.format("failed-%s.png", name)
            gx.cube:saveScreenshot(failedPath)
            error(string.format("Screenshot mismatch\n\n" ..
                                "-- At location (%d,%d)\n" ..
                                "-- Actual pixel 0x%04x, expected 0x%04x\n" ..
                                "-- Wrote failed image to \"%s\"\n", 
                                x, y, lcdColor, refColor, failedPath))
        end
    end
    
    function gx:drawAndAssert(name)
        gx:drawFrame()
        gx:assertScreenshot(name)
    end
    
    function gx:hexDumpVRAM()
        for addr = 0, 0x3F0, 0x10 do
            line = string.format("VRAM %04x:", addr)
            for i = addr, addr + 0xF, 1 do
                line = line .. string.format(" %02x", gx.cube:xbPeek(i))
            end
            print(line)
        end
    end
   
    function gx:setMode(m)
        gx.cube:xbPoke(VA_MODE, m)
    end
    
    function gx:setWindow(first, num)
        gx.cube:xbPoke(VA_FIRST_LINE, first)
        gx.cube:xbPoke(VA_NUM_LINES, num)
        
        -- Zero lines will cause a wraparound and be treated as 256 lines
        if num == 0 then
            num = 256
        end
        gx.expectedPixelCount = num * LCD_WIDTH
    end
    
    function gx:RGB565(r, g, b)
        -- Original colors are in floating point, between 0 and 1.
        -- Note that tobit() already appears to round rather than truncating, so we don't add 0.5
        r = bit.tobit(clamp(r, 0, 1) * 31)
        g = bit.tobit(clamp(g, 0, 1) * 63)
        b = bit.tobit(clamp(b, 0, 1) * 31)
        return bit.bor(bit.lshift(r, 11), bit.bor(bit.lshift(g, 5), b))
    end

    function gx:setColor(index, value)
        gx.cube:xwPoke(VA_COLORMAP/2 + index, value)
    end
    
    function gx:setColors(t)
        for k, v in pairs(t) do
            -- Note that we're converting from Lua's one-based indices to zero-based
            gx:setColor(k-1, v)
        end
    end
    
    function gx:setGrayscalePalette()
        for i = 0, 15, 1 do
            local g = i/15
            gx:setColor(i, gx:RGB565(g,g,g))
        end
    end
    
    function gx:setUniquePalette()
        -- Set a palette for testing purposes, in which every byte of every color is unique
        gx:setColors{0x00ff, 0x10ef, 0x20df, 0x30cf,
                     0x40bf, 0x52a4, 0x6294, 0x7284,
                     0x8274, 0x9264, 0xa157, 0xb147,
                     0xc137, 0xd127, 0xe117, 0xf107}
    end
    
    function gx:setRotation(f)
        gx:xbReplace(VA_FLAGS, 5, 3, f)
    end    
    
    function gx:pokeWords(byteAddr, tab)
        for k, v in pairs(tab) do
            gx.cube:xwPoke(byteAddr/2 + k - 1, v)
        end
    end
    
    function gx:panBG0(x, y)
        gx.cube:xbPoke(VA_BG0_X, x)
        gx.cube:xbPoke(VA_BG0_Y, y)
    end
    
    function gx:panBG1(x, y)
        gx.cube:xbPoke(VA_BG1_X, x)
        gx.cube:xbPoke(VA_BG1_Y, y)
    end
        
    function gx:xbReplace(byteIndex, bitIndex, width, value)
        -- Replace a bit-field in an xdata byte
        local mask = bit.bnot(bit.lshift(bit.lshift(1, width) - 1, bitIndex))
        gx.cube:xbPoke(byteIndex, bit.bor(bit.band(gx.cube:xbPeek(byteIndex), mask), bit.lshift(value, bitIndex)))
    end
        
    function gx:xbFill(addr, len, value)
        for i = addr, addr+len-1, 1 do
            gx.cube:xbPoke(i, value)
        end
    end

    function gx:xwFill(addr, len, value)
        for i = addr, addr+len-1, 1 do
            gx.cube:xwPoke(i, value)
        end
    end
    
    function gx:tileIndex(i)
        return bit.bor( bit.band(bit.lshift(i, 2), 0xFE00), bit.band(bit.lshift(i, 1), 0x00FE) )
    end
    
    function gx:putPixelFB32(x, y, color)
        gx:xbReplace(bit.rshift(x, 1) + y*16, bit.band(x,1) * 4, 4, color)
    end
    
    function gx:putPixelFB64(x, y, color)
        gx:xbReplace(bit.rshift(x, 3) + y*8, bit.band(x,7), 1, color)
    end

    function gx:putPixelFB128(x, y, color)
        gx:xbReplace(bit.rshift(x, 3) + y*16, bit.band(x,7), 1, color)
    end
    
    function gx:putPixelFlash(index, x, y, r, g, b)
        -- Draw a colored pixel to a tile in flash
        local addr = index * 64 + y * 8 + x
        local rgb = gx:RGB565(r,g,b)
        
        -- Endian swap
        gx.cube:fwPoke(addr, bit.bor(bit.rshift(rgb, 8), bit.lshift(rgb, 8)))
    end
    
    function gx:putTileBG0(x, y, index)
        gx.cube:xwPoke(x + y*18, gx:tileIndex(index))
    end

    function gx:putTileBG1(addr, index)
        gx.cube:xwPoke(VA_BG1_TILES/2 + addr, gx:tileIndex(index))
    end

    function gx:putBitBG1(x, y, value)
        gx:xbReplace(VA_BG1_BITMAP + y*2 + bit.rshift(x, 3),
                     bit.band(x, 7), 1, value)
    end
    
    function gx:putTileBG0ROM(x, y, tile, palette, mode)
        gx:putTileBG0(x, y, bit.bor( tile, bit.bor( bit.lshift(palette, 10), bit.lshift(mode, 9) )))
    end
    
    function gx:setMatrixBG2(m)
        gx:pokeWords(VA_BG2_AFFINE, { m[1][3] * 256, m[2][3] * 256,
                                      m[1][1] * 256, m[2][1] * 256,
                                      m[1][2] * 256, m[2][2] * 256 })
    end   
  
    function gx:translation(x, y)
        return matrix:new{{1, 0, x},
                          {0, 1, y},
                          {0, 0, 1}}
    end
    
    function gx:scaling(x, y)
        return matrix:new{{x, 0, 0},
                          {0, y, 0},
                          {0, 0, 1}}
    end

    function gx:rotation(degrees)
        local a = degrees / 180 * math.pi
        local s = math.sin(a)
        local c = math.cos(a)
        return matrix:new{{c, -s, 0},
                          {s,  c, 0},
                          {0,  0, 1}}
    end

    function gx:drawROMPattern()
        -- Create a test pattern that uses both modes, and a variety
        -- of characters and palettes, while trying to keep to things
        -- at the beginning of the ROM, which aren't likely to change
        -- as we modify the ROM graphics set.

        for y = 0, 17, 1 do
            for x = 0, 17, 1 do
                local ascii_a = 97 - 32
                local tile = bit.band(x+y, 0xF) + ascii_a
                local palette = bit.band(y, 0x3)
                local mode = bit.band(y, 1)
                gx:putTileBG0ROM(x, y, tile, palette, mode)
            end
        end
    end
    
    function gx:drawBG0Pattern()
        -- Create a test pattern for BG0 modes. Picks arbitrary unique tile
        -- indices for the map, and draws unique tile data to flash. Each tile
        -- has added uniqueness in its colors, plus it has a distinctive border
        -- and labelling to aid in debugging.
        
       for y = 0, 17, 1 do
            for x = 0, 17, 1 do
                gx:putTileBG0(x, y, gx:drawUniqueTile(x + y*18))

            end
        end        
    end
    
    function gx:drawUniqueTile(i)
        -- Draw a uniquely marked tile to flash, using nonsequential flash addresses.
        -- "i" should be between 0 and 18*18
        
        local index = bit.bxor(i * 31, 0x3FFF)
        for ty = 0,7,1 do
            for tx = 0,7,1 do
                
                -- Gradient background
                local r = tx / 16
                local g = ty / 16
                local b = index / 0x8000
                
                -- Brighter border
                if tx == 0 or tx == 7 or ty == 0 or ty == 7 then
                    r = r + 0.5
                    g = g + 0.5
                    b = b + 0.5
                end
                
                -- Encode the tile's index in binary, as a 4x4 square of bits
                if tx >= 2 and tx <= 5 and ty >= 2 and ty <= 5 then
                    local bitIndex = (tx - 2) + (ty - 2)*4
                    if bit.band(bit.rshift(index, bitIndex), 1) > 0 then
                        b = 1
                    end
                end
                
                gx:putPixelFlash(index, tx, ty, r,g,b)
            end
        end     
        
        return index
    end
    
    function gx:drawTransparentTile()
        -- Draw a fully transparent tile to flash, using an index not used by drawUniqueTile.
        -- This uses many different pixel values that all happen to have the CHROMA_KEY byte as their MSB.
        
        local index = 1

        for i = 0, 63 do
            gx.cube:fwPoke(index*64 + i, CHROMA_KEY + bit.lshift(i, 9))
        end
        return index
    end
    
    function gx:drawAndAssertWithWindow(mode, name, sizes)
        -- Run a test at a variety of different window sizes
        
        for i, width in pairs(sizes) do
            gx:setMode(VM_SOLID)
            gx:setWindow(0, 128)
            gx:drawFrame()
            gx:setMode(mode)
            gx:setWindow(33, width)
            gx:drawAndAssert(string.format("%s-win-%d", name, width))
        end    
    end
    
    function gx:drawAndAssertWithBG0Pan(name)
        -- Run a test at a variety of BG0 pixel/tile panning offsets.
        -- Tests pixel-panning plus tile-panning and map wrap-around

        local function tryPan(x, y)
            gx:panBG0(x, y)
            gx:drawAndAssert(string.format("%s-pan0-%d-%d", name, x, y))
        end   
         
        -- Pixel panning, near the origin
        for i = 0, 18, 1 do
            tryPan(i, bit.rshift(i, 1))
        end
        
        -- Horizontal wrap
        for i = 0, 17, 4 do
            tryPan(i*8, 0)
        end
        
        -- Vertical wrap
        for i = 0, 17, 4 do
            tryPan(0, i*8)
        end
        
        -- Edge of valid range
        tryPan(143, 143)
    end

    function gx:drawAndAssertWithBG1Pan(name)
        -- Run a test at a variety of BG1 pixel/tile panning offsets.
        -- This tests positive and negative offsets in both dimensions.
        -- Remember, BG1 does not wrap. For all practical purposes, it
        -- treats the panning offset as a signed 8-bit number.
        
        local function tryPan(x, y)
            x = bit.band(x, 0xFF)
            y = bit.band(y, 0xFF)
            gx:panBG1(x, y)
            gx:drawAndAssert(string.format("%s-pan1-%d-%d", name, x, y))
        end   
         
        -- Pixel panning around the origin, in both directions
        for i = -20, 20, 1 do
            tryPan(i, i*3)
        end
    end
