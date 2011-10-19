-- Simple "Hello World" example.
-- Starts the emulator and frontend, and keeps a message posted on the frontend.

print "Hello"

sys = system()
fe = frontend()

sys:setOptions{numCubes=1}

sys:init()
fe:init()
sys:start()

counter = 0

repeat
    fe:postMessage("Hello from Lua! Frame #" .. counter)
    counter = counter + 1
until not fe:runFrame()

sys:exit()
fe:exit()
