--[[
    Lua code specific to the "fault" SDK test.
]]--


F_UNKNOWN                 = 0x00
F_STACK_OVERFLOW          = 0x01
F_BAD_STACK               = 0x02
F_BAD_CODE_ADDRESS        = 0x03
F_BAD_SYSCALL             = 0x04
F_LOAD_ADDRESS            = 0x05
F_STORE_ADDRESS           = 0x06
F_LOAD_ALIGNMENT          = 0x07
F_STORE_ALIGNMENT         = 0x08
F_CODE_FETCH              = 0x09
F_CODE_ALIGNMENT          = 0x0a
F_CPU_SIM                 = 0x0b
F_RESERVED_SVC            = 0x0c
F_RESERVED_ADDROP         = 0x0d
F_ABORT                   = 0x0e
F_LONG_STACK_LOAD         = 0x0f
F_LONG_STACK_STORE        = 0x10
F_PRELOAD_ADDRESS         = 0x11
F_RETURN_FRAME            = 0x12
F_LOG_FETCH               = 0x13
F_SYSCALL_ADDRESS         = 0x14
F_SYSCALL_PARAM           = 0x15
F_SCRIPT_EXCEPTION        = 0x16
F_BAD_VOLUME_HANDLE       = 0x17
F_BAD_ELF_HEADER          = 0x18
F_BAD_ASSET_IMAGE         = 0x19
F_NO_LAUNCHER             = 0x1a
F_SYSCALL_ADDR_ALIGN      = 0x1b
F_BAD_ASSETSLOT           = 0x1c
F_RWDATA_SEG              = 0x1d
F_NOT_RESPONDING          = 0x1e
F_BAD_ASSET_CONFIG        = 0x1f
F_BAD_ASSET_LOADER        = 0x20


function Runtime:onFault(code)
	-- Do not handle ASSERTs or Lua errors
	if code ~= F_ABORT and code ~= F_SCRIPT_EXCEPTION then

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
