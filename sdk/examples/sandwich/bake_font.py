import Image, ImageDraw

im = Image.open("monaco.png").convert("1")
w,h = im.size
px = im.load()
assert w%5 == 0
assert h == 11
glyphs = map(chr, range(ord(' ')+1, ord('z')+1))
assert (w/5) == len(glyphs)

print "static const uint8_t font_data[] = {"
print "    0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,"

def width_of(i):
	result = 5
	for x in range(5):
		if len([y for y in range(h) if px[5*(i+1)-x-1,y]]) == 0:
			result = result - 1
		else:
			break
	return result

for i,glyph in enumerate(glyphs):
	bytes = [ width_of(i)+1 ]
	for y in range(h):
		byte = 0
		for x in range(5):
			if px[5*i+x,y]:
				byte = byte | (1<<x)
		bytes.append(byte)
	print "    %s" % "".join(["0x%02x," % x for x in bytes])

print "};"