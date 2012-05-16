--[[
    Lua code specific to the "filesystem" SDK test.

    We can use pure Lua code in combination with the Siftulator API in order
    to test the firmware's internal filesystem operations, whereas we use the
    SDK itself to test operations that are exposed to applications.
]]--

require('siftulator')
require('luaunit')

fs = Filesystem()


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


function dumpFilesystem()
    -- For debugging: Print out the current state of the filesystem.

    print("------ Volumes ------")
    for index, vol in ipairs(fs:listVolumes()) do
        print(volumeString(vol))
    end
end


function testFilesystem()

    -- Dump and save the volumes that existed on entry.

    dumpFilesystem()
    origVolumes = fs:listVolumes()

    -- Test data

    data_1K = string.rep("foobars.", 1024 / 8)
    data_128K = string.rep("foobars.", 128 * 1024 / 8)
    data_1M = string.rep("foobars.", 1024 * 1024 / 8)

    -- Create some test volumes

    local v1 = fs:newVolume(0x1234, data_1K)
    local v2 = fs:newVolume(0x1235, data_1M)
    local v3 = fs:newVolume(0x1236, data_1K)
    local v4 = fs:newVolume(0x1237, data_128K)

    dumpFilesystem()

end
