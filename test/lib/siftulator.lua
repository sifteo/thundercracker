--[[
    Utilities for the Lua scripting environment in Siftulator.

    M. Elizabeth Scott <beth@sifteo.com> 
    Copyright <c> 2012 Sifteo, Inc. All rights reserved.
]]--

SCREENSHOT_PATH_FMT = "screenshots/%s.png"

util = {}

    function util:assertScreenshot(cube, name, tolerance)
        -- Assert that a screenshot matches the current LCD contents.
        -- If not, we save a copy of the actual LCD screen, and error() out.
        
        local fullPath = string.format(SCREENSHOT_PATH_FMT, name)
        local x, y, lcdColor, refColor;
        
        local status, err = pcall(function()
            x, y, lcdColor, refColor, errVal = cube:testScreenshot(fullPath, tolerance)
        end)

        if not status then
            -- We failed to load the reference screenshot! Odds are, this means
            -- the developer just added a new test and hasn't made a reference
            -- image yet. We'll still want to error out here, but after loudly
            -- explaining what we're doing, and creating a reference image.
            
            cube:saveScreenshot(fullPath)
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
            cube:saveScreenshot(failedPath)
            error(string.format("Screenshot mismatch\n\n" ..
                                "-- At location (%d,%d)\n" ..
                                "-- Actual pixel 0x%04x, expected 0x%04x (error of %d)\n" ..
                                "-- Wrote failed image to \"%s\"\n", 
                                x, y, lcdColor, refColor, errVal, failedPath))
        end
    end

    function util:findVolumeInSysLFS(vol)
        -- See if we can find a particular volume inside SysLFS. Returns 1 if we see it anywhere, 0 if not.

        fs = Filesystem()

        -- Iterate over all SysLFS CubeRecords
        for key = 0x28, 0x3f do
            local rec = fs:readObject(0, key)
            if rec then

                -- Iterate over slots
                for slot = 1, 8 do

                    -- Extract slots[slot].identity.volume
                    local slotIdentityVolume = rec:byte(1 + 2 + slot*4)

                    if slotIdentityVolume == vol then
                        return 1
                    end
                end
            end
        end

        return 0
    end
