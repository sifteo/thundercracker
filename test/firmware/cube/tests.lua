--[[
    Sifteo Thundercracker firmware unit tests

    M. Elizabeth Scott <beth@sifteo.com> 
    Copyright <c> 2012 Sifteo, Inc.
   
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.
]]--

package.path = package.path .. ";../../lib/?.lua"

-- Libraries
require('luaunit')
require('siftulator')
require('vram')

-- Test code
require('test-graphics')
require('test-radio')
require('test-testjig')

--[[
    XXX: There are some big gaps in our testing coverage...
    
    * Radio protocol decoder
    * Flash loadstream decoder
    * Sensors
    * Power management?
    * Recovery from hardware faults, e.g. I2C lockups, bad Flash byte
    
    Also, would be nice to have automated code coverage analysis.
    The emulator's profiling features could be (ab)used to give
    per-instruction or per-basic-block coverage stats.

    Also also, would be good to have automated performance testing,
    to catch any performance regressions early, and to quantify
    any gains we get from optimization.
]]--
        
--[[
    You can use environment vars to pass in other options.
    By default, we run all tests with no GUI.

    In particular, you can set TEST to a string of the form
    "TestClass:test_name", just like the strings that luainit
    will print as the tests run. It can also be a space-separated
    list of such strings.
]]--

tests = {}
for k,v in string.gmatch(os.getenv("TEST") or "", "[^%s]+") do
    tests[1+#tests] = k
end

gx:init(os.getenv("USE_FRONTEND"))
failures = LuaUnit:run(tests)
gx:exit()

if failures > 0 then
    -- Exit with an error code
    error("Some of the tests failed!")
end
