
validList = [

# Thumb encoding that are always allowed:
'00xxxxxx xxxxxxxx',  # Shift, add, subtract, mov, cmp
'010000xx xxxxxxxx',  # Data processing (r0-r7)
'1001xxxx xxxxxxxx',  # Load/store [SP, #imm8]
'10110010 xxxxxxxx',  # Sign/zero extend
'10111010 0xxxxxxx',  # REV / REV16
'10111010 11xxxxxx',  # REVSH

# Encodings we may allow as hypercalls / pseudo-ops:
'10111110 xxxxxxxx',  # Breakpoint
'11011110 xxxxxxxx',  # Undefined instruction
'11011111 xxxxxxxx',  # Supervisor call

# Encodings that are allowed pending static bounds checks:
'01001xxx xxxxxxxx',  # Load literal
'1011x0x1 xxxxxxxx',  # CBZ / CBNZ
'1101xxxx xxxxxxxx',  # Conditional branch
'11100xxx xxxxxxxx',  # Unconditional branch
]	

import re, struct, os

def toBinary(x):
	return ''.join(["01"[(x >> i) & 1] for i in range(15, -1, -1)]) 

def isValid(b):
	for pattern in validList:
		if re.match(pattern.replace('x','.').replace(' ',''), b):
			return True

fValid = open("valid.bin", 'wb')
fInvalid = open("invalid.bin", 'wb')

for i in range(0x10000):
	h = struct.pack("H", i)
	if isValid(toBinary(i)):
		fValid.write(h)
	else:
		fInvalid.write(h)

fValid.close()
fInvalid.close()

for fn in 'valid', 'invalid':
	os.system("arm-none-eabi-objdump --disassembler-options=force-thumb "
		"-D -b binary -m armv5t %s.bin > %s.s" % (fn, fn))
