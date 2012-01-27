import lxml.etree, os, os.path, re, tmx, misc, math

# constants
PORTAL_OPEN = 0
PORTAL_WALL = 1
PORTAL_DOOR = 2
SIDE_TOP = 0
SIDE_LEFT = 1
SIDE_BOTTOM = 2
SIDE_RIGHT = 3
EXP_GATEWAY = re.compile(r"^(\w+):(\w+)$")
TRIGGER_GATEWAY = 0
TRIGGER_ITEM = 1
TRIGGER_NPC = 2
KEYWORD_TO_TRIGGER_TYPE = {
	"gateway": TRIGGER_GATEWAY,
	"item": TRIGGER_ITEM,
	"npc": TRIGGER_NPC
}


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

class Door:
	def __init__(self, room):
		self.id = "_door_%s_%d" % (room.map.id, room.lid)
		self.room = room
		room.door = self
		self.flag = room.map.quest.add_flag_if_undefined(self.id) \
			if room.map.quest is not None \
			else room.map.world.script.add_flag_if_undefined(self.id)

class AnimatedTile:
	def __init__(self, tile):
		self.tile = tile
		self.numframes = int(tile.props["animated"])
		assert self.numframes < 16, "tile animation too long (capacity 15)"

class Trigger:
	def __init__(self, room, obj):
		self.id = obj.name.lower()
		self.room = room
		self.raw = obj
		self.type = KEYWORD_TO_TRIGGER_TYPE[obj.type]
		# deterine quest state
		self.quest = None
		self.minquest = None
		self.maxquest = None
		self.qflag = None
		self.unlockflag = None
		if "quest" in obj.props:
			self.quest = room.map.world.script.getquest(obj.props["quest"])
			self.minquest = self.quest
			self.maxquest = self.quest
			if "questflag" in obj.props:
				self.qflag = self.quest.flag_dict[obj.props["questflag"]]
		else:
			self.minquest = room.map.world.script.getquest(obj.props["minquest"]) if "minquest" in obj.props else None
			self.maxquest = room.map.world.script.getquest(obj.props["maxquest"]) if "maxquest" in obj.props else None
			if self.minquest is not None and self.maxquest is not None:
				assert self.minquest.index <= self.maxquest.index, "MaxQuest > MinQuest for object in map: " + room.map.id
		if self.quest is None and self.minquest is None and self.maxquest is None and room.map.quest is not None:
			self.quest = room.map.quest
			self.minquest = room.map.quest
			self.maxquest = room.map.quest
			self.qflag = self.quest.add_flag_if_undefined(obj.props["questflag"]) if "questflag" in obj.props else None
		if self.quest is None and "unlockflag" in obj.props:
			self.unlockflag = room.map.world.script.add_flag_if_undefined(obj.props["unlockflag"])
		# type-specific initialization
		if self.type == TRIGGER_ITEM:
			self.itemid = int(obj.props["id"])
			if self.quest is not None:
				if self.qflag is None:
					self.qflag = self.quest.add_flag_if_undefined(self.id)
			elif self.unlockflag is None:
				self.unlockflag = room.map.world.script.add_flag_if_undefined(self.id)
		elif self.type == TRIGGER_GATEWAY:
			m = EXP_GATEWAY.match(obj.props.get("target", ""))
			assert m is not None, "Malformed Gateway Target in Map: " + room.map.id
			self.target_map = m.group(1).lower()
			self.target_gate = m.group(2).lower()
		elif self.type == TRIGGER_NPC:
			did = obj.props["id"].lower()
			assert did in room.map.world.dialog.dialog_dict, "Invalid Dialog ID in Map: " + room.map.id
			self.dialog = room.map.world.dialog.dialog_dict[did]
		
				
		self.qbegin = self.minquest.index if self.minquest is not None else 0xff
		self.qend = self.maxquest.index if self.maxquest is not None else 0xff
		if self.qflag is not None: self.flagid = self.qflag.gindex
		elif self.unlockflag is not None: self.flagid = self.unlockflag.gindex
		else: self.flagid = 0


	def is_active_for(self, quest):
		if self.qbegin != 0xff and self.qbegin < quest.index: return False
		if self.qend != 0xff and self.qend > quest.index: return False
		return True

	def local_position(self):
		return (self.raw.px + (self.raw.pw >> 1) - 128 * self.room.x, 
				self.raw.py + (self.raw.ph >> 1) - 128 * self.room.y)

	
	def write_trigger_to(self, src):
		src.write("{ 0x%x, 0x%x, 0x%x, 0x%x }" % (self.qbegin, self.qend, self.flagid, self.room.lid))

	def write_item_to(self, src):
		src.write("{ ")
		self.write_trigger_to(src)
		src.write(", %d }, " % self.itemid)
	
	def write_gateway_to(self, src):
		src.write("{ ")
		self.write_trigger_to(src)
		mapid = self.room.map.world.map_dict[self.target_map].index
		gateid = self.room.map.world.map_dict[self.target_map].gate_dict[self.target_gate].index
		x,y = self.local_position()
		src.write(", 0x%x, 0x%x, 0x%x, 0x%x }, " % (mapid, gateid, x, y))
	
	def write_npc_to(self, src):
		src.write("{ ")
		self.write_trigger_to(src)
		x,y = self.local_position()
		src.write(", 0x%x, 0x%x, 0x%x }, " % (self.dialog.index, x, y))

		
def iswalkable(tile): return "wall" not in tile.props and "obstacle" not in tile.props

def portal_type(tile):
	if "door" in tile.props:
		return PORTAL_DOOR
	elif "wall" in tile.props or "obstacle" in tile.props:
		return PORTAL_WALL
	else:
		return PORTAL_OPEN
