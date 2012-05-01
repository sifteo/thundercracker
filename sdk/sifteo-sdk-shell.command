#!/bin/bash
# SDK Setup Script for Mac OS

export SDK_DIR="`(cd $(dirname \"$0\"); pwd)`"
export PATH="$SDK_DIR/bin:$PATH"

export PS1="[\[\e[1;32m\]Sifteo\[\e[0m\]] \h:\W \u\$ "
export PS2="> "

clear
echo \ \ \_\_\_\ \_\ \ \_\_\ \_\ \ \ \ \ \ \ \ \ \ \ \ \ \_\_\_\ \_\_\_\ \ \_\ \ \_\_
echo \ \/\ \_\_\(\_\)\/\ \_\|\ \|\_\ \_\_\_\ \_\_\_\ \ \/\ \_\_\|\ \ \ \\\|\ \|\/\ \/
echo \ \\\_\_\ \\\ \|\ \ \_\|\ \ \_\/\ \-\_\)\ \_\ \\\ \\\_\_\ \\\ \|\)\ \|\ \'\ \<
echo \ \|\_\_\_\/\_\|\_\|\ \ \\\_\_\\\_\_\_\\\_\_\_\/\ \|\_\_\_\/\_\_\_\/\|\_\|\\\_\\
echo
echo This shell has a PATH and SDK_DIR set up for building
echo Sifteo games using the SDK. This directory contains
echo example projects.
echo

cd $SDK_DIR/examples
exec bash
