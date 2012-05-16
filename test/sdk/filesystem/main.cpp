/*
 * Unit tests for the firmware's flash filesystem.
 */

#include <sifteo/macros.h>

void main()
{
    // Initialization
    SCRIPT(LUA,
        package.path = package.path .. ";../../lib/?.lua"
        require('test-filesystem')
    );

    // Run all of the pure Lua tests (no API exercise needed)
    SCRIPT(LUA, testFilesystem());

    // To do: Test the SDK's filesystem APIs once they're available.
}
