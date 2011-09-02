#!/usr/bin/env python
#
# Build the palette we use in the heat map.
#

import colorsys, math

STEPS = 512
HUE_RANGE = (0.666, 0)
LIGHTNESS_RANGE = (0.4, 0.9)

for i in range(STEPS):
	norm = i / float(STEPS - 1)

	hue = HUE_RANGE[0] + norm * (HUE_RANGE[1] - HUE_RANGE[0])
	lightness = LIGHTNESS_RANGE[0] + norm * (LIGHTNESS_RANGE[1] - LIGHTNESS_RANGE[0])

	rgb = colorsys.hls_to_rgb(hue, lightness, 1)
	
	r = int(0xFF * rgb[0])
	g = int(0xFF * rgb[1])
	b = int(0xFF * rgb[2])
	rgb16 = (((r) >> 3)<<11) | (((g) >> 2)<<5) | ((b) >> 3)

	print "0x%04x," % rgb16,
	if (i & 7) == 7:
		print

