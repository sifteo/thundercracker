import os, os.path, lxml, lxml.etree, Image, sys, misc, tmx

def export(file_path):
	src = Image.open(file_path)
	if src.mode != "RGBA": src = src.convert("RGBA")
	w,h = src.size
	assert w > 0 and h > 0 and w % 16 == 0 and h % 16 == 0
	print "[ Loading %s ]" % file_path
	px = src.load()
	basedir,filename = os.path.split(file_path)

	# extract tiles to a list
	print "[ Extracting Unique Tiles ]"
	tw,th = w/16, h/16
	unique_tiles = []
	for ty in range(th): 
		for tx in range(tw):
			tile = tuple(px[16*tx+x, 16*ty+y] for y in range(16) for x in range(16))
			if not tile in unique_tiles:
				unique_tiles.append(tile)
	print "[ Number of Duplicates: %d ]" % ((tw*th) - len(unique_tiles))
	dw = 8
	dh = (len(unique_tiles)+dw-1) / dw
	tile_iterator = unique_tiles.__iter__()
	dst = Image.new("RGBA", (16*dw,16*dh))
	px = dst.load()
	for dy in range(dh):
		for dx in range(dw):
			try:
				tile = tile_iterator.next()
			except:
				break
				break
			for y in range(16):
				for x in range(16):
					px[16*dx + x, 16*dy + y] = tile[x + 16 * y]
	dst.show()

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
