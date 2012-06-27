#!/usr/bin/env python
#
# Firmware linker script generator.
#
# We must set the FLASH_START address appropriately, based on whether
# we're preparing an image that will run in cooperation with the
# bootloader or not
#

import sys, os
from string import Template

#######################################################################

DIR = os.path.dirname(__file__)

if '--bootloadable' in sys.argv:
    flashStart = "0x08002000"
else:
    flashStart = "0x08000000"

with open(os.path.join(DIR, 'ldscript-template.ld'), 'r') as fin:
    s = Template(fin.read())

    with open(os.path.join(DIR, '../stm32/target.ld'), 'w') as fout:
        fout.write(s.substitute(FLASH_START=flashStart))
