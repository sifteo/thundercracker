import math

for i in range(64):
    t = math.sin(i / 64.0 * math.pi / 2)
    print "0x%02x," % round(t * 127),
    if (i % 8) == 7:
        print

