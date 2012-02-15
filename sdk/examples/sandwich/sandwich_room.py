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
SUBDIV_NONE = 0
SUBDIV_DIAG_POS = 1
SUBDIV_DIAG_NEG = 2
SUBDIV_BRDG_HOR = 3
SUBDIV_BRDG_VER = 4

def bit_count(mask):
	result = 0
	while mask != 0:
		if mask & 1: result += 1
		mask >>= 1
	return result

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
		self.subdiv_type = SUBDIV_NONE

	def hasitem(self): return len(self.item) > 0 and self.item != "ITEM_NONE"
	def tileat(self, x, y): return self.map.background.tileat(8*self.x + x, 8*self.y + y)
	def iswalkable(self, x, y): return iswalkable(self.tileat(x, y))
	def overlaytileat(self, x, y): return self.map.overlay.tileat(8*self.x + x, 8*self.y + y)

	def primary_center(self):
		# todo bridges
		if self.subdiv_type == SUBDIV_BRDG_VER:
			return (self.first_bridge_col+1, 4)
		elif self.subdiv_type == SUBDIV_NONE or self.subdiv_type == SUBDIV_BRDG_HOR:
			for (x,y) in misc.spiral_into_madness():
				if self.iswalkable(x-1, y) and self.iswalkable(x,y):
					return self.adjust(x,y)
			return (0,0)
		elif self.subdiv_type == SUBDIV_DIAG_POS or self.subdiv_type == SUBDIV_DIAG_NEG:
			# in this case, we pick the "center" which is reachable from the top
			for (x,y) in misc.spiral_into_madness():
				if self.iswalkable(x-1, y) and self.iswalkable(x,y) and self.subdiv_masks[x+(y<<3)] & 1:
					return (x,y)
	
	def secondary_center(self):
		# todo bridges
		assert self.subdiv_type != SUBDIV_NONE, "non-subdivided rooms don't have a secondary center"
		if self.subdiv_type == SUBDIV_DIAG_POS or self.subdiv_type == SUBDIV_DIAG_NEG:
			for (x,y) in misc.spiral_into_madness():
				if self.iswalkable(x-1, y) and self.iswalkable(x,y) and not self.subdiv_masks[x+(y<<3)] & 1:
					return (x,y)
		elif self.subdiv_type == SUBDIV_BRDG_HOR:
			return (4,self.first_bridge_row)
		elif self.subdiv_type == SUBDIV_BRDG_VER:
			for (x,y) in misc.spiral_into_madness():
				if self.iswalkable(x-1, y) and self.iswalkable(x,y):
					return (x,y)
		return (0,0)

	def subdiv_center(self):
		pass
	
	def ispath(self, x, y):
		return "path" in self.tileat(x,y).props

	def adjust(self, x, y):
		if self.ispath(x-1,y):
			if not self.ispath(x,y) and self.ispath(x-2,y):
				x-=1
		elif self.ispath(x,y) and self.ispath(x+1,y):
			x+=1
		return (x,y)
	
	def isblocked(self): return self.primary_center() == (0,0)

	def hasoverlay(self):
		if self.map.overlay is None: return False
		for y in range(8):
			for x in range(8):
				if self.overlaytileat(x,y) is not None:
					return True
		return False
	
	def find_is_it_a_trap(self):
		traptiles = [ (x,y) for y in range(8) for x in range(8) if "trapdoor" in self.tileat(x,y).props ]
		self.is_a_trap = len(traptiles) > 0
		if self.is_a_trap:
			assert len(traptiles) == 16, "each trap is a 4x4 grid"
			x,y = traptiles[0]
			self.trapx = x + 2
			self.trapy = y + 2

	def find_subdivisions(self):
		if all((portal == PORTAL_OPEN for portal in self.portals)):
			#first bridge-rows or cols
			for y in range(8):
				if all(("bridge" in self.tileat(x,y).props for x in range(8))):
					self.subdiv_type = SUBDIV_BRDG_HOR
					self.first_bridge_row = y
					return
				elif all(("bridge" in self.tileat(y,x).props for x in range(8))):
					self.subdiv_type = SUBDIV_BRDG_VER
					self.first_bridge_col = y
					return
			# let's try looking for diagonals
			# start by listing the "canonical" cardinal open tiles
			cardinals = [
				((x,0) for x in range(8) if self.iswalkable(x,0)).next(),
				((0,y) for y in range(8) if self.iswalkable(0,y)).next(),
				((x,7) for x in range(8) if self.iswalkable(x,7)).next(),
				((7,y) for y in range(8) if self.iswalkable(7,y)).next()
			]
			slots = [ 0 for _ in range(64) ]
			# flood-fill "reachable" tiles
			for side,(x,y) in enumerate(cardinals): 
				self._subdiv_visit(slots, 1<<side, x, y)
			for cnt in (bit_count(slots[x+(y<<3)]) for (x,y) in cardinals): 
				assert cnt == 2 or cnt == 4, "Unusual subdivision in map: " + self.map.id
			tx,ty = cardinals[SIDE_TOP]
			maskTop = slots[tx+(ty<<3)]
			if bit_count(maskTop) == 2:
				self.subdiv_masks = slots
				if maskTop & (1<<SIDE_LEFT):
					self.subdiv_type = SUBDIV_DIAG_POS
				elif maskTop & (1<<SIDE_RIGHT):
					self.subdiv_type = SUBDIV_DIAG_NEG
				else:
					raise Exception("unusual subdivision in map: " + self.map.id)
		
	
	def _subdiv_visit(self, slots, mask, x, y):
		if x >= 0 and y >= 0 and x < 8 and y < 8 and self.iswalkable(x,y) and slots[x+(y<<3)] & mask == 0:
			slots[x+(y<<3)] |= mask
			self._subdiv_visit(slots, mask, x+1, y)
			self._subdiv_visit(slots, mask, x-1, y)
			self._subdiv_visit(slots, mask, x, y+1)
			self._subdiv_visit(slots, mask, x, y-1)

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
		# centerx, centery
		cx,cy = self.primary_center()
		src.write("        %d, %d, \n" % (cx, cy))
		src.write("    },\n")		

