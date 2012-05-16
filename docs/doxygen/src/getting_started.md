
Getting Started     {#getting_started}
===============

# Requirements
If you don't have it already, grab the latest version of the SDK at https://www.sifteo.com/developers

Check the requirements for @ref setup_osx, @ref setup_win, or @ref setup_linux to make sure you have everything you need.

# Installation

The Sifteo SDK is distributed as a zip archive, and can be unpacked anywhere on your machine.

![](@ref installation.png)

## What's Inside?
* @b arm-clang is a C/C++ compiler for Sifteo games
* @b slinky is an optimizing linker/code generator for the Sifteo platform
* @b stir is an asset preparation tool that converts your visual and audio assets into the compact formats used on Sifteo Cubes
* @b siftulator is an emulator that allows you to run and test your applications in simulation before running them on hardware
* @b doc contains a copy of the SDK documentation
* @b include contains the SDK headers
* @b examples contains a variety of example projects
* @b sifteo-sdk-shell is a shortcut which starts a new shell with the right environment variables for compiling Sifteo applications with this SDK

The Sifteo SDK doesn't dictate any build tools or IDE that must be used to develop applications. This guide, as well as the examples included with the SDK, will assume you're building with good old Makefiles, but you may configure your IDE of choice to build Sifteo applications as long as it can use the compiler and linker we provide.

# Build & Run
Now we'll build and run __stars__, one of the example applications in the SDK.

First, open the appropriate shell, depending on your platform:
* Windows: `sifteo-sdk-shell.cmd`
* Mac OS X: `sifteo-sdk-shell.command`
* Linux: `sifteo-sdk-shell.sh`

The shell opens to the __sdk/examples__ folder within the SDK distribution. From here, navigate into the __stars__ demo, and run `make` to build the project.

	cd stars
	make

This should create `stars.elf` in the current folder - this is the binary that contains the stars application.

@note The name of the .elf file is specified by the line `APP = stars` in the `Makefile` in the stars folder.

Finally, let's run `stars.elf` in the Siftulator.

	siftulator stars.elf

The simulator should start up and run `stars`, which should look like the following:

![](@ref stars_example_getting_started.png)

Congratulations! You've just run your first Sifteo Cubes application. Try building the other examples in the __examples__ folder, and modifying them to expore the API.

You may also use any of the examples as a template to create an application of your own.
* Copy the entire folder of your example of choice to a new location.
* Rename the folder, and edit the `Makefile` to reference your project's new name and source files.
* run `make` to confirm that your new project builds, and run it in the __Siftulator__

# Other Build Environments

@note If you set up your own build system from scratch, be sure to use the same compiler flags that we provide by default in the Makefiles.
* Compile for a minimal environment, and generate LLVM object files: <pre>-emit-llvm -ffreestanding -nostdinc -msoft-float</pre>
* Disable unavailable C++ features: <pre>-fno-exceptions -fno-threadsafe-statics -fno-rtti -fno-stack-protector</pre>
* Recommended flags for warnings and debugging:<pre>-g -Wall -Werror -Wno-unused -Wno-gnu -Wno-c++11-extensions</pre>
