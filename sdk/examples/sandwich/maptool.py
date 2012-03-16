import os, os.path, lxml, lxml.etree, Image, sys, misc, tmx

def export(file_path):
	src = Image.open(file_path)
	if src.mode != "RGBA": src = src.convert("RGBA")
	w,h = src.size
	assert w > 0 and h > 0 and w % 16 == 0 and h % 16 == 0
	print "[ Loading %s ]" % file_path
	px = src.load()
	basedir,filename = os.path.split(file_path)

	#identify the transparent color key
	print "[ Identifying Color Key ]"
	counts = {}
	common_count = 0
	common_pixel = None
	for x in range(w):
		for y in range(h):
			pixel = px[x,y]
			if pixel in counts:
				counts[pixel] += 1
			else:
				counts[pixel] = 1
			if counts[pixel] > common_count:
				common_count = counts[pixel]
				common_pixel = pixel
	#print common_pixel
	for x in range(w):
		for y in range(h):
			if px[x,y] == common_pixel:
				px[x,y] = (0, 0, 0, 0)


	# extract tiles to a list
	print "[ Extracting Unique Tiles ]"
	tw,th = w/16, h/16
	background_tiles = []
	overlay_tiles = []
	src_to_dst = {} # will only create keys for 
	for ty in range(th): 
		for tx in range(tw):
			tile = tuple(px[16*tx+x, 16*ty+y] for y in range(16) for x in range(16))
			opaque_count = 0
			for r,g,b,a in tile:
				if a == 255: opaque_count += 1
			if opaque_count < 16*16:
				if opaque_count > 0 and not tile in overlay_tiles:
					overlay_tiles.append(tile)
			else:
				if not tile in background_tiles:
					src_to_dst[(tx,tw)] = len(background_tiles)
					background_tiles.append(tile)
				else:
					src_to_dst[(tx,tw)] = background_tiles.index(tile)
	
	# build background image
	print "[ Building Background Tileset ]"
	bw = 8
	bh = (len(background_tiles)+bw-1) / bw
	tile_iterator = background_tiles.__iter__()
	background = Image.new("RGBA", (16*bw,16*bh))
	px = background.load()
	for by in range(bh):
		for bx in range(bw):
			try:
				tile = tile_iterator.next()
			except:
				break
				break
			for y in range(16):
				for x in range(16):
					px[16*bx + x, 16*by + y] = tile[x + 16 * y]
	background.show()

	# build overlay image
	if len(overlay_tiles) > 0:
		print "[ Building Overlay Tileset ]"
		ow = 8
		oh = (len(overlay_tiles)+ow-1) / ow
		tile_iterator = overlay_tiles.__iter__()
		overlay = Image.new("RGBA", (16*ow, 16*oh))
		px = overlay.load()
		for oy in range(oh):
			for ox in range(ow):
				try:
					tile = tile_iterator.next()
				except:
					break
					break
				for y in range(16):
					for x in range(16):
						px[16*ox + x, 16*oy + y] = tile[x + 16 * y]
		overlay.show()
	else:
		print "[ Skipping Overlay Tileset ]"
		overlay = None



if __name__ == "__main__":
	test_path = "/Users/max/Dropbox/Sandwich/Ian Concepts/cave_interactions3.png"
	if os.path.exists(test_path):
		export(test_path)
	else:
		import Tkinter, tkFileDialog
		Tkinter.Tk().withdraw() # hide the main tk window
		file_path = tkFileDialog.askopenfilename(
			title		= "Open Interactions Image", 
			filetypes	= [("interactions image",".png")]
		)
		if len(file_path) > 0: export(file_path) 
