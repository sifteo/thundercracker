@echo off
rem SDK Setup Script for Windows

set SDK_DIR=%~dp0
set PATH=%SDK_DIR%\bin;%PATH%

echo ^ ^ ^_^_^_^ ^_^ ^ ^_^_^ ^_^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^_^_^_^ ^_^_^_^ ^ ^_^ ^ ^_^_
echo ^ ^/^ ^_^_^(^_^)^/^ ^_^|^ ^|^_^ ^_^_^_^ ^_^_^_^ ^ ^/^ ^_^_^|^ ^ ^ ^\^|^ ^|^/^ ^/
echo ^ ^\^_^_^ ^\^ ^|^ ^ ^_^|^ ^ ^_^/^ ^-^_^)^ ^_^ ^\^ ^\^_^_^ ^\^ ^|^)^ ^|^ ^'^ ^<
echo ^ ^|^_^_^_^/^_^|^_^|^ ^ ^\^_^_^\^_^_^_^\^_^_^_^/^ ^|^_^_^_^/^_^_^_^/^|^_^|^\^_^\
echo.
echo This shell has a PATH and SDK_DIR set up for building
echo Sifteo games using the SDK. This directory contains
echo example projects.
echo.

cd %SDK_DIR%\examples
cmd