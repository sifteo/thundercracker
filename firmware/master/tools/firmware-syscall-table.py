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
# Library aliases.
#
# Our floating point and 64-bit integer syscalls intentionally
# have the same signature as the corresponding EABI or libgcc
# library functions. GCC is particularly terrible at realizing that
# our syscall wrappers are actually the same thing, and it
# generates a ton of redundant loads and stores.
#
# So, to guarantee the best math performance, we directly alias
# these syscalls to the corresponding library functions.
#
# Note that we apparently can't do this in the linker script
# for functions which otherwise would be garbage collected.
# Implementing the aliasing with generated C++ code ensures
# that the library functions are linked properly.
#

aliasTable = {
    # Floating point operators
    '_SYS_add_f32': '__addsf3',
    '_SYS_add_f64': '__adddf3',
    '_SYS_sub_f32': '__aeabi_fsub',
    '_SYS_sub_f64': '__aeabi_dsub',
    '_SYS_mul_f32': '__aeabi_fmul',
    '_SYS_mul_f64': '__aeabi_dmul',
    '_SYS_div_f32': '__aeabi_fdiv',
    '_SYS_div_f64': '__aeabi_ddiv',

    # Floating point conversions
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

    # Floating point comparisons
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

    # Math library functions
    '_SYS_fmodf': '__ieee754_fmodf',
    '_SYS_sqrtf': '__ieee754_sqrtf',
    '_SYS_logf': '__ieee754_logf',
    '_SYS_fmod': '__ieee754_fmod',
    '_SYS_sqrt': '__ieee754_sqrt',
    '_SYS_logd': '__ieee754_log',
    '_SYS_pow': '__ieee754_pow',
    '_SYS_powf': '__ieee754_powf',
    '_SYS_sinf': 'sinf',
    '_SYS_cosf': 'cosf',
    
    # 64-bit integer operators
    '_SYS_sdiv_i64' : '__aeabi_ldivmod',
    '_SYS_udiv_i64' : '__aeabi_uldivmod',
    '_SYS_shl_i64' : '__aeabi_llsl',
    '_SYS_srl_i64' : '__aeabi_llsr',
    '_SYS_sra_i64' : '__aeabi_lasr',
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
for v in sorted(aliasTable.values()):
    if v[0] == '_':
        print '    extern void %s();' % v
print '};\n'

#
# Generate the syscall table itself, with aliases substituted and casted.
#

print "static const SvmSyscall SyscallTable[] = {"
for i in range(highestNum+1):
    name = callMap.get(i) or "0"

    if name in aliasTable:
        name = "SYS_ALIAS(%s, %s)" % (name, aliasTable[name])

    print "    /* %4d */ %s %s," % (i, typedef, name)

print "};"
