
Getting Started     {#getting_started}
===============

## Requirements
If you don't have it already, grab the latest version of the SDK at https://www.sifteo.com/developers

Check the requirements for @ref setup_osx and @ref setup_win to make sure you have everything you need.

## Installation

The Sifteo SDK is distributed as a zip archive, and can be unpacked anywhere on your machine.

![](@ref installation.png)

### What's Inside?
* @b arm-clang is a C/C++ compiler for Sifteo games
* @b slinky is an optimizing linker/code generator for the Sifteo platform
* @b stir is an asset preparation tool that converts your visual and audio assets into the compact formats used on Sifteo Cubes
* @b siftulator is an emulator that allows you to run and test your applications in simulation before running them on hardware
* @b doc contains a copy of the SDK documentation
* @b include contains the SDK headers
* @b examples contains a variety of example projects
* @b sifteo-sdk-shell is a shortcut which starts a new shell with the right environment variables for compiling Sifteo applications with this SDK

The Sifteo SDK doesn't dictate any build tools or IDE that must be used to develop applications, but we do ship support in the SDK for building with good old Makefiles. This guide will cover a Makefile-based build setup, but you may configure your IDE of choice.
