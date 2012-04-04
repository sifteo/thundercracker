import lxml.etree, os, posixpath, re, tmx, misc, math
from sandwich_trigger import *
from itertools import product

# constants
PORTAL_OPEN = 0
PORTAL_WALL = 1
PORTAL_DOOR = 2
PORTAL_TO_CHAR = {
	PORTAL_OPEN: "O",
	PORTAL_WALL: "X",
	PORTAL_DOOR: "_"
}
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

def iswalkable(tile, x, y): 
	if "wall" in tile.props or "obstacle" in tile.props:
		return False
	for obj in tile.tileset.map.objects:
		if obj.type == "obstacle" and obj.is_overlapping(x, y):
			return False
	return True
	

class Room:
	def __init__(self, map, lid):
		self.map = map
		self.lid = lid
		self.locations = [ loc for loc in map.locations if loc.rid == lid ]
		self.x = lid % map.width
		self.y = lid / map.width
		self.portals = [ PORTAL_WALL, PORTAL_WALL, PORTAL_WALL, PORTAL_WALL ]
		self.triggers = []
		self.trigger_dict = {}
		self.door = None
		self.subdiv_type = SUBDIV_NONE
		
		trapdoors = [obj for obj in map.raw.objects if obj.type == "trapdoor" and self.contains_obj(obj)]
		self.its_a_trap = len(trapdoors) > 0
		if self.its_a_trap:
			#                                       IT'S A TRAP!
			# . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
			# . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
			# . . . . . . . . . . . . . . . . _,,,-------------------,_ . . . . . . . . . . . . . . . .
			# . . . . . . . . . . . . . . ,-  : : : :::: :::: :: : : : : :  '-, . . . . . . . . . . . .
			# . . . . . . . . . . . . .,-' :: : : :::: :::: :::: :::: : : :o : '-,  . . . . . . . . . .
			# . . . . . . . . . . . ,-' :: ::: :: : : :: :::: :::: :: : : : : :O '-,  . . . . . . . . .
			# . . . . . . . . . .,-' : :: :: :: :: :: : : : : : , : : :  :::: :::: ::'; . . . . . . . .
			# . . . . . . . . .,-' / / : :: :: :: :: : : :::: :::-, ;; ;; ;; ;; ;; ;; ;\  . . . . . . .
			# . . . . . . . . /,-',' :: : : : : : : : : :: :: :: : '-, ;; ;; ;; ;; ;; ;;| . . . . . . .
			# . . . . . . . /,',-' :: :: :: :: :: :: :: : ::_,-----,_'-, ;; ;; ;; ;; |  . . . . . . . .
			# . . . . . _/ :,' :/ :: :: :: : : :: :: _,-'/ : ,-';'-'''''---, ;; ;; ;;,' . . . . . . . .
			# . . . ,-' / : : : : : : ,-''' : : :,--'' :|| /,-'-'--'''__,''' \ ;; ;,-'  . . . . . . . .
			# . . . \ :/,, : : : _,-' --,,_ : : \ :\ ||/ /,-'-'x### ::\ \ ;;/ . . . . . . . . . . . . .
			# . . . . \/ /--'''' : \ #\ : :\ : : \ :\ \| | : (O##  : :/ /-''  . . . . . . . . . . . . .
			# . . . . /,'____ : :\ '-#\ : \, : :\ :\ \ \ : '-,___,-',-`-,,  . . . . . . . . . . . . . .
			# . . . . ' ) : : : :''''--,,--,,,,,,- \ \ :: ::--,,_''-,,'''- :'- :'-, . . . . . . . . . .
			# . . . . .) : : : : : : ,, : ''''---------' \ :: :: :: :'''''- ::  :,/\  . . . . . . . . .
			# . . . . .\,/ /|\\| | :/ / : : : : : : : ,'--, :: :: :: :: ::,--'' :,-' \ \  . . . . . . .
			# . . . . .\\'|\\ \|/ '/ / :: :_---,, : , | )'; :: :: :: :,-'' : ,-' : : :\ \,  . . . . . .
			# . . . ./- :| \ |\ : |/\ :: ::-----, :\/ :|/ :: :: ,-'' : :,-' : : : : : : ''-,,_  . . . .
			# . . ..| : : :/ ''-(, :: :: :: '''''--,,,,,'' :: ,-'' : :,-' : : : : : : : : :,-'''\\  . .
			# . ,-' : : : | : : '') : : :-''''---,: : ,--''' : :,-'' : : : : : : : : : ,-' :-'''''-,_ .
			# ./ : : : : :'-, :: | :: :: :: _,,-''''- : ,---'' : : : : : : : : : : : / : : : : : : :''-
			# / : : : : : -, :-'''''''''''- : : _,,---'' : : : : : : : : : : : : : :| : : : : : : : : :
			# . : : : : : : :-''------------''' : : : : : : : : : : : : : : : : : : | : : : : : : : : :
			obj = trapdoors[0]
			#print "pix position = %d, %d" % (obj.px, obj.py)
			assert obj.pw/16 == 4 and obj.ph/16 == 4, "Trapdoor must be 4x4 tiles: " + self.map.id
			respawn = obj.props.get("respawn", "")
			m = EXP_LOCATION.match(respawn)
			if m is None:
				# assuming we have a location
				assert respawn in self.map.location_dict
				self.trapRespawnRoomId = self.map.location_dict[respawn].rid
			else:
				self.trapRespawnRoomId = int(m.group(1)) + int(m.group(2)) * map.width
			self.trapx = (obj.px / 16) - self.x * 8 + 2
			self.trapy = (obj.py / 16) - self.y * 8 + 2
			self.its_a_switch = False
		else:
			# switches?
			switches = [ obj for obj in map.raw.objects if obj.type == "switch" and self.contains_obj(obj) ]
			self.its_a_switch = len(switches) > 0
			if self.its_a_switch:
				obj = switches[0]
				self.switchx = (obj.px / 16) - self.x * 8 + 2
				self.switchy = (obj.py / 16) - self.y * 8 + 2
				assert obj.pw/16 == 4 and obj.ph/16 == 4, "Switch must be 4x4 tiles: " + self.map.id
				compute_trigger_event_id(self, obj)

	def contains_obj(self, obj):
		cx = obj.px + obj.pw/2
		cy = obj.py + obj.ph/2;
		px = 128 * self.x
		py = 128 * self.y
		return cx >= px and cy >= py and cx < px+128 and cy < py+128

	def hasitem(self): return len(self.item) > 0 and self.item != "ITEM_NONE"
	def tileat(self, x, y): return self.map.background.tileat(8*self.x + x, 8*self.y + y)
	def iswalkable(self, x, y): return iswalkable(self.tileat(x, y), 8*self.x + x, 8*self.y + y)
	def overlaytileat(self, x, y): return self.map.overlay.tileat(8*self.x + x, 8*self.y + y)

	def resolve_trigger_event_id(self):
		if self.its_a_switch:
			resolve_trigger_event_id(self, self.map)

	def primary_center(self):
		if self.its_a_trap: return (self.trapx, self.trapy)
		if self.its_a_switch: return (self.switchx, self.switchy)
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
		return (0,0)
		
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

	def write_tiles_to(self, src):
		src.write("    {{")
		for ty,tx in product(range(8),range(8)):
			src.write("0x%x," % self.tileat(tx,ty).lid)
		src.write("}},\n")

	def write_telem_source_to(self, src):
		src.write("    {0x%x,0x%x,{" % self.primary_center())
		for row in range(8):
			rowMask = 0
			for col in range(8):
				if not self.iswalkable(col, row):
					rowMask |= (1<<col)
			src.write("0x%x," % rowMask)
		src.write("}},\n")

