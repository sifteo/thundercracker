#!/bin/bash
# SDK Tests Setup Script for Mac OS / Linux

export TC_DIR="`(cd $(dirname $0); cd ../..; pwd)`"
export SDK_DIR="$TC_DIR/sdk"
export PATH="$SDK_DIR/bin:$PATH"

export PS1="[\[\e[1;32m\]Sifteo\[\e[0m\]] \h:\W \u\$ "
export PS2="> "

clear
echo \ \ \_\_\_\ \_\ \ \_\_\ \_\ \ \ \ \ \ \ \ \ \ \ \ \ \_\_\_\ \_\_\_\ \ \_\ \ \_\_
echo \ \/\ \_\_\(\_\)\/\ \_\|\ \|\_\ \_\_\_\ \_\_\_\ \ \/\ \_\_\|\ \ \ \\\|\ \|\/\ \/
echo \ \\\_\_\ \\\ \|\ \ \_\|\ \ \_\/\ \-\_\)\ \_\ \\\ \\\_\_\ \\\ \|\)\ \|\ \'\ \<
echo \ \|\_\_\_\/\_\|\_\|\ \ \\\_\_\\\_\_\_\\\_\_\_\/\ \|\_\_\_\/\_\_\_\/\|\_\|\\\_\\
echo
echo This shell has a PATH, TC_DIR, and SDK_DIR set up for
echo building and running Sifteo SDK unit tests.
echo

cd $TC_DIR/test/sdk
exec bash
