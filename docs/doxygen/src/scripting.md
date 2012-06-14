
Scripting   {#scripting}
=======

# Running Lua code

The Sifteo platform simulator _Siftulator_ includes scripting support, via an embedded [Lua](http://www.lua.org/) interpreter. This scripting support is intended primarily for automated testing, and as a way to interface emulated Sifteo games with the outside world for debugging, testing, and exploration.

Within Siftulator, there are two ways to use Lua scripting.

## Shell mode

You can start Siftulator with the `-e` command line option. This instructs Siftulator to run the specified Lua script, in the main thread, _instead_ of starting the usual simulation environment.

In this mode, the Siftulator binary acts as a very basic shell around the
Lua interpreter. By creating objects like __System()__ and __Frontend()__, the script can then explicitly set up the parts of the simulation that it needs.

This mode is used internally, for unit-testing of Siftulator and the Sifteo Cube hardware model. It may also be suitable for _black box_ testing of Sifteo games which have no debug instrumentation at all. These external tests can inspect the display contents of each cube, and they may read or write any memory in the system. They also have some limited capacity for interacting with the hardware simulation or with the UI Frontend.

Sample shell-mode script:

~~~~~~~~~~~~~{.lua}
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
~~~~~~~~~~~~~

## Inline scripting

In this mode, script fragments are interleaved with normal C++ game code, using some macro, linker, and runtime tricks. It all starts with the SCRIPT()
and SCRIPT_FMT() macros, which allow inline execution of Lua code in your game, with limited ways for game code and Lua code to exchange data.

It is often useful to include out-of-line Lua code via require(). All inline
scripting runs in its own context, distinct from the context used by shell-mode scripting. This single context is shared between all blocks
of inline Lua code. The code is always parsed and run in the order that the
enclosing C++ code runs, so these opening declarations commonly happen at the
top of main().

Sample in-line scripting code:

~~~~~~~~~~~~~{.cpp}
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
~~~~~~~~~~~~~

## Interfacing with the outside world

The Lua [Input / Output library](http://www.lua.org/manual/5.1/manual.html#5.7) is available, for reading or writing files on disk..

Additionally, environment variables (via `os.getenv`) are a convenient way to read parameters at any point, whether you're using shell mode or inline scripting.

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

Built-in extension modules:

- [BitOp Library](http://bitop.luajit.org/api.html)

## System object

This is a singleton object which represents the simulated state of the system, at a high level.

### System()

The constructor for System does not actually allocate any resources in Siftulator, it just allocates a Lua object which can then be used as a proxy for the simulated system state. Takes no parameters.

### System():setOptions{ _key_ = _value_, ... }

Set one or more key-value pairs which describe global simulation options.

Option                  | Meaning
-------                 | -------------
`numCubes`              | Number of cubes to simulate. Also set by the `-n` command line option.
`turbo`                 | Boolean value. If false, the simulation runs as close to real-time as possible. If true, the simulation runs as fast as possible.
`paintTrace`            | Boolean value. If true, dump detailed Paint Controller logs.
`radioTrace`            | Boolean value. If true, log the contents of all radio packets.
`svmTrace`              | Boolean value. If true, log all executed SVM instructions.
`svmFlashStats`         | Boolean value. If true, dump statistics about flash memory usage.
`svmStackMonitor`       | Boolean value. If true, monitor SVM stack usage.

### System():numCubes()

Retrieve the current number of simulated cubes. This value can be set with `System():setOptions{numCubes=N}`, the `-n` command line option, or keyboard commands in the UI.

### System():init()

Initialize the simulation subsystem. This includes the simulated Cubes, radio, and Base. The system must be initialized before most other methods are invoked. Note that this function is only needed when using scripting in _shell mode_. With inline scripting, you're running from within the simulated environment, so it by necessity is already initialized.

### System():start()

Begin running the simulation, on a separate thread pool. Like init(), this is only applicable in _shell mode_. Other modules, such as Frontend, may be initialized between calling init() and start().

### System():exit()

Halt the simulation, and free resources associated with it. Only useful in _shell mode_.

### System():setAssetLoaderBypass( _true_ | _false_ )

Enable or disable _asset loader bypass_ mode. In this mode, all asset downloads will appear to complete instantaneously. Instead of fully simulating the asset download process using Siftulator's hardware-accurate simulation engine, the assets are decompressed using native code and written directly to the Cube's simulated Asset Flash memory.

Note that your game code must still go through the same procedure to install assets; this just reduces the amount of time taken by the install process, making for a quicker dev/test cycle.

### System():vclock()

Return the current _virtual time_, in seconds. This is the elapsed time, from the perspective of the simulated system. If the simulation is running at 50% real-time, for example, this value will increase at a rate of 0.5 virtual seconds per real second.

The virtual clock's resolution is approximately 60 nanoseconds.

### System():vsleep( _seconds_ )

Block the caller for the specified number of seconds, in _virtual time_. This is not an exact delay. It tries to sleep for the minimum amount of time which is greater than or equal to the specified duration. The Lua scripting engine is not precisely synchronized with the simulation engine, however.

### System():sleep( _seconds_ )

Block the caller for a specified number of real wall-clock seconds. This depends on the underlying operating system's sleep primitive, and the accuracy will vary depending on the platform.

## Frontend object

This is a singleton object which represents the graphical frontend to Siftulator. In _shell mode_, the frontend must be explicitly initialized, and your script is responsible for running the frontend's main loop. With _inline_ scripting, the frontend is run automatically on a separate thread.

### Frontend()

The constructor for Frontend does not actually allocate any resources in Siftulator, it just allocates a Lua object which can then be used as a proxy for the grapical frontend state. Takes no parameters.

### Frontend():init()

Set up the frontend. Creates a window, allocates GPU resources, etc. If any of this fails, init() will throw a Lua error.

### Frontend():exit()

Terminate the graphical frontend. Closes the window, frees textures, and so on.

### Frontend():runFrame()

Run the frontend's main loop, for a single frame. This automatically includes a frame-rate throttling feature, which dynamically decreases the frame rate during periods of no interaction.

Returns _true_ if the frontend should still be running or _false_ if the user has asked Siftulator to exit.

### Frontend():postMessage( _string_ )

Post a message string to the frontend's heads-up display. A posted message will display for a few seconds before automatically clearing. Posting a new message will instantly replace the previously posted message.

This method is suitable for either transient messages, or for information displays that stay visible for long periods of time.

## Cube object

The Cube object is an accessor for a single simulated Sifteo Cube. This is a lightweight Lua object which acts as a proxy for the internal object which simulates a single cube.

### Cube( _number_ )

The constructor for Cube does not actually allocate any resources in Siftulator, it just allocates a Lua object and binds it to a particular cube ID. Cube IDs are zero-based.

### Cube(N):reset()

Reset the state of this cube's simulation. Equivalent to removing and reinserting the cube's batteries.

### Cube(N):lcdFrameCount()

Read this cube's LCD frame counter. Every time the cube hardware finishes drawing one full frame, this counter increments. It is a 32-bit unsigned integer, which will wrap around.

When comparing two frame counts, the easiest way to handle wrap-around is to ensure that you're using 32-bit arithmetic as well. For example:

~~~~~~~~~~~~~~~{.lua}
local c = Cube(0)
local count = c:lcdFrameCount()
local framesRendered = bit.band(count - lastCount, 0xFFFFFFFF)
lastCount = count
~~~~~~~~~~~~~~~

### Cube(N):lcdPixelCount()

Read this cube's LCD pixel counter. This is much like lcdFrameCount(), except instead of incrementing once per completed frame, it increments every time a pixel is written to the display hardware. The pixel count will change continuously while a frame is being rendered.

This is also an unsigned 32-bit integer. Note that integer wraparound could occur in as little as 1 hour.

### Cube(N):saveScreenshot( _filename_ )

Save a screenshot of this cube, to a 128x128 pixel PNG file with the given name.

### Cube(N):testScreenshot( _filename_ )

Capture a screenshot of this cube, and compare it to an existing 128x128 pixel PNG file with the given name.

If the images match, returns nothing. If there was an error opening the reference image file, raises a Lua error.

If the reference image was loaded successfully but there is even a slight difference in any pixel as compared to the current screen contents, this function returns four parameters describing the first pixel mismatch:

Position    | Name      | Meaning
--------    | ----      | ----------------------------------------------------
1           | x         | Zero-based X coordinate
2           | y         | Zero-based Y coordinate
3           | lcdPixel  | Actual pixel on the LCD, as a 16-bit RGB565 value
4           | refPixel  | Reference pixel from the provided PNG, after conversion to 16-bit RGB565 format

### Cube(N):xbPoke( _address_, _byte_ )

Write one byte to the cube's Video RAM, at the specified byte address. Byte addresses must be in the range [0, 1023]. Out-of-range addresses will wrap around.

### Cube(N):xwPoke( _address_, _word_ )

Write one 16-bit word to the cube's Video RAM, at the specified word address. Word addresses must be in the range [0, 511]. Out-of-range addresses will wrap around.

### Cube(N):xbPeek( _address_ )

Read one byte from the cube's Video RAM, at the specified byte address. Byte addresses must be in the range [0, 1023]. Out-of-range addresses will wrap around.

### Cube(N):xwPeek( _address_ )

Read one 16-bit word to the cube's Video RAM, at the specified word address. Word addresses must be in the range [0, 511]. Out-of-range addresses will wrap around.

### Cube(N):fbPoke( _address_, _byte_ )

Write one byte to the cube's Asset Flash memory, at the specified byte address.

### Cube(N):fwPoke( _address_, _word_ )

Write one 16-bit word to the cube's Asset Flash memory, at the specified word address.

### Cube(N):fbPeek( _address_ )

Read one byte from the cube's Asset Flash memory, at the specified byte address.

### Cube(N):fwPeek( _address_ )

Read one 16-bit word to the cube's Asset Flash memory, at the specified word address.

## Runtime object

This is a singleton object which represents the simulated state of the @ref execution_env.

### Runtime()

The constructor for Runtime does not actually allocate any resources in Siftulator, it just allocates a Lua object which can then be used as a proxy for the simulated runtime state. Takes no parameters.

### Runtime():poke( _address_, _word_ )

Write a 32-bit word into RAM, at the specified virtual address. When using _inline_ scripting, this address is equivalent to the value of a C++ pointer to integer or unsigned integer.

If the virtual address is invalid, raises a Lua error.

## Runtime():virtToFlashAddr( _virtual address_ )

Convert an SVM virtual address to a physical Flash memory address, using the currently active mappings. If this VA is not mapped, returns zero.

## Runtime():flashToVirtAddr( _flash address_ )

Convert a physical Flash memory address to an SVM virtual address. If the supplied flash address is not part of any virtual address space, returns zero.

## Filesystem object

This is a singleton object which can be used to script the Base's filesystem.

This is the same filesystem which can be accessed with the SDK's @ref filesystem objects. By scripting the filesystem, you can automatically test portions of your game which depend on persistent data storage.

### Filesystem()

The constructor for Filesystem does not actually allocate any resources in Siftulator, it just allocates a Lua object which can then be used as a proxy for the filesystem layer in the Base's operating system. Takes no parameters.

### Filesystem():newVolume( _type_, _payload data_, _type-specific data_ = "", _parent_ = 0 )

Creates, writes, and commits a new Sifteo::Volume in the filesystem. The specified _type_ is the same 16-bit type code used by the Sifteo::Volume::Type enumeration. _Payload data_ is a string with binary data to load the Volume with.

The _type-specific data_ parameter is an optional binary string, used by some volume types. Omitting this parameter is equivalent to passing in the empty string.

The _parent_ parameter is a block code for another volume that is hierarchically above this new volume. If the parent volume is deleted, this volume (and its children, if any) are also deleted.

On success, returns the volume's block code. This is an opaque identifier which is indirectly related to the identity of a Sifteo::Volume object. It is guaranteed to be nonzero.

On error (out of filesystem space), returns no results.

### Filesystem():listVolumes()

List all volumes on the filesystem. Returns an array of block codes.

### Filesystem():deleteVolume( _block code_ )

Mark a volume as deleted. If the provided block code is not valid, raises a Lua error.

### Filesystem():volumeType( _block code_ )

Given a volume's block code, return the associated 16-bit type identifier. If the provided block code is not valid, raises a Lua error.

### Filesystem():simulatedSectorEraseCounts()

Return an array of simulated erase counts for every sector in the Base's flash memory. This data is tracked by Siftulator's simulation engine itself.

This can be used to measure how much flash wear and tear is being caused by a particular test.

If Siftulator is running with persistent flash storage (`-F` command line option), the erase counts are also persisted in the same file.

### Filesystem():rawRead( _address_, _count_ )

Read _count_ bytes from the raw Flash device, starting at the specified device address. Returns the data as a string.

### Filesystem():rawWrite( _address_, _data_ )

Write the string _data_ to the raw Flash device, starting at _address_. Does not automatically erase the device. This effectively performs a bitwise AND of the supplied data with the existing contents of the device.

### Filesystem():rawErase( _address_ )

Erase one 4 kB sector of the raw Flash device, starting at the specified sector-aligned address.

### Filesystem():invalidateCache()

Force the system to discard or reload all cached Flash data. Any memory blocks which are still in use will be overwritten with freshly-loaded data, while unused cached memory blocks are simply discarded.

### Filesystem():setCallbacksEnabled( _true_ | _false_ )

This enables a set of Lua callbacks which fire on any low-level access to flash memory. These can be used for low-level tracing and debugging, or for collecting metrics on how memory is being used during a particular operation.

By default these callbacks are disabled for performance reasons. When enabled, the system will call onRawRead(), onRawWrite(), and onRawErase() during the corresponding low-level events. These functions all have default implementations which do nothing. They are meant to be overridden by user code.

For example:

    function Filesystem:onRawRead(addr, data)
        print(string.format("Read %08x, %d bytes", addr, string.len(data)))
    end

    function Filesystem:onRawWrite(addr, data)
        print(string.format("Write %08x, %d bytes", addr, string.len(data)))
    end

    function Filesystem:onRawErase(addr)
        print(string.format("Erase %08x", addr))
    end

    fs = Filesystem()
    fs:setCallbacksEnabled(true)

This overrides the three callback methods on Filesystem, then enables those callbacks. Any Flash memory operations after this point will be accompanied by logging.

The addresses supplied are physical flash addresses. Where applicable, you can translate them back to virtual addresses (a.k.a C++ pointers) using `Runtime():flashToVirtAddr()`.
