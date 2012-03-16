import os, os.path, lxml, lxml.etree, Image, sys, misc, tmx

def export(file_path):
	src = Image.open(file_path)
	if src.mode != "RGBA": src = src.convert("RGBA")
	w,h = src.size
	assert w > 0 and h > 0 and w % 16 == 0 and h % 16 == 0
	print "[ Loading %s ]" % file_path
	px = src.load()
	basedir,filename = os.path.split(file_path)

	# isolate filename keyword
	if "_" in filename:
		idx = filename.index("_")
		keyword = filename[0:idx]
	else:
		keyword = "tileset_" + filename[0:-4]
	print "[ Identified Filename Keyword: %s ]" % keyword


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
					src_to_dst[(tx,ty)] = len(background_tiles)
					background_tiles.append(tile)
				else:
					src_to_dst[(tx,ty)] = background_tiles.index(tile)
	
	# build background image
	print "[ Writing Background Tileset: %s_background.png ]" % keyword
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
	background.save(keyword+"_background.png")

	# build overlay image
	if len(overlay_tiles) > 0:
		print "[ Writing Overlay Tileset: %s_overlay.png ]" % keyword
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
		overlay.save(keyword+"_overlay.png")
	else:
		print "[ Skipping Overlay Tileset ]"
		overlay = None

	# write tmx
	print "[ Writing TMX TileMap: %s.tmx ]" % keyword
	with open(keyword+".tmx", "w") as tmx:
		tmx.write('<?xml version="1.0" encoding="UTF-8"?>\n')
		tmx.write('<map version="1.0" orientation="orthogonal" width="%d" height="%d" tilewidth="16" tileheight="16">\n' % (tw,th))
 		tmx.write('\t<properties>\n')
 		tmx.write('\t</properties>\n')
 		tmx.write('\t<tileset firstgid="1" name="%s_background" tilewidth="16" tileheight="16">\n' % keyword)
 		tmx.write('\t\t<image source="%s_background.png" width="%d" height="%d"/>\n' % (keyword, 16*bw, 16*bh))
 		tmx.write('\t</tileset>\n')
 		if overlay is not None:
	 		tmx.write('\t<tileset firstgid="%d" name="%s_overlay" tilewidth="16" tileheight="16">\n' % (1+bw*bh,keyword))
	 		tmx.write('\t\t<image source="%s_overlay.png" width="%d" height="%d"/>\n' % (keyword, 16*ow, 16*oh))
	 		tmx.write('\t</tileset>\n')
 		tmx.write('\t<layer name="background" width="%d" height="%d">\n' % (tw,th))
 		tmx.write('\t\t<data encoding="csv">\n')
 		for ty in range(th):
 			for tx in range(tw):
 				tileId = 0
 				if (tx,ty) in src_to_dst:
 					tileId = 1 + src_to_dst[(tx,ty)]
 				if tx != tw-1 or ty != th-1:
 					tmx.write('%d,' % tileId)
 				else:
 					tmx.write('%d' % tileId)
 			tmx.write('\n')
 		tmx.write('\t\t</data>\n')
 		tmx.write('\t</layer>')
 		tmx.write('\t<objectgroup name="triggers" width="%d" height="%d"/>\n' % (tw,th))
 		tmx.write('</map>\n\n')

if __name__ == "__main__":
	test_path = "/Users/max/Dropbox/Sandwich/Ian Concepts/cave_interactions3 copy.png"
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
