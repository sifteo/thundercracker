# implementation of sandwich kingdom maps

import os
import tmx
import misc

# enums
PORTAL_OPEN = 0
PORTAL_WALLED = 1
PORTAL_DOOR = 2
SIDE_TOP = 0
SIDE_LEFT = 1
SIDE_BOTTOM = 2
SIDE_RIGHT = 3


def load():
	return World(".")

class World:
	
	def __init__(self, dir):
		self.maps = dict((path[0:-4], Map(path)) for path in os.listdir(dir) if path.endswith(".tmx"))

		

class Map:
	def __init__(self, path):
		self.name = path[0:-4]
		self.raw = tmx.Map(path)
		if not "background" in self.raw.layer_dict:
			raise Exception("Map Does Note Contain Background Layer: %s" % self.name)
		self.background = self.raw.layer_dict["background"]
		if self.background.gettileset().count >= 256:
			raise Exception("Map is too large (must have < 256 tiles): %s" % self.name)
		self.overlay = self.raw.layer_dict.get("overlay", None)
		self.width = self.raw.pw / 128
		self.height = self.raw.ph / 128
		self.count = self.width * self.height
		self.rooms = [Room(self, i) for i in range(self.count)]

		# infer portal states (part of me reeeally wants this to be more clever)
		for r in self.rooms:
			if r.isblocked():
				r.portals[0] = PORTAL_WALLED
				r.portals[1] = PORTAL_WALLED
				r.portals[2] = PORTAL_WALLED
				r.portals[3] = PORTAL_WALLED
				continue
			tx,ty = (8*r.x, 8*r.y)
			# top
			prevType = r.portals[0] = PORTAL_WALLED
			for i in range(8):
				typ = portal_type(self.background.tileat(tx+i, ty))
				if typ == PORTAL_WALLED:
					pass
				elif typ == PORTAL_OPEN:
					if prevType == PORTAL_OPEN:
						r.portals[0] = PORTAL_OPEN
						break
				else:
					r.portals[0] = typ
					break
				prevType = typ
			if r.portals[0] != PORTAL_WALLED and (r.y == 0 or self.roomat(r.x, r.y-1).isblocked()):
				r.portals[0] = PORTAL_WALLED
			# left
			prevType = r.portals[1] = PORTAL_WALLED
			for i in range(8):
				typ = portal_type(self.background.tileat(tx, ty+i))
				if typ == PORTAL_WALLED:
					pass
				elif typ == PORTAL_OPEN:
					r.portals[1] = PORTAL_OPEN
					break
				else:
					r.portals[i] =typ
					break
				prevType = typ
			if r.portals[1] != PORTAL_WALLED and (r.x == 0 or self.roomat(r.x-1, r.y).isblocked()):
				r.portals[1] = PORTAL_WALLED
			# bottom
			prevType = r.portals[2] = PORTAL_WALLED
			for i in range(8):
				typ = portal_type(self.background.tileat(tx+i, ty+7))
				if typ == PORTAL_WALLED:
					pass
				elif typ == PORTAL_OPEN:
					if prevType == PORTAL_OPEN:
						r.portals[2] = PORTAL_OPEN
						break
				else:
					r.portals[2] = typ
					break
				prevType = typ
			if r.portals[2] != PORTAL_WALLED and (r.y == self.height-1 or self.roomat(r.x, r.y+1).isblocked()):
				r.portals[2] = PORTAL_WALLED
			# right
			prevType = r.portals[3] = PORTAL_WALLED
			for i in range(8):
				typ = portal_type(self.background.tileat(tx+7, ty+i))
				if typ == PORTAL_WALLED:
					pass
				elif typ == PORTAL_OPEN:
					r.portals[3] = PORTAL_OPEN
				else:
					r.portals[3] = typ
					break
				prevType = typ
			if r.portals[3] != PORTAL_WALLED and (r.x == self.width-1 or self.roomat(r.x+1, r.y).isblocked()):
				r.portals[3] = PORTAL_WALLED
		# validate portals
		for y in range(self.height):
			if self.roomat(0,y).portals[1] == PORTAL_OPEN or self.roomat(self.width-1, y).portals[3] == PORTAL_OPEN:
				raise Exception("Boundary Wall Error in Map: %s" % self.name)
		for x in range(self.width):
			if self.roomat(x,0).portals[0] == PORTAL_OPEN or self.roomat(x, self.height-1).portals[2] == PORTAL_OPEN:
				raise Exception("Boundary Wall Error in Map: %s" % self.name)
		for x in range(self.width-1):
			for y in range(self.height):
				if self.roomat(x,y).portals[3] != self.roomat(x+1,y).portals[1]:
					raise Exception ("Portal Mismatch in Map: %s" % self.name)
		for x in range(self.width):
			for y in range(self.height-1):
				if self.roomat(x,y).portals[2] != self.roomat(x,y+1).portals[0]:
					raise Exception ("Portal Mismatch in Map: %s" % self.name)
		# unpack objects
		print "TODO: UNPACK OBJECTS"
		for obj in self.raw.objects:
			pass



	
	def roomat(self, x, y): return self.rooms[x + y * self.width]

		
class Room:
	def __init__(self, map, lid):
		self.map = map
		self.lid = lid
		self.x = lid % map.width
		self.y = lid / map.width
		self.portals = [ PORTAL_WALLED, PORTAL_WALLED, PORTAL_WALLED, PORTAL_WALLED ]
		self.trigger = None
		self.item = ""

	def hasitem(self): return len(self.item) > 0 and self.item != "ITEM_NONE"
	def tileat(self, x, y): return self.map.background.tileat(8*self.x + x, 8*self.y + y)
	def overlaytileat(self, x, y): return self.map.overlay.tileat(8*self.x + x, 8*self.y + y)

	def listtorchtiles(self):
		for x in range(8):
			for y in range(8):
				if istorch(self.gettile(x,y)) and (y == 0 or not istorch(gettile(x,y-1))):
					yield (x,y) 
	
	def center(self):
		for (x,y) in misc.spiral_into_madness():
			if iswalkable(self.tileat(x-1, y)) and iswalkable(self.tileat(x,y)):
				return self.adjust(x,y)
		return (0,0)
	
	def adjust(self, x, y):
		if ispath(self.tileat(x-1,y)):
			if not ispath(self.tileat(x,y)) and ispath(self.tileat(x-2,y)):
				x-=1
		elif ispath(self.tileat(x,y)) and ispath(self.tileat(x+1,y)):
			x+=1
		return (x,y)
	
	def isblocked(self): return self.center() == (0,0)

	def hasoverlay(self):
		if map.overlay is None: return False
		for y in range(8):
			for x in range(8):
				if overlaytileat(x,y) is not None:
					return True
		return False

#convenience getters

def istorch(tile): return "torch" in tile.props
def isobst(tile): return "obstacle" in tile.props or istorch(tile)
def iswall(tile): return "wall" in tile.props
def isdoor(tile): return "door" in tile.props
def isopen(tile): return not isdoor(tile) and not isobst(tile) and not iswall(tile)
def iswalkable(tile): return not iswall(tile) and not isobst(tile)
def ispath(tile): return not "path" in tile.props

def portal_type(tile):
	if isdoor(tile):
		return PORTAL_DOOR
	elif iswall(tile) or isobst(tile):
		return PORTAL_WALLED
	else:
		return PORTAL_OPEN

