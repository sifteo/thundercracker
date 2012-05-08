
Graphics Engine       {#gfx}
===============

# Overview

Sifteo cubes have a fairly unique graphics architecture. This section introduces the main differences you'll notice between Sifteo cubes and other systems you may be familiar with. We assume at least some basic familiarity with computer architecture and computer graphics.

## Distributed Rendering

Unlike most game systems, the display is not directly attached to the hardware your application runs on. For this reason, Sifteo cubes use a *distributed rendering* architecture:

![](@ref distributed-rendering.png)

Each cube has its own graphics engine hardware, display hardware, and storage. Cubes have two types of local memory:

* __Asset Flash__
   - It is relatively large (several megabytes).
   - It is very fast to draw from, but very slow to rewrite.
   - It contains uncompressed 16-bit pixel data, arranged in 8x8-pixel tiles.

* __Video RAM__
   - It is tiny (1 kilobyte).
   - It is fast both to read and to write.
   - It contains metadata and commands, typically no pixel data.
   - It orchestrates the process of drawing a scene or part of a scene to the display.
   - Applications keep a shadow copy of this memory in a Sifteo::VideoBuffer object.
   - The system continuously synchronizes each cube's Video RAM with any correspondingly attached Sifteo::VideoBuffer.

## Concurrency

This distributed architecture implies that many rendering operations are inherently **asynchronous**. The system synchronizes Video RAM with your Sifteo::VideoBuffer, the cube's graphics engine draws to the display, and the display refreshes its pixels. All of these processes run concurrently, and "in the background" with respect to the game code you write.

The system can automatically optimize performance by overlapping many common operations. Typically, the cube's graphics engine runs concurrently with a game's code, and when you're drawing to multiple cubes, those cubes can all update their displays concurrently.

In the SDK, this concurrency is managed at a very high level with the Sifteo::System::paint() and Sifteo::System::finish() API calls. In short, *paint* asks for some rendering to begin, and *finish* waits for it to complete. Unless your application has a specific reason to wait for the rendering to complete, however, it is not necessary to call *finish*.

Sifteo::System::paint() is backed by an internal *paint controller* which does a lot of work so you don't have to. Frame rate is automatically throttled. Even though each cube's graphics engine may internally be rendering at a different rate (as shown by the FPS display in Siftulator) your application sees a single frame rate, as measured by the rate at which *paint* calls complete. This rate will never be higher than 60 FPS or lower than 10 FPS, and usually it will match the frame rate of the slowest cube.

This is a simplified example timeline, showing one possible way that asynchronous rendering could occur:

![](@ref concurrency.png)

1. The system starts out idle
2. Game logic runs, and prepares the first frame for rendering.
3. Game code calls Sifteo::System::paint()
4. This begins asynchronously synchronizing each cube's Video RAM with the corresponding Sifteo::VideoBuffer. In the mean time, game logic begins working on the next frame.
5. After each cube receives its Sifteo::VideoBuffer changes, it begins rendering. This process uses data from Video RAM and from Asset Flash to compose a scene on the Display.
6. In this example, the Game Logic completes before the cubes are finished rendering. The system limits how far ahead the application can get, by blocking it until the previous frame is complete.
7. When all cubes finish rendering, the application is unblocked, and we begin painting the second frame. In this example, there were no changes to Cube 1's display on the second frame, so it remains idle.
8. Likewise, the third frame begins rendering on Cube 1 once the second frame is finished. In this example, the third frame *only* includes changes to Cube 1.

Note that reality is a little messier than this, since the system tries really hard to avoid blocking any one component unless it's absolutely necessary, and some latency is involved in communicating between the system and each cube. Because of this, no guarantees are made about exactly when Sifteo::System::paint() returns, only about the average rate at which *paint* calls complete. If you need hard guarantees that synchronization and rendering have finished on every cube, use Sifteo::System::finish().

## Graphics Modes

Due to the very limited amount of Video RAM available, the graphics engine defines several distinct *modes*, which each define a different behavior for the engine and potentially a different layout for the contents of Video RAM. These modes include different combinations of tiled layers and sprites, as well as low-resolution frame-buffer modes and a few additional special-purpose modes.

Each of these modes, however, fits into a consistent overall rendering framework that provides a few constants that you can rely on no matter which video mode you use:

![](@ref gfx-modes.png)

* __Paint control__ happens in the same way regardless of the active video mode. Some of the Video RAM is used for paint control.

* __Windowing__ is a feature in which only a portion of the display is actually repainted. The mode renderers operate on one horizontal scanline of pixels at a time, so even when windowing is in use the entire horizontal width of the display must be redrawn. Windowing can be used to create letterbox effects, to render status bars, dialogue, etc.

* __Rotation__ by a multiple of 90-degrees can be enabled as the very last step in the graphics pipeline, after all mode-specific drawing, and after windowing.

These effects can be composed over the course of multiple paint/finish operations. For example:

![](@ref rotated-windowing.png)
