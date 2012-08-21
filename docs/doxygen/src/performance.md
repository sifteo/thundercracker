
Performance   {#performance}
===========

@brief Tips and best practices for performance optimization

# Introduction

The @ref execution_env used by Sifteo Cubes is fairly resource constrained, meaning that most games will at some point have to face performance optimization challenges. If you're used to optimizing C++ code on a small portable platform, many of those skills will serve you well in optimizing Sifteo applications as well. But in many ways this is a very unique platform. This guide covers many of the specific optimization techniques for Sifteo Cubes.

There are four types of common bottlenecks:

- Flash Bandwidth
- CPU
- Graphics
- Radio

Let's cover each one individually.

# Flash Bandwidth

This is a common bottleneck on the Sifteo cubes, so it deserves some detailed attention. Since the the Base has such a tiny execution environment, it must be "fed" code and static data in small chunks from external Flash memory. And of course, the bandwidth used to make these transfers is limited, so it's easy to choke if you are paging lots of code.

The primary tool to reduce the load is a small 16K cache. By keeping your code small enough to fit in this cache most of the time, you can reduce the load on flash bandwidth and make the cubes sing!

The best way to optimize here is adopt a style of code that lends itself to fitting within your cache. These rules all boil down to keeping code _linear_ as much as possible. If we spend the time to page in a block of code, as much of that code as possible should be relevant to the current gameplay. Time spent paging in _dead_ code is wasted time.

So how does one code to maximize the cache?

- Code directly, do not create many layers of runtime indirection.
- Use compile-time abstraction (for example, inline functions) instead of runtime abstraction (i.e. virtual functions)
- Prefer modal functions over highly object-oriented designs.
- Avoid intermediate data formats. Generate static data in the form that the machine accepts it.
- Data that will be referenced very frequently and/or very randomly can be copied to RAM first.
- Don't jump around in code a lot, keep it local.
- Consider decompressing tile data into a Sifteo::TileBuffer or Sifteo::RelocatableTileBuffer and drawing that.
- Think carefully about using lookup tables. Consider whether it may be faster to use a switch statement, or to copy that table to RAM.
- Use masks to quickly iterate over "all entities that do X" as opposed to entering functions that do the check "is entity X" and immediately returning. The Sifteo::BitArray class and its iterator has this operation built-in.

This last point encourages a practice called Data-Oriented Design. In summary, learn to process your updates based on the type of data it uses instead of its logical hierarchy. For example, don't do this:

~~~~~~~~~~~~~~~
for (Entity e : entites)
    if (e.IsRoom())
        e.ProcessRoom();
    else if (e.IsView())
        e.ProcessView();
~~~~~~~~~~~~~~~

Instead, do something like this:

~~~~~~~~~~~~~~~
for (Entity e : ListRooms(entities))
    e.ProcessRoom();
for (Entity e : ListViews(entities))
    e.ProcessView();
~~~~~~~~~~~~~~~

The trick is to build BitArrays that can quickly tell you which entities need which types of updates. The ARM processor in the base has a handy-dandy instruction called CLZ (Count Leading Zeroes) that makes this kind of iteration super fast.

There are two main source of metrics to determine how well you are utilizing the flash cache. Running in siftulator with the --svm-flash-stats option will log cache hits and misses. There are also @ref scripting "Lua hooks" for catching flash misses programmatically.

So what are some techniques to better utilize the cache?

- Write smaller functions.
- Hunt down flash misses, and examine why you are missing.
- Avoid heavy functions that do early-outs. The goal is code linearity!
- Check assembly to see what's really going on.
- Use memcpy() whenever you can. This is the most efficient way to copy data out of flash.
- Write code to model "what the player is doing" instead of going for hierarchical object-oriented design.

# CPU

Most Sifteo games are not CPU bound. Flash bandwidth and graphics performance are much more common sources of slowness. But still, watch out for types of operations that place a large load on the CPU:

- High sample rates in tracker audio
- Poorly-written game logic
- Decompressing data from flash:
- Heavy floating-point math
- Unnecessary memory loads and stores

## Decompression bottlenecks

There are several places where the system may spend CPU time to decompress data from flash:

 - Decompressing AssetImages
 - Decompressing ADPCM audio
 - Invoking other runtime decompression, such as FastLZ1.

Normally these are not bottlenecks, but if they do become too much of a CPU burden, you may be able to selectively disable compression on the problematic data (such as by using FlatAssetImage or uncompressed PCM audio), or you can pre-decompress it (i.e. by storing images in a TileBuffer).

## Floating point math bottlenecks

The Base's CPU has no hardware floating point coprocessor. It is super fast at integer math, even at integer multiplication. But floating point will always be a bit pokey.

For human-timescale operations, such as animation that happens once or a few times per frame, this shouldn't be a problem. But if you have any inner loops or very math-heavy algorithms, consider avoiding floating point in those cases, and using integer math or fixed-point instead.

In particular, math library functions like sin(), cos(), sqrt(), and log() are especially slow. When possible, avoid these operations or pre-compute the values you need.

If you're using trig heavily but you don't need amazing precision, consider the table-driven alternatives: tsin() and tcos(), or their fixed-point counterparts tsini() and tcosi().

Remember, integer math is very fast! Integer math runs at full native speed in SVM, without either the software floating point overhead itself, or the virtualization overhead inherent in calling into the floating point library.

Pro Tip! Use Sifteo::TimeDelta for frame time delta instead of floats. Floats are convenient, but beyond that it's rarely a good idea to use them for time.

## Memory load/store bottlenecks

Due to details of the SVM architecture, some types of memory are vastly speedier than others:

 - General purpose registers are extremely fast
 - Local variables in a non-oversized stack frame are almost as fast
 - Local variables in an oversized stack frame are somewhat slower
 - Any other kind of RAM is a little slower still
 - Uncached flash memory is very slow

The compiler will naturally try to keep things in fast memory whenever possible, but due to the semantics of memory access in C++ it isn't always possible for the compiler to optimize out loads and stores. So it can help the optimizer to keep data in local variables instead of repeatedly accessing class members.

 - Use local variables for temporary space, not class member variables
 - In some cases, it may be faster to memcpy() a data structure to the stack, manipulate it there, then memcpy() it back.

# Graphics

Along with flash bandwidth, graphics performance is the other common bottleneck in Sifteo games. The graphic engine is distributed to maximize performance, but you have to keep in mind what it's doing in order to not get in it's way. 

So what are some techniques to optimize graphics?

- **Eliminate overdraw**. This is really important, and we discuss it separately below.
- Try moving sprites (it's cheap!).  Also BG0 and BG1 panning.
- Do not change masks or tiles every frame.
- Think carefully about what you are asking the graphics pipeline to do, and unnecessary operations like early System::finish().
 - Note that finish calls near the end of the frame often are "free" if the rendering has already completed.

Cubes do not render synchronously, but certain function calls (namely System::finish) can force them to wait and sync up. Some types of rendering require this behavior, but it is also easy to call finish more often than you need to. Also watch out for functions that implicitly call finish like VideoBuffer::initMode and BG1Drawable::setMask. Only call finish when you need it (for example, before some dependent BG1 calls).

## Overdraw

In this case, _overdraw_ refers to any situation where you're drawing data to a Sifteo::VideoBuffer which is different from the final content you intend the VideoBuffer to contain at the end of the frame.

For example, this operation causes overdraw:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Overdraw! AVOID THIS!
vid.bg0.image(vec(0,0), Background);
vid.bg0.image(vec(4,2), Door);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If you were do to this every frame (which itself is probably a bad idea if this screen is not changing) you would be repeatedly marking the Door's tiles as _changed_, causing them to go out over the radio every frame, and the cube to continuously re-draw the screen.

You can tell if this is happening by looking at the frame rate counter under each cube in Siftulator. If it shows a nonzero FPS count even when the scene is idle, you're probably causing overdraw. This will waste radio bandwidth and waste battery power!

Additionally, overdraw can cause __flickering__. If we happen to send the contents of the VideoBuffer over the radio between these two draws, the cube will momentarily show the background but not the door, causing the door to flicker. These types of bugs are often highly timing-dependent, and it's common for them to be visible in hardware but not in simulation, or vice versa.

There are multiple ways to solve overdraw:

- Draw to a Sifteo::TileBuffer or Sifteo::RelocatableTileBuffer as a "back-buffer", then copy that to the VideoBuffer.
- Slice your drawing so that each tile is painted at most once per frame. For example, the above example could draw the Background in four separate slices, with a 'cutout' where the Door will go.
- Invert your rendering operations. Instead of visiting each object from back-to-front and painting those objects, visit each tile of the screen and decide what should be shown at that location. This approach is especially effective when you're implementing a scrolling graphics engine which needs to update arbitrary small slices of the screen.

If you have the memory to spare, double-buffering is usually the easiest way around overdraw. For example:

~~~~~~~~~~~~~~~~~~~~~~~~~~
// Using a TileBuffer to avoid overdraw
TileBuffer<16,16> buffer;
buffer.image(vec(0,0), Background);
buffer.image(vec(4,2), Door);
vid.bg0.image(vec(0,0), buffer);
~~~~~~~~~~~~~~~~~~~~~~~~~~

# Radio

Radio communication between the base and cubes can also be a bottleneck. However, this is rare since the system is designed to minimize radio transmissions. But watch out if you are continuously transmitting data (for example, with non-stop changing of frame buffer mode pixels).

You can take a peek at radio traffic using the siftulator option --radio-trace. The format is not documented, but you can see how many packets are going to what cubes and how full those packets are.

So what can you do to optimize radio traffic?

- Eradicate any overdraw. This is the #1 enemy of radio bandwidth.
- If you're scrolling, make use of panning and modulo arithmetic in order to keep from "moving" large amounts of data around in the VideoBuffer. Keep your tiles put, and use panning to move the layer around.
- It's much more efficient to copy data from nearby in the VideoBuffer than it is to send completely new data, if that's possible. The compression codec will look for copyable tiles above and to the left of tiles that are being encoded. Copies do not need to be exact- we can also copy tiles and add a small delta to them.
- In FB32 mode, use even-numbered color indices primarily. Data words with bit 0 and bit 8 set to zero compress much better in our radio protocol.
