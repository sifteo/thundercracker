--[[
    Lua code specific to the "filesystem" SDK test.

    We can use pure Lua code in combination with the Siftulator API in order
    to test the firmware's internal filesystem operations, whereas we use the
    SDK itself to test operations that are exposed to applications.
]]--

require('siftulator')
require('luaunit')

fs = Filesystem()
System():setOptions{ turbo=true }

TEST_VOL_TYPE = 0x8765


function volumeString(vol)
    -- Return a string representation of a volume

    local str = string.format("[%02x] type=%04x {", vol, fs:volumeType(vol))
    local map = fs:volumeMap(vol)
    local ec = fs:volumeEraseCounts(vol)

    for index, block in ipairs(map) do
        str = string.format("%s %02x:%d", str, block, ec[index])
    end

    return str .. " }"
end


function countUsedBlocks()
    -- How many total blocks are in use by all volumes?

    local count = 0

    for i, vol in ipairs(fs:listVolumes()) do
        local type = fs:volumeType(vol)

        if type ~= 0x0000 and type ~= 0xFFFF then
            for j, block in ipairs(fs:volumeMap(vol)) do
                if block ~= 0 then
                    count = count + 1
                end
            end
        end
    end

    return count
end


function getFreeSpace()
    -- Estimate how much space is free in the filesystem, in bytes
    local blockSize = 128 * 1024
    local totalSize = 16 * 1024 * 1024
    local overhead = 1024
    return totalSize - overhead - blockSize * countUsedBlocks()
end


function dumpFilesystem()
    -- For debugging: Print out the current state of the filesystem.

    print("------ Volumes ------")

    for index, vol in ipairs(fs:listVolumes()) do
        print(volumeString(vol))
    end

    print(string.format("Free: %d bytes", getFreeSpace()))
end


function filterVolumes()
    -- Filter the list of volumes, looking for only TEST_VOL_TYPE

    local result = fs:listVolumes()
    local j = 1
    while result[j] do
        if fs:volumeType(result[j]) == TEST_VOL_TYPE then
            j = j + 1
        else
            table.remove(result, j)
        end
    end
    return result
end


function testFilesystem()

    -- Dump the volumes that existed on entry

    dumpFilesystem()

    -- Psuedorandomly create and delete volumes

    local testData = string.rep("I am bytes, 16! ", 1024*1024)
    local writeTotal = 0

    math.randomseed(1234)
    for iteration = 1, 50 do

        -- How big of a volume to create?
        local volSize = math.random(10 * 1024 * 1024)

        -- Randomly delete volumes until we can fit this new volume
        while getFreeSpace() < volSize do
            local candidates = filterVolumes()
            local vol = candidates[math.random(table.maxn(candidates))]

            fs:deleteVolume(vol)
            print(string.format("-- Deleted volume [%02x]", vol))
        end

        -- Create the volume
        local vol = fs:newVolume(TEST_VOL_TYPE, string.sub(testData, 1, volSize))
        print(string.format("-- Created volume [%02x], %d bytes", vol, volSize))
        writeTotal = writeTotal + volSize
    end

    dumpFilesystem()
end
