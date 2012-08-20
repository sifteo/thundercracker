#!/usr/bin/env siftulator -e
--[[

    Sifteo SDK:
    A Lua script to dump the contents of Siftulator's filesystem.

    Save a filesystem image using:
        rm -f filesystem.bin
        siftulator -F filesystem.bin [game.elf ...]

    Dump the filesystem contents using:
        siftulator -F filesystem.bin -e filesystem-dump.lua

    Or equivalently, on UNIX-like OSes:
        filesystem-dump.lua -F filesystem.bin

--]]

sys = System()
fs = Filesystem()

-- Information about volume types
VOLUME_TYPE_INFO = {}
VOLUME_TYPE_INFO[0x0000] = { name="Deleted", hidden=true }
VOLUME_TYPE_INFO[0xffff] = { name="Incomplete", hidden=true }
VOLUME_TYPE_INFO[0x4c45] = { name="Pre-erase Log", hidden=true }
VOLUME_TYPE_INFO[0x5346] = { name="Log-structured Filesystem", hidden=true, lfs=true }
VOLUME_TYPE_INFO[0x4d47] = { name="Game", elf=true }
VOLUME_TYPE_INFO[0x4e4c] = { name="Launcher", elf=true }

-- Information about metadata types
METADATA_INFO = {}
METADATA_INFO[0x0001] = { name="UUID" }
METADATA_INFO[0x0002] = { name="Bootstrap Assets" }
METADATA_INFO[0x0003] = { name="Title String" }
METADATA_INFO[0x0004] = { name="Package String" }
METADATA_INFO[0x0005] = { name="Version String" }
METADATA_INFO[0x0006] = { name="96x96 Icon" }
METADATA_INFO[0x0007] = { name="AssetSlot Count" }
METADATA_INFO[0x0008] = { name="Cube Range" }


function main()
    -- Run the FS dump in a trivial master-only simulation.
    -- We initialize the master simulation, but don't start() it.
    -- In script mode (as opposed to inline mode), Lua normally
    -- doesn't run on the same thread as the simulated base.

    sys:setOptions{numCubes=0, turbo=true}
    sys:init()

    heading("Block Allocation")
    dumpAllocation()

    heading("Endurance Histogram")
    dumpEndurance()

    heading("Volumes")
    dumpVolume(getVolumeTree())

    sys:exit()
end


function heading(text)
    print(string.format("\n%s %s\n", string.rep("=", 16), text))
end

function indentStr(indent)
    return string.rep(" ", indent * 4)
end

