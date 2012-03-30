# reads the tmx file format

import lxml.etree
import posixpath

TILE_SIZE = 16

class Map:
	def __init__(self, path):
		doc = lxml.etree.parse(path)
		self.path = path
		self.dir = posixpath.dirname(path)
		self.width = int(doc.getroot().get("width"))
		self.height = int(doc.getroot().get("height"))
		self.pw = TILE_SIZE * self.width
		self.ph = TILE_SIZE * self.height
		self.tilesets = [TileSet(self, elem) for elem in doc.findall("tileset")]
		self.layers = [Layer(self, elem) for elem in doc.findall("layer")]
		self.layer_dict = dict((layer.name,layer) for layer in self.layers)
		self.objects = [Obj(self, elem) for elem in doc.findall("objectgroup/object")]
		self.object_dict = dict((obj.name, obj) for obj in self.objects)
		self.props = dict((prop.get("name"), prop.get("value")) for prop in doc.findall("properties/property"))


	def gettile(self, gid):
		for result in (tileset.gettile(gid) for tileset in self.tilesets):
			if result is not None:
				return result
		return None

class TileSet:
	def __init__(self, map, xml):
		self.map = map
		self.name = xml.get("name")
		self.gid = int(xml.get("firstgid"))
		img = xml.find("image")
		self.imgpath = img.get("source")
		assert posixpath.exists(posixpath.join(map.dir, self.imgpath)), "TileSet image missing:  %s" % self.imgpath
		self.pw = int(img.get("width"))
		self.ph = int(img.get("height"))
		self.width = self.pw/TILE_SIZE
		self.height = self.ph/TILE_SIZE
		self.count = self.width * self.height
		self.tiles = [Tile(self, lid) for lid in range(self.count)]
		for node in xml.findall("tile/properties"):
			self.tiles[int(node.getparent().get("id"))].props = \
				dict((prop.get("name").lower(), prop.get("value")) for prop in node)

	def tileat(self, x, y):
		return self.tiles[x + y * self.width]

	def gettile(self, gid):
		lid = gid - self.gid
		if lid >= 0 and lid < self.count:
			return self.tiles[lid]
		return None

class Tile:
	def __init__(self, tileset, lid):
		self.tileset = tileset
		self.lid = lid
		self.gid= tileset.gid + self.lid
		self.x = lid % tileset.width
		self.y = lid / tileset.width
		self.props = {}

class Layer:
	def __init__(self, map, xml):
		self.map = map
		self.name = xml.get("name")
		self.width = int(xml.get("width"))
		self.height = int(xml.get("height"))
		self.visible = xml.get("visible", "1") != "0"
		self.opacity = float(xml.get("opacity", "1"))
		# replace with csv module?  support base64?
		self.tiles = [int(ch) for l in xml.findtext("data").strip().splitlines() for ch in l.split(',') if len(ch) > 0]
		
	def tileat(self, x, y):
		return self.map.gettile(self.tiles[x + y * self.width])

	def gettileset(self):
		for tile in (self.map.gettile(t) for t in self.tiles):
			if tile is not None:
				return tile.tileset
		return None

class Obj:
	def __init__(self, map, xml):
		self.map = map
		self.name = xml.get("name")
		self.type = xml.get("type").lower()
		self.px = int(xml.get("x"))
		self.py = int(xml.get("y"))
		self.pw = int(xml.get("width"))
		self.ph = int(xml.get("height"))

		self.tx = self.px / TILE_SIZE
		self.ty = self.py / TILE_SIZE
		self.tw = self.pw / TILE_SIZE
		self.th = self.ph / TILE_SIZE

		if self.tw < 1: self.tw = 1
		if self.th < 1: self.th = 1

		#self.tile = map.gettile(int(xml.get("gid", "0")))
		self.props = dict((prop.get("name").lower(), prop.get("value")) for prop in xml.findall("properties/property"))

	def is_overlapping(self, tx, ty):
		return tx >= self.tx and ty >= self.ty and tx < self.tx + self.tw and ty < self.ty + self.th


