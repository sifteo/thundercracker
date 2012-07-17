@echo off
rem SDK Tests Setup Script for Windows

set TC_DIR=%~dp0\..\..
set SDK_DIR=%TC_DIR%\sdk
set PATH=%SDK_DIR%\bin;%PATH%

echo ^ ^ ^_^_^_^ ^_^ ^ ^_^_^ ^_^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^_^_^_^ ^_^_^_^ ^ ^_^ ^ ^_^_
echo ^ ^/^ ^_^_^(^_^)^/^ ^_^|^ ^|^_^ ^_^_^_^ ^_^_^_^ ^ ^/^ ^_^_^|^ ^ ^ ^\^|^ ^|^/^ ^/
echo ^ ^\^_^_^ ^\^ ^|^ ^ ^_^|^ ^ ^_^/^ ^-^_^)^ ^_^ ^\^ ^\^_^_^ ^\^ ^|^)^ ^|^ ^'^ ^<
echo ^ ^|^_^_^_^/^_^|^_^|^ ^ ^\^_^_^\^_^_^_^\^_^_^_^/^ ^|^_^_^_^/^_^_^_^/^|^_^|^\^_^\
echo.
echo This shell has a PATH, TC_DIR, and SDK_DIR set up for
echo building and running Sifteo SDK unit tests.
echo.

cd %TC_DIR%\test\sdk
cmd