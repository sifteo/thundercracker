--[[
    Sifteo Thundercracker firmware unit tests

    M. Elizabeth Scott <beth@sifteo.com> 
    Copyright <c> 2011 Sifteo, Inc. All rights reserved.
]]--

require('scripts/luaunit')
require('scripts/test-graphics')
    
-- You can use environment vars to pass in other options.
-- By default, we run all tests with no GUI.
--
-- In particular, you can set TEST to a string of the form
-- "TestClass:test_name", just like the strings that luainit
-- will print as the tests run.

gx:init(os.getenv("USE_FRONTEND"))
LuaUnit:run(os.getenv("TEST"))
gx:exit()
