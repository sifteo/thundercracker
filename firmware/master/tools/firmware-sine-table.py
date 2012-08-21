#!/usr/bin/env python
#
# Firmware sine table generator.
#

import math, sys

sys.stdout.write("static const uint16_t sineTable[] = {")

for n in range(0x801):
    radians = n * (math.pi / 2) / 0x800
    sine = math.sin(radians)
    intSin = int(sine * 0x10000 + 0.5)

    if intSin > 0xFFFF:
        break

    if not (n & 7):
        sys.stdout.write("\n    ")

    sys.stdout.write("0x%04x," % intSin)

sys.stdout.write("\n};\n")

