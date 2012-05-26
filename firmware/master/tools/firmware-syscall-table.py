#!/usr/bin/env python
#
# Firmware syscall table generator.
#
# Reads "abi.h" from stdin, produces a function pointer table on stdout,
# for use in the SVM runtime implementation.
#
# M. Elizabeth Scott <beth@sifteo.com>
# Copyright <c> 2012 Sifteo, Inc. All rights reserved.
#

import sys, re

#######################################################################

#
# Floating point library aliases.
#
# Our floating point syscalls intentionally have the same
# signature as the corresponding EABI or libgcc soft floating
# point functions. GCC is particularly terrible at realizing that
# our syscall wrappers are actually the same thing, and it
# generates a ton of redundant loads and stores.
#
# So, to guarantee the best floating point performance, we
# directly alias these syscalls to the corresponding library
# functions.
#
# Note that we apparently can't do this in the linker script
# for functions which otherwise would be garbage collected.
# Implementing the aliasing with generated C++ code ensures
# that the library functions are linked properly.
#

fpAliasTable = {
    '_SYS_add_f32': '__addsf3',
    '_SYS_add_f64': '__adddf3',
    '_SYS_sub_f32': '__aeabi_fsub',
    '_SYS_sub_f64': '__aeabi_dsub',
    '_SYS_mul_f32': '__aeabi_fmul',
    '_SYS_mul_f64': '__aeabi_dmul',
    '_SYS_div_f32': '__aeabi_fdiv',
    '_SYS_div_f64': '__aeabi_ddiv',
    '_SYS_fpext_f32_f64': '__aeabi_f2d',
    '_SYS_fpround_f64_f32': '__aeabi_d2f',
    '_SYS_fptosint_f32_i32': '__aeabi_f2iz',
    '_SYS_fptosint_f32_i64': '__aeabi_f2lz',
    '_SYS_fptosint_f64_i32': '__aeabi_d2iz',
    '_SYS_fptosint_f64_i64': '__aeabi_d2lz',
    '_SYS_fptouint_f32_i32': '__aeabi_f2uiz',
    '_SYS_fptouint_f32_i64': '__aeabi_f2ulz',
    '_SYS_fptouint_f64_i32': '__aeabi_d2uiz',
    '_SYS_fptouint_f64_i64': '__aeabi_d2ulz',
    '_SYS_sinttofp_i32_f32': '__aeabi_i2f',
    '_SYS_sinttofp_i32_f64': '__aeabi_i2d',
    '_SYS_sinttofp_i64_f32': '__aeabi_l2f',
    '_SYS_sinttofp_i64_f64': '__aeabi_l2d',
    '_SYS_uinttofp_i32_f32': '__aeabi_ui2f',
    '_SYS_uinttofp_i32_f64': '__aeabi_ui2d',
    '_SYS_uinttofp_i64_f32': '__aeabi_ul2f',
    '_SYS_uinttofp_i64_f64': '__aeabi_ul2d',
    '_SYS_eq_f32': '__aeabi_fcmpeq',
    '_SYS_eq_f64': '__aeabi_dcmpeq',
    '_SYS_lt_f32': '__aeabi_fcmplt',
    '_SYS_lt_f64': '__aeabi_dcmplt',
    '_SYS_le_f32': '__aeabi_fcmple',
    '_SYS_le_f64': '__aeabi_dcmple',
    '_SYS_ge_f32': '__aeabi_fcmpge',
    '_SYS_ge_f64': '__aeabi_dcmpge',
    '_SYS_gt_f32': '__aeabi_fcmpgt',
    '_SYS_gt_f64': '__aeabi_dcmpgt',
    '_SYS_un_f32': '__unordsf2',
    '_SYS_un_f64': '__unorddf2',
    '_SYS_fmodf': 'fmodf',
    '_SYS_sqrtf': 'sqrtf',
    '_SYS_logf': 'logf',
    '_SYS_fmod': 'fmod',
    '_SYS_sqrt': 'sqrt',
    '_SYS_logd': 'log',
}


#######################################################################

regex = re.compile(r"^.*(_SYS_\w+)\s*\(.*\)\s+_SC\((\d+)\)");
fallback = re.compile(r"_SC\((\d+)\)");
highestNum = 0
callMap = {}
typedef = "(SvmSyscall)"

#
# Scan the header for system calls
#

for line in sys.stdin:
    m = regex.match(line)
    if m:
        name, num = m.group(1), int(m.group(2))
        highestNum = max(highestNum, num)
        if num in callMap:
            raise Exception("Duplicate syscall #%d" % num)
        callMap[num] = name
    
    elif fallback.search(line):
        raise Exception("Regex might have missed a syscall on line: %r" % line);

#
# Generate extern declarations for all internal libgcc functions
#

print 'extern "C" {'
for v in sorted(fpAliasTable.values()):
    if v[0] == '_':
        print '    extern void %s();' % v
print '};\n'

#
# Generate the syscall table itself, with aliases substituted and casted.
#

print "static const SvmSyscall SyscallTable[] = {"
for i in range(highestNum+1):
    name = callMap.get(i) or "0"

    if name in fpAliasTable:
        name = "FP_ALIAS(%s, %s)" % (name, fpAliasTable[name])

    print "    /* %4d */ %s %s," % (i, typedef, name)

print "};"
