-- Simple "Hello World" example.
-- Starts the emulator and frontend, and keeps a message posted on the frontend.

print "Hello"

sys = system()
fe = frontend()

sys:setOptions{numCubes=1}

sys:init()
fe:init()
sys:start()

repeat
    fe:postMessage("Hello from Lua!")
until not fe:runFrame()

sys:exit()
fe:exit()