function hexDump(str, indent)
    for byte = 1, #str, 16 do
        local chunk = str:sub(byte, byte+15)
        io.write(string.format("%s[%04x] ", indentStr(indent), byte-1))
        chunk:gsub(".", function (c) io.write(string.format('%02x ',string.byte(c))) end)
        io.write(string.rep(" ", 3 * (16 - #chunk)))
        io.write("  ", chunk:gsub("[^%w%p ]", "."), "\n")
    end
end


function getMaxEraseCount()
    local maxEC = 0
    for index, ec in ipairs(fs:simulatedBlockEraseCounts()) do
        maxEC = math.max(maxEC, ec)
    end
    return maxEC
end


function dumpAllocation()
    local blocks = {}
    local vols = fs:listVolumes()

    -- Create a reverse mapping from blocks to volumes
    for i, vol in ipairs(vols) do
        local map = fs:volumeMap(vol)
        for j, block in ipairs(map) do
            blocks[block] = vol
        end
    end

    -- Display all FS blocks

    local prevStr = nil
    for block = 1, 128 do

        local str = shortVolumeString(blocks[block])
        io.write((str == prevStr and str ~= ' .. ') and '----' or str)
        prevStr = str

        if (block % 16) == 0 then
            io.write("\n")
            prevStr = nil
        end
    end

    -- Display all volumes

    io.write("\n")
    for i, vol in ipairs(vols) do
        print(longVolumeString(vol))
    end
end


function longVolumeString(vol)
    -- Return a string representation of a volume

    local str = string.format("Volume<%02x> type=%04x parent=%02x {",
        vol, fs:volumeType(vol), fs:volumeParent(vol))

    local map = fs:volumeMap(vol)
    local ec = fs:volumeEraseCounts(vol)

    for i, block in ipairs(map) do
        str = string.format("%s %02x:%d", str, block, ec[i])
    end

    return str .. " }"
end


function shortVolumeString(vol)
    -- Return a tiny (4 char) representation of a volume, for the usage map

    if not vol then
        -- Block is totally unallocated
        return ' .. '
    end

    local type = fs:volumeType(vol)
    local parent = fs:volumeParent(vol)

    if t == 0x0000 or t == 0xffff then
        -- Deleted / Incomplete
        return ' __ '
    end

    -- Under a parent? Display the parent's info.
    if parent > 0 then
        return string.format(" %02x ", parent)
    end

    -- Normal volume
    return string.format("<%02x>", vol)
end


function dumpEndurance()
    local numBins = 40
    local maxEC = math.max(1, getMaxEraseCount())
    local binWidth = math.ceil(maxEC / numBins)
    local histogram = {}
    local maxCount = 0
    local maxBin = 0    -- Due to quantization, may be less than numBins

    for index, ec in ipairs(fs:simulatedBlockEraseCounts()) do
        local bin = 1 + math.floor(ec / binWidth)
        histogram[bin] = 1 + (histogram[bin] or 0)
        maxCount = math.max(maxCount, histogram[bin])
        maxBin = math.max(maxBin, bin)
    end

    for bin = 1, maxBin do
        local count = histogram[bin] or 0
        print(string.format("[EC < %4d] %4d blocks %s",
            binWidth * bin, count,
            string.rep("#", math.ceil(40 / maxCount * count))
        ))
    end
end


function getVolumeTree()
    -- Convert the volume list into a tree structure, which preserves
    -- the parent-child relationship between volumes.

    local root = { children={} }
    local map = {}
    map[0] = root

    -- Allocate tables for each volume
    local volList = fs:listVolumes()
    for i, vol in ipairs(volList) do
        local type = fs:volumeType(vol)
        local typeInfo = VOLUME_TYPE_INFO[type] or { name = "Unknown" }
        map[vol] = { vol=vol, children={}, type=type, typeInfo=typeInfo }
    end

    -- Build the tree
    for i, vol in ipairs(volList) do
        local parent = fs:volumeParent(vol) or 0
        map[parent].children[vol] = map[vol]

        -- Propagate LFS flag to parent
        if map[vol].typeInfo.lfs then
            map[parent].lfs = true
        end
    end

    return root
end


function dumpMetadata(vol, indent)
    for key = 0x0000, 0xFFFF do
        local value = fs:readMetadata(vol, key)
        if value then
            info = METADATA_INFO[key] or { name="" }
            print(string.format("%sMetadata<%04x> %s",
                indentStr(indent), key, info.name))
            hexDump(value, indent + 1)
        end
    end
end


function dumpObjects(vol, indent)
    for key = 0x00, 0xFF do
        local value = fs:readObject(vol, key)
        if value then
            print(string.format("%sObject<%02x>", indentStr(indent), key))
            hexDump(value, indent + 1)
        end
    end
end


function dumpVolume(obj, indent)
    -- Recursively dump a volume and all of its logical children

    indent = indent or 0

    if obj.typeInfo then
        -- This is a volume

        if obj.typeInfo.hidden then
            return false
        end

        print(string.format("%sVolume<%02x> type=%04x (%s), %d bytes",
            indentStr(indent), obj.vol, obj.type, obj.typeInfo.name,
            #fs:volumePayload(obj.vol)))

        if obj.typeInfo.elf then
            dumpMetadata(obj.vol, indent + 1)
        end

        if obj.lfs then
            dumpObjects(obj.vol, indent + 1)
        end

        for i in pairs(obj.children) do
            dumpVolume(obj.children[i], indent + 1)
        end

    else
        -- This is the root of the filesystem

        if obj.lfs then
            print(string.format("%sSystem Objects", indentStr(indent)))
            dumpObjects(0, indent + 1)
            print ""
        end

        for i in pairs(obj.children) do
            if dumpVolume(obj.children[i], indent) then
                print ""
            end
        end
    end

    return true
end


main()
