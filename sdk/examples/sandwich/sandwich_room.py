import lxml.etree, os, os.path, re, tmx, misc, math
from sandwich_trigger import *

# constants
PORTAL_OPEN = 0
PORTAL_WALL = 1
PORTAL_DOOR = 2
SIDE_TOP = 0
SIDE_LEFT = 1
SIDE_BOTTOM = 2
SIDE_RIGHT = 3

def iswalkable(tile): 
	return "wall" not in tile.props and "obstacle" not in tile.props

class Room:
	def __init__(self, map, lid):
		self.map = map
		self.lid = lid
		self.x = lid % map.width
		self.y = lid / map.width
		self.portals = [ PORTAL_WALL, PORTAL_WALL, PORTAL_WALL, PORTAL_WALL ]
		self.triggers = []
		self.trigger_dict = {}
		self.door = None

	def hasitem(self): return len(self.item) > 0 and self.item != "ITEM_NONE"
	def tileat(self, x, y): return self.map.background.tileat(8*self.x + x, 8*self.y + y)
	def overlaytileat(self, x, y): return self.map.overlay.tileat(8*self.x + x, 8*self.y + y)

	def center(self):
		for (x,y) in misc.spiral_into_madness():
			if iswalkable(self.tileat(x-1, y)) and iswalkable(self.tileat(x,y)):
				return self.adjust(x,y)
		return (0,0)
	
	def ispath(self, x, y):
		return "path" in self.tileat(x,y).props

	def adjust(self, x, y):
		if self.ispath(x-1,y):
			if not self.ispath(x,y) and self.ispath(x-2,y):
				x-=1
		elif self.ispath(x,y) and self.ispath(x+1,y):
			x+=1
		return (x,y)
	
	def isblocked(self): return self.center() == (0,0)

	def hasoverlay(self):
		if self.map.overlay is None: return False
		for y in range(8):
			for x in range(8):
				if self.overlaytileat(x,y) is not None:
					return True
		return False
	
	def validate_triggers_for_quest(self, quest):
		assert len([t for t in self.triggers if t.is_active_for(quest)]) < 2, "Too many triggers in room in map: " + self.map.id

	def write_source_to(self, src):
		src.write("    {\n")
		# collision mask rows
		src.write("        { ")
		for row in range(8):
			rowMask = 0
			for col in range(8):
				if not iswalkable(self.tileat(col, row)):
					rowMask |= (1<<col)
			src.write("0x%x, " % rowMask)
		src.write("},\n")
		# tiles
		src.write("        { ")
		for ty in range(8):
			#src.write("            ")
			for tx in range(8):
				src.write("0x%x, " % self.tileat(tx,ty).lid)
			#src.write("\n")
		src.write("},\n")
		# overlay
		if self.hasoverlay():
			src.write("        %s_overlay_%d_%d, " % (self.map.id, self.x, self.y))
		else:
			src.write("        0, ")
		# centerx, centery, reserved
		cx,cy = self.center()
		src.write("%d, %d, \n" % (cx, cy))
		src.write("    },\n")		

