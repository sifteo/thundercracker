--[[
    Lua code specific to the "fault" SDK test.
]]--

function Runtime:onFault(code)
	-- Do not handle ASSERTs or Lua errors
	if code ~= 0xe and code ~= 0x16 then

		if rt:getFP() == 0 then
			error("Should not be handling faults while still in main()")
		end

		lastFault = code
	    rt:branch(pReturnFunc)
	    return true
	end
end

function fmtFault(code)
	if code == nil then
		return 'nil'
	end
	return string.format("0x%02x (%s)", code, Runtime():faultString(code))
end

function assertFault(code)
	if code ~= lastFault then
		error("Failed fault assertion. Expected " .. fmtFault(code) .. ", actual " .. fmtFault(lastFault))
	end
	lastFault = nil
end

lastFault = nil
rt = Runtime()