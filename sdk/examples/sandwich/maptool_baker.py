import os, os.path, lxml, lxml.etree, Image, sys, misc, tmx, itertools
from tmx import TILE_SIZE

# idea - writer a "stripper" to removed unused tiles
# as a final FINAL final cleanup

def get_tile(bpx, x, y):
	return tuple( bpx[TILE_SIZE*x+px, TILE_SIZE*y+py] for px,py in itertools.product(range(TILE_SIZE), range(TILE_SIZE)))

def render_map(mp, bpx, opx):
	return [ render_tile(mp, bpx, opx, i) for i in range(mp.width * mp.height) ]

def render_tile(mp, bpx, opx, i):
	return tuple( render_pixel(mp, bpx, opx, i, px, py) for px,py in itertools.product(range(TILE_SIZE), range(TILE_SIZE)) )

def render_pixel(mp, bpx, opx, i, px, py):
	t = mp.layer_dict["overlay"].tiles[i]
	if t is not None:
		t = t.type
		result = opx[TILE_SIZE * t.x + px, TILE_SIZE * t.y + py]
		if result[3] == 255:
			return result
	t = mp.layer_dict["background"].tiles[i].type
	return bpx[TILE_SIZE * t.x + px, TILE_SIZE * t.y + py]

def bake_tileset(imgpath):
	# map image to tmxs
	dname,fname = os.path.split(imgpath)
	tiled_maps = ( tmx.Map(os.path.join(dname, path)) for path in os.listdir(dname) if path.lower().endswith(".tmx") )
	tiled_maps = ( mp for mp in tiled_maps if "background" in mp.layer_dict and "overlay" in mp.layer_dict ) 
	tiled_maps = [ mp for mp in tiled_maps if mp.layer_dict["background"].gettileset().imgpath == fname ] 

	if len(tiled_maps) == 0:
		print "Selected file is not a background image:", imgpath

	# verify that all bgs, fgs match
	for m0,m1 in itertools.product(tiled_maps, tiled_maps):
		if m0.layer_dict["overlay"].gettileset().imgpath != m1.layer_dict["overlay"].gettileset().imgpath:
			print "Maps using overlay do now share overlay: [ %s | %s ]" % (m0.path, m1.path)

	# render all the maps
	background_tileset = Image.open(imgpath)
	overlay_tileset = Image.open(os.path.join(dname, tiled_maps[0].layer_dict["overlay"].gettileset().imgpath))
	if background_tileset.mode != "RGBA": 
		background_tileset = background_tileset.convert("RGBA")
	if overlay_tileset.mode != "RGBA": 
		overlay_tileset = overlay_tileset.convert("RGBA")
	bpx = background_tileset.load()
	opx = overlay_tileset.load()
	renderered_maps = [ render_map(m, bpx, opx) for m in tiled_maps ]
	w,h = background_tileset.size
	source_tiles = [ get_tile(bpx, x, y) for x,y in itertools.product(range(w/TILE_SIZE), range(h/TILE_SIZE)) ]

	# append missing rendered tiles to bg
	dirty_maps = set()
	for map_index,rmap in enumerate(renderered_maps):
		for tile_index,rtile in enumerate(rmap):
			if not rtile in source_tiles:
				mp = tiled_maps[map_index]
				dirty_maps.add(map_index)
				new_tile_id = len(source_tiles)
				source_tiles.append(rtile)
				layer = mp.layer_dict["background"]
				layer.tiles[tile_index].gid = layer.gettileset().gid + new_tile_id

	if len(dirty_maps) > 0:
		# render new tileset image (preserved width to preserve tile IDs)
		tw = w / TILE_SIZE
		new_height = TILE_SIZE * (len(source_tiles)+tw-1) / tw
		new_tileset = Image.new("RGBA", (w, new_height))
		npx = new_tileset.load()
		for i,tile in enumerate(source_tiles):
			x = TILE_SIZE * (i % tw)
			y = TILE_SIZE * (i / tw)
			for px,py in itertools.product(range(TILE_SIZE), range(TILE_SIZE)):
				npx[x + px, y + py] = tile[px + TILE_SIZE * py]
		
		# save image
		new_tileset.save(imgpath)

		# save maps (kinda hacky - read in origin source and just change the part that I know I touched)
		for map_index in dirty_maps:
			map = tiled_maps[map_index]
			tiles = map.layer_dict["background"].tiles
			doc = lxml.etree.parse(map.path)
			doc_bg = (l for l in doc.findall("layer") if l.get("name") == "background").next().getchildren()[0]
			txt = ""
			for ty in range(map.height):
				txt = txt + ",".join( ( str(tiles[map.width * ty + tx].gid) for tx in range(map.width) ) )
				if ty < map.height-1: txt = txt + ",\n"
				else: txt = txt + "\n"
			doc_bg.text = txt
			with open(map.path, "w") as f:
				f.write(lxml.etree.tostring(doc, pretty_print=True))

if __name__ == "__main__":
	test_path = "/Users/max/src/thundercracker/sdk/examples/sandwich/Content/skytemple_background.png"
	if os.path.exists(test_path):
		bake_tileset(test_path)
	else:
		import Tkinter, tkFileDialog
		Tkinter.Tk().withdraw() # hide the main tk window
		file_path = tkFileDialog.askopenfilename(
			title		= "Open Background TileSet Image", 
			filetypes	= [("tileset image",".png")]
		)
		if len(file_path) > 0: bake_tileset(file_path) 
