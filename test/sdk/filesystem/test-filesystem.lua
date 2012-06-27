--[[
    Lua code specific to the "filesystem" SDK test.

    We can use pure Lua code in combination with the Siftulator API in order
    to test the firmware's internal filesystem operations, whereas we use the
    SDK itself to test operations that are exposed to applications.
]]--

require('siftulator')
require('flashreplay')
require('luaunit')

System():setOptions{ turbo=true, numCubes=0 }
fs = Filesystem()
writeTotal = 0

TEST_VOL_TYPE = 0x8765
BLOCK_SIZE = 128 * 1024
DEVICE_SIZE = 16 * 1024 * 1024


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
    local overhead = 1024
    return DEVICE_SIZE - overhead - BLOCK_SIZE * countUsedBlocks()
end


function dumpFilesystem()
    -- For debugging: Print out the current state of the filesystem.

    print("------ Volumes ------")

    for index, vol in ipairs(fs:listVolumes()) do
        print(volumeString(vol))
    end

    print(string.format("Free: %d bytes", getFreeSpace()))
end


function getMaxEraseCount()
    local maxEC = 0
    for index, ec in ipairs(fs:simulatedSectorEraseCounts()) do
        maxEC = math.max(maxEC, ec)
    end
    return maxEC
end


function dumpEndurance()
    local numBins = 40
    local maxEC = getMaxEraseCount()
    local binWidth = math.ceil(maxEC / numBins)
    local histogram = {}
    local maxCount = 0
    local maxBin = 0    -- Due to quantization, may be less than numBins

    for index, ec in ipairs(fs:simulatedSectorEraseCounts()) do
        local bin = 1 + math.floor(ec / binWidth)
        histogram[bin] = 1 + (histogram[bin] or 0)
        maxCount = math.max(maxCount, histogram[bin])
        maxBin = math.max(maxBin, bin)
    end

    print("------ Endurance Histogram ------")

    for bin = 1, maxBin do
        local count = histogram[bin] or 0
        print(string.format("[EC < %4d] %4d blocks %s",
            binWidth * bin, count,
            string.rep("#", math.ceil(40 / maxCount * count))
        ))
    end
end


function checkEndurance()
    -- Ensure that our wear levelling didn't fail too badly.

    local idealEraseCount = writeTotal / DEVICE_SIZE
    local maxEC = getMaxEraseCount()
    local ratio = maxEC / idealEraseCount
    local ratioLimit = 1.5

    print(string.format("--        Ideal erase count: %.2f", idealEraseCount))
    print(string.format("--  Actual peak erase count: %d", maxEC))
    print(string.format("--    Ratio of peak / ideal: %.4f (Max %f)", ratio, ratioLimit))

    if ratio > ratioLimit then
        error("Wear levelling failed, peak erase count higher than allowed")
    end
end


function testRandomVolumes(verbose)
    -- Psuedorandomly create and delete volumes

    print "Testing random volume creation and deletion"

    local testData = string.rep("I am bytes, 16! ", 1024*1024)
    math.randomseed(1234)

    for iteration = 1, 100 do

        -- How big of a volume to create?
        local volSize = math.random(10 * 1024 * 1024)

        -- Randomly delete volumes until we can fit this new volume
        while getFreeSpace() < volSize do
            local candidates = filterVolumes()
            local vol = candidates[math.random(table.maxn(candidates))]

            fs:deleteVolume(vol)
            if verbose then
                print(string.format("Deleted volume [%02x]", vol))
            end
        end

        -- Create the volume
        local vol = fs:newVolume(TEST_VOL_TYPE, string.sub(testData, 1, volSize))
        if verbose then
            print(string.format("Created volume [%02x], %d bytes", vol, volSize))
        end

        writeTotal = writeTotal + math.ceil(volSize / BLOCK_SIZE) * BLOCK_SIZE
    end
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


function assertVolumes(t)
    -- Raise an error if the output of filterVolumes() doesn't contain
    -- the exact same set of volumes as 'table'. Note that the parameter
    -- is sorted in-place.

    local filtered = filterVolumes()

    table.sort(filtered)
    table.sort(t)

    for index = 1, math.max(table.maxn(filtered), table.maxn(t)) do
        if filtered[index] ~= t[index] then
            error(string.format("Mismatch in test volumes, expected {%s}, found {%s}",
                table.concat(t, ","), table.concat(filtered, ",")))
        end
    end
end


