-- Simple "Hello World" example.
-- Starts the emulator and frontend, and keeps a message posted on the frontend.

print "Hello"

sys = System()
fe = Frontend()
c = Cube(0)

sys:setOptions{numCubes=1}

sys:init()
fe:init()
sys:start()

repeat
    fe:postMessage(string.format("Hello from Lua! (%.2f sec)", sys:vclock()))
    print(string.format("LCD: %10d pixels, %10d writes", c:lcdPixelCount(), c:lcdWriteCount()))
until not fe:runFrame()

sys:exit()
fe:exit()
