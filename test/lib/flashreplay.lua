--[[
    Flash memory logging / replay engine.

    M. Elizabeth Scott <beth@sifteo.com> 
    Copyright <c> 2012 Sifteo, Inc. All rights reserved.
]]--


function toHex(s)
    return s:gsub("(.)", function (c) return string.format("%02x", c:byte()) end)
end

function fromHex(s)
    return s:gsub("(..)", function (c) return string.char("0x"..c) end)
end

function saveFlashSnapshot(fs, filename)
    local f = io.open(filename, 'wb')    
    f:write(fs:rawRead(0, 0x1000000))
    f:close()
end

function loadFlashSnapshot(fs, filename)
    local f = io.open(filename, 'rb')
    local data = f:read(0x1000000)

    for i = 0, 0xff0000, 0x10000 do
        fs:rawErase(i)
    end
    fs:rawWrite(0, data)
    fs:invalidateCache()

    if fs:rawRead(0, #data) ~= data then
        saveFlashSnapshot(fs, "error.bin")
        error(string.format("Failed to restore flash data from '%s'. Actual data in 'error.bin'", filename))
    end
end


FlashLogger = {}

    function FlashLogger:start(fs, filename)

        function Filesystem:onRawRead(addr, data)
            -- Nothing to do
        end

        function Filesystem:onRawWrite(addr, data)
            local logger = getmetatable(self).logger
            logger.file:write(string.format("w %06x %s\n", addr, toHex(data)))
        end

        function Filesystem:onRawErase(addr)
            local logger = getmetatable(self).logger
            logger.file:write(string.format("e %06x\n", addr))
        end

        local o = { file=io.open(filename, 'w'), fs=fs }
        setmetatable(o, self)
        self.__index = self

        getmetatable(fs).logger = o
        fs:setCallbacksEnabled(true)

        return o
    end

    function FlashLogger:stop()
        self.fs:setCallbacksEnabled(false)
        self.file:close()
    end


FlashReplay = {}

    function FlashReplay:start(fs, filename)
        local o = { file=io.open(filename, 'r'), fs=fs }
        setmetatable(o, self)
        self.__index = self
        return o
    end

    function FlashReplay:stop()
        self.file:close()
    end

    function FlashReplay:play(numEvents)
        local result = 0

        while result < numEvents do
            local line = self.file:read()
            if not line then
                break
            end

            result = result + 1
            op = line:sub(1,1)
            addr = "0x" .. line:sub(3,9)

            if op == 'e' then
                self.fs:rawErase(addr)
            end

            if op == 'w' then
                self.fs:rawWrite(addr, fromHex(line:sub(10)))
            end
        end

        self.fs:invalidateCache()
        return result
    end