function testHierarchy(verbose)
    -- Create trees of volumes, and assure that they are deleted correctly.

    print "Testing hierarchial volume deletion"

    -- By running this several times, we both contribute toward testing wear
    -- levelling, and we test the deletion code with various orderings of
    -- volumes on the device.
    for iteration = 1, 100 do

        -- Account for our test volumes when computing wear levelling performance
        writeTotal = writeTotal + BLOCK_SIZE * 9

        local a = fs:newVolume(TEST_VOL_TYPE, "Foo", "", 0)
        local b = fs:newVolume(TEST_VOL_TYPE, "Foo", "", a)
        local c = fs:newVolume(TEST_VOL_TYPE, "Foo", "", a)
        local d = fs:newVolume(TEST_VOL_TYPE, "Foo", "", c)
        local e = fs:newVolume(TEST_VOL_TYPE, "Foo", "", d)

        local f = fs:newVolume(TEST_VOL_TYPE, "Foo", "", 0)
        local g = fs:newVolume(TEST_VOL_TYPE, "Foo", "", f)
        local h = fs:newVolume(TEST_VOL_TYPE, "Foo", "", g)
        local i = fs:newVolume(TEST_VOL_TYPE, "Foo", "", h)

        local allVolumes = {a, b, c, d, e, f, g, h, i}
        if verbose then
            print(string.format("Hierarchy iter %d: volumes {%s}",
                iteration, table.concat(allVolumes, ",")))
        end

        -- All volumes must be present to start with.
        -- (Note that this has the side-effect of sorting allVolumes)
        assertVolumes(allVolumes)

        -- Delete the subtree containing {h, i}
        fs:deleteVolume(h)
        assertVolumes{a, b, c, d, e, f, g}

        -- Delete the entire tree below 'a', but leave the other tree
        fs:deleteVolume(a)
        assertVolumes{f, g}

        -- Now delete the other subtree
        fs:deleteVolume(f)
        assertVolumes{}

    end
end

function measureVolumeSize(payloadSize, hdrDataSize)
    -- Creates a volume, measures its size (in map blocks), and deletes it.

    local vol = fs:newVolume(TEST_VOL_TYPE, string.rep("x", payloadSize), string.rep("y", hdrDataSize))
    local size = table.maxn(fs:volumeMap(vol))
    fs:deleteVolume(vol)

    writeTotal = writeTotal + payloadSize + hdrDataSize + 256
    return size
end

function testVolumeSizes()
    -- Test some edge cases for volume payload and header-data size,
    -- and be sure the volume itself gets the correct size.

    print "Testing volume size edge-cases"

    -- Various small size combinations (< one cache block)
    assertEquals(measureVolumeSize(0, 0), 1)
    assertEquals(measureVolumeSize(100, 0), 1)
    assertEquals(measureVolumeSize(0, 100), 1)
    assertEquals(measureVolumeSize(100, 100), 1)

    -- Greater than one cache block, still one volume
    assertEquals(measureVolumeSize(0, 0), 1)
    assertEquals(measureVolumeSize(10000, 0), 1)
    assertEquals(measureVolumeSize(0, 10000), 1)
    assertEquals(measureVolumeSize(10000, 10000), 1)

    -- Payload size overflow from 1 to 2 volumes
    assertEquals(measureVolumeSize(128*1024 - 256, 0), 1)
    assertEquals(measureVolumeSize(128*1024 - 256 + 1, 0), 2)

    -- Payload size overflow from 1 to 2 volumes, with max hdrdata size
    assertEquals(measureVolumeSize(128*1024 - 256, 216), 1)
    assertEquals(measureVolumeSize(128*1024 - 256 + 1, 216), 2)
    assertEquals(measureVolumeSize(128*1024 - 256, 217), 2)
    assertEquals(measureVolumeSize(128*1024 - 256 + 1, 217), 2)

    -- Here's where the overflows get nonlinear, due to the header exceeding one cache block
    assertEquals(measureVolumeSize(0x57ff00, 0), 44)
    assertEquals(measureVolumeSize(0x57ff01, 0), 45)
    assertEquals(measureVolumeSize(0x57fe00, 8), 44)
    assertEquals(measureVolumeSize(0x57fe01, 8), 45)

    -- And the next discontiguous bit..
    assertEquals(measureVolumeSize(0xbdfe00, 0), 95)
    assertEquals(measureVolumeSize(0xbdfe01, 0), 96)
    assertEquals(measureVolumeSize(0xbdfd00, 8), 95)
    assertEquals(measureVolumeSize(0xbdfd01, 8), 96)
    assertEquals(measureVolumeSize(0xbffe00, 0), 96)
    assertEquals(measureVolumeSize(0xbffe01, 0), 97)
    assertEquals(measureVolumeSize(0xbffd00, 8), 96)
    assertEquals(measureVolumeSize(0xbffd01, 8), 97)
end

function testAllocFail()
    -- Keep allocating until we fail

    print "Testing allocation failure"

    local volumes = {}
    local volId = 0

    assertVolumes{}

    repeat
        volId = volId + 1
        if volId > 128 then
            error("Filesystem isn't filling up when it should?")
        end

        volumes[volId] = fs:newVolume(TEST_VOL_TYPE, "Foo")
        assertVolumes(volumes)

    until not volumes[volId]

    for index, v in ipairs(volumes) do
        fs:deleteVolume(v)
    end

    assertVolumes{}
end

function testFilesystem()
    -- Dump the volumes that existed on entry
    dumpFilesystem()

    -- Frob the cache, why not.
    fs:invalidateCache()

    -- Individual filesystem exercises
    testHierarchy()
    testAllocFail()
    testVolumeSizes()
    testRandomVolumes()
end

function dumpAndCheckFilesystem()
    dumpFilesystem()
    dumpEndurance()
    checkEndurance()
end
