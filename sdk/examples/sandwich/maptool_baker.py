import os, os.path, lxml, lxml.etree, Image, sys, misc, tmx, itertools
from tmx import TILE_SIZE

# idea - writer a "stripper" to removed unused tiles
# as a final FINAL final cleanup

def get_tile(bpx, x, y):
	return tuple( bpx[TILE_SIZE*x+px, TILE_SIZE*y+py] for py,px in itertools.product(range(TILE_SIZE), range(TILE_SIZE)))

def render_map(mp, bpx, opx):
	return [ render_tile(mp, bpx, opx, i) for i in range(mp.width * mp.height) ]

def render_tile(mp, bpx, opx, i):
	return tuple( render_pixel(mp, bpx, opx, i, px, py) for py,px in itertools.product(range(TILE_SIZE), range(TILE_SIZE)) )

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
	source_tiles = [ get_tile(bpx, x, y) for y,x in itertools.product(range(h/TILE_SIZE), range(w/TILE_SIZE)) ]

	# append missing rendered tiles to bg
	dirty_maps = set()
	for map_index,rmap in enumerate(renderered_maps):
		mp = tiled_maps[map_index]
		layer = mp.layer_dict["background"]
		for tile_index,rtile in enumerate(rmap):
			if not rtile in source_tiles:
				dirty_maps.add(map_index)
				new_tile_id = len(source_tiles)
				source_tiles.append(rtile)
				layer.tiles[tile_index].gid = layer.gettileset().gid + new_tile_id
			else:
				layer.tiles[tile_index].gid = layer.gettileset().gid + source_tiles.index(rtile)


	if len(dirty_maps) > 0:
		# render new tileset image (preserved width to preserve tile IDs)
		tw = w / TILE_SIZE
		th = h / TILE_SIZE
		new_th = (len(source_tiles)+tw-1) / tw
		new_height = TILE_SIZE * new_th
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
		tiles_per_row = w / TILE_SIZE
		gid_offset = tiles_per_row * (new_height - h) / TILE_SIZE
		for map_index,map in enumerate(tiled_maps):
			firstGid = map.layer_dict["background"].gettileset().gid

			doc = lxml.etree.parse(map.path)

			# fixup tileset declarations
			doc.find("tileset[@firstgid='%d']/image" % firstGid).attrib["height"] = str(new_height)
			old_to_new = {}
			old_to_new[0] = 0
			for i in range(tw*new_th):
				old_to_new[i + firstGid] = i + firstGid
			for doc_tileset in doc.findall("tileset"):
				otherFirstGid = int(doc_tileset.attrib["firstgid"])
				if (otherFirstGid == firstGid): continue
				otherw = int(doc_tileset.find("image").attrib["width"]) / TILE_SIZE
				otherh = int(doc_tileset.find("image").attrib["height"]) / TILE_SIZE
				if otherFirstGid > firstGid:
					doc_tileset.attrib["firstgid"] = str(otherFirstGid + gid_offset);
					for i in range(otherw*otherh):
						old_to_new[i + otherFirstGid] = i + otherFirstGid + gid_offset
				else:
					for i in range(otherw*otherh):
						old_to_new[i + otherFirstGid] = i + otherFirstGid

			# update map gids, and add FakeTiles for the Nones
			for layer in map.layers:
				for i,tile in enumerate(layer.tiles):
					if tile is None:
						layer.tiles[i] = FakeTile()
					elif layer.name != "background":
						tile.gid = old_to_new[tile.gid]
						

			# update all layers
			for layer in doc.findall("layer"):
				tiles = map.layer_dict[layer.attrib["name"]].tiles
				txt = layer.getchildren()[0]
				txt.text = "\n"
				for ty in range(map.height):
					txt.text = txt.text + ",".join( ( str(tiles[map.width * ty + tx].gid) for tx in range(map.width) ) )
					txt.text = txt.text + ",\n" if ty < map.height-1 else txt.text + "\n"

			# save
			with open(map.path, "w") as f:
				f.write(lxml.etree.tostring(doc, pretty_print=True))

class FakeTile:
	def __init__(self):
		self.gid = 0


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
