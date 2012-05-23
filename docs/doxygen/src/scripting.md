
Scripting   {#scripting}
=======

# Running Lua code

The Sifteo platform simulator _Siftulator_ includes scripting support, via an embedded [Lua](http://www.lua.org/) interpreter. This scripting support is intended primarily for automated testing, and as a way to interface emulated Sifteo games with the outside world for debugging, testing, and exploration.

Within Siftulator, there are two ways to use Lua scripting.

## Shell mode

You can start Siftulator with the `-e` command line option. This instructs Siftulator to run the specified Lua script, in the main thread, _instead_ of starting the usual simulation environment.

In this mode, the Siftulator binary acts as a very basic shell around the
Lua interpreter. By creating objects like System() and Frontend(), the script
can then explicitly set up the parts of the simulation that it needs.

This mode is used internally, for unit-testing of Siftulator and the Sifteo Cube hardware model. It may also be suitable for _black box_ testing of Sifteo games which have no debug instrumentation at all. These external tests can
inspect the display contents of each cube, and they may read or write any memory in the system. They also have some limited capacity for interacting with the hardware simulation or with the UI Frontend.

Sample shell-mode script:

    -- Lua script file

    print "Hello"

    sys = System()
    fe = Frontend()
    c = Cube(0)

    sys:setOptions{numCubes=1}

    sys:init()
    fe:init()
    sys:start()

    print(string.format("Radio address: %s", c:getRadioAddress()))

    repeat
        fe:postMessage(string.format("Hello from Lua! (%.2f sec)",          
            sys:vclock()))
        print(string.format("LCD: %10d pixels, %10d frames",
            c:lcdPixelCount(), c:lcdFrameCount()))
    until not fe:runFrame()

    sys:exit()
    fe:exit()

## Inline scripting

In this mode, script fragments are interleaved with normal C++ game code, using some macro, linker, and runtime tricks. It all starts with the SCRIPT()
and SCRIPT_FMT() macros, which allow inline execution of Lua code your game, with limited ways for game code and Lua code to exchange data.

It is often useful to include out-of-line Lua code via require(). All inline
scripting runs in its own context, distinct from the context used by shell-mode scripting. This single contex is shared between all blocks
of inline Lua code. The code is always parsed and run in the order that the
enclosing C++ code runs, so these opening declarations commonly happen at the
top of main().

Sample in-line scripting code:

    // Example fragments of inline scripting from C++

    SCRIPT(LUA,
        package.path = package.path .. ";scripts/?.lua"
        require('my-test-library')
    );

    for (unsigned i = 0; i < 10; ++i)
        SCRIPT(LUA, invokeTest());

    SCRIPT(LUA, System():setAssetLoaderBypass(true));
    SCRIPT(LUA, Cube(0):saveScreenshot("myScreenshot.png"));
    SCRIPT(LUA, Cube(0):testScreenshot("myScreenshot.png"));

    SCRIPT_FMT(LUA, "Frontend():postMessage('Power is >= %d')", 9000);

    int luaGetInteger(const char *expr) {
        int result;
        SCRIPT_FMT(LUA, "Runtime():poke(%p, %s)", &result, expr);
        return result;
    }
 
    void luaSetInteger(const char *varName, int value) {
        SCRIPT_FMT(LUA, "%s = %d", varName, value);
    }

# Execution Environment

## Standard Lua packages

The Lua interpreter is based on [Lua 5.1](http://www.lua.org/manual/5.1/). 

Standard Lua libraries:

- [Basic Functions](http://www.lua.org/manual/5.1/manual.html#5.1)
- [Modules](http://www.lua.org/manual/5.1/manual.html#5.3)
- [Table Manipulation](http://www.lua.org/manual/5.1/manual.html#5.5)
- [Input and Output Facilities](http://www.lua.org/manual/5.1/manual.html#5.7)
- [Operating System Facilities](http://www.lua.org/manual/5.1/manual.html#5.8)
- [String manipulation](http://www.lua.org/manual/5.1/manual.html#5.4)
- [Mathematical functions](http://www.lua.org/manual/5.1/manual.html#5.6)
- [Debug Library](http://www.lua.org/manual/5.1/manual.html#5.9)
- [BitOp Library](http://bitop.luajit.org/api.html)

## System object

This is a singleton object which represents the simulated state of the system, at a high level.

- init
- start
- exit
- setOptions
- setTraceMode
- setAssetLoaderBypass
- vclock
- vsleep
- sleep

## Frontend object

- init
- runFrame
- exit
- postMessage

## Cube object

- reset
- isDebugging
- lcdFrameCount
- lcdPixelCount
- exceptionCount
- getRadioAddress
- handleRadioPacket
- saveScreenshot
- testScreenshot
- testSetEnabled
- testGetACK
- testWriteVRAM
- xbPoke
- xwPoke
- xbPeek
- xwPeek
- ibPoke
- ibPeek
- fwPoke
- fbPoke
- fwPeek
- fbPeek

## Runtime object

- poke

## Filesystem object

- newVolume
- listVolumes
- deleteVolume
- volumeType
- volumeMap
- volumeEraseCounts
- simulatedSectorEraseCounts
