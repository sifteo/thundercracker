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

class Map:
	def __init__(self, world, path):
		self.world = world
		self.id = os.path.basename(path)[:-4].lower()
		assert os.path.exists(path[:-4]+"_blank.png"), "Map missing blank image: " + self.id
		self.raw = tmx.Map(path)
		assert "background" in self.raw.layer_dict, "Map does not contain background layer: " + self.id
		self.background = self.raw.layer_dict["background"]
		assert self.background.gettileset().count < 256, "Map is too large (must have < 256 tiles): " + self.id
		self.overlay = self.raw.layer_dict.get("overlay", None)
		self.width = self.raw.pw / 128
		self.height = self.raw.ph / 128
		self.count = self.width * self.height
		self.quest = world.script.getquest(self.raw.props["quest"]) if "quest" in self.raw.props else None
		self.rooms = [Room(self, i) for i in range(self.count)]
		# infer portals
		for r in self.rooms:
			if r.isblocked(): continue
			tx,ty = (8*r.x, 8*r.y)
			if r.y != 0 and not self.roomat(r.x, r.y-1).isblocked():
				ports = [ portal_type(self.background.tileat(tx+i, ty)) for i in range(8) ]
				r.portals[SIDE_TOP] = PORTAL_DOOR if PORTAL_DOOR in ports else PORTAL_OPEN if PORTAL_OPEN in ports else PORTAL_WALL
			if r.x != 0 and not self.roomat(r.x-1, r.y).isblocked():
				ports = [ portal_type(self.background.tileat(tx, ty+i)) for i in range(8) ]
				r.portals[SIDE_LEFT] = PORTAL_DOOR if PORTAL_DOOR in ports else PORTAL_OPEN if PORTAL_OPEN in ports else PORTAL_WALL
			if r.y != self.height-1 and not self.roomat(r.x, r.y+1).isblocked():
				ports = [ portal_type(self.background.tileat(tx+i, ty+7)) for i in range(8) ]
				r.portals[SIDE_BOTTOM] = PORTAL_DOOR if PORTAL_DOOR in ports else PORTAL_OPEN if PORTAL_OPEN in ports else PORTAL_WALL
			if r.x != self.width-1 and not self.roomat(r.x+1, r.y).isblocked():
				ports = [ portal_type(self.background.tileat(tx+7, ty+i)) for i in range(8) ]
				r.portals[SIDE_RIGHT] = PORTAL_DOOR if PORTAL_DOOR in ports else PORTAL_OPEN if PORTAL_OPEN in ports else PORTAL_WALL
		# validate portals
		for r in self.rooms:
			assert r.portals[SIDE_LEFT] != PORTAL_DOOR, "Horizontal Door in Map: " + self.id
			assert r.portals[SIDE_RIGHT] != PORTAL_DOOR, "Horizontal Door in Map: " + self.id
		for x in range(self.width-1):
			for y in range(self.height):
				assert self.roomat(x,y).portals[3] == self.roomat(x+1,y).portals[1], "Portal Mismatch in Map: " + self.id
		for x in range(self.width):
			for y in range(self.height-1):
				assert self.roomat(x,y).portals[2] == self.roomat(x,y+1).portals[0], "Portal Mismatch in Map: " + self.id
		# unpack triggers
		for obj in self.raw.objects:
			if obj.type in KEYWORD_TO_TRIGGER_TYPE:
				room = self.roomatpx(obj.px, obj.py)
				trig = Trigger(room, obj)
				assert not trig.id in room.trigger_dict, "Duplicate Trigger IDs in the same room for map: " + self.id #extend map-wide?
				room.triggers.append(trig)
				room.trigger_dict[trig.id] = trig
		# assign trigger indices, build map-dicts
		self.item_dict = {}
		self.gate_dict = {}
		self.npc_dict = {}
		for i,item in enumerate(self.list_triggers_of_type(TRIGGER_ITEM)):
			item.index = i
			self.item_dict[item.id] = item
		for i,gate in enumerate(self.list_triggers_of_type(TRIGGER_GATEWAY)):
			gate.index = i
			self.gate_dict[gate.id] = gate
		for i,npc in enumerate(self.list_triggers_of_type(TRIGGER_NPC)):
			npc.index = i
			self.npc_dict[npc.id] = npc
		self.doors = [Door(r) for r in self.rooms if r.portals[0] == PORTAL_DOOR]
		for i,d in enumerate(self.doors):
			d.index = i
				
	
	def roomat(self, x, y): return self.rooms[x + y * self.width]
	def roomatpx(self, px, py): return self.roomat(px/128, py/128)
	def list_triggers_of_type(self, type): return (t for r in self.rooms for t in r.triggers if t.type == type)

	def write_source_to(self, src):
		src.write("//--------------------------------------------------------\n")
		src.write("// EXPORTED FROM %s.tmx\n" % self.id)
		src.write("//--------------------------------------------------------\n\n")
		src.write("static const uint8_t %s_xportals[] = { " % self.id)

		byte = 0
		cnt = 0
		for y in range(self.height):
			for x in range(self.width-1):
				if self.roomat(x,y).portals[SIDE_RIGHT] != PORTAL_WALL:
					byte = byte | (1 << cnt)
				cnt = cnt + 1
				if cnt == 8:
					cnt = 0
					src.write("%s, " % hex(byte))
					byte = 0
		if cnt > 0: src.write("%s, " % hex(byte))
		src.write("};\n")
		src.write("static const uint8_t %s_yportals[] = { " % self.id)
		byte = 0
		cnt = 0
		for x in range(self.width):
			for y in range(self.height-1):
				if self.roomat(x,y).portals[SIDE_BOTTOM] != PORTAL_WALL:
					byte = byte | (1 << cnt)
				cnt = cnt + 1
				if cnt == 8:
					cnt = 0
					src.write("%s, " % hex(byte))
					byte = 0
		if cnt > 0: src.write("%s, " % hex(byte))
		src.write("};\n")
		if len(self.item_dict) > 0:
			src.write("static const ItemData %s_items[] = { " % self.id)
			for item in self.list_triggers_of_type(TRIGGER_ITEM):
				item.write_item_to(src)
			src.write("};\n")
		
		if len(self.gate_dict):
			src.write("static const GatewayData %s_gateways[] = { " % self.id)
			for gate in self.list_triggers_of_type(TRIGGER_GATEWAY):
				gate.write_gateway_to(src)
			src.write("};\n")

		if len(self.npc_dict) > 0:
			src.write("static const NpcData %s_npcs[] = { " % self.id)
			for npc in self.list_triggers_of_type(TRIGGER_NPC):
				npc.write_npc_to(src)
			src.write("};\n")
		
		if len(self.doors) > 0:
			src.write("static const DoorData %s_doors[] = { " % self.id)
			for door in self.doors:
				src.write("{ %s, %s }, " % (hex(door.room.lid), hex(door.flag.gindex)))
			src.write("};\n")

		if self.overlay is not None:
			for room in self.rooms:
				if room.hasoverlay():
					src.write("static const uint8_t %s_overlay_%d_%d[] = { " % (self.id, room.x, room.y))
					for y in range(8):
						for x in range(8):
							tile = room.overlaytileat(x,y)
							if tile is not None:
								src.write("%s, %s, " % (hex(x<<4|y), hex(tile.lid)))
					src.write("0xff };\n")
		src.write("static const RoomData %s_rooms[] = {\n" % self.id)
		for y in range(self.height):
			for x in range(self.width):
				room = self.roomat(x,y)
				room.write_source_to(src)
		src.write("};\n")

	
	def write_decl_to(self, src):
		src.write(
			"    { &TileSet_%(name)s, %(overlay)s, &Blank_%(name)s, %(name)s_rooms, " \
			"%(name)s_xportals, %(name)s_yportals, %(item)s, %(gate)s, %(npc)s, %(door)s," \
			"%(nitems)d, %(ngates)d, %(nnpcs)d, %(doorQuestId)d, %(ndoors)d, %(w)d, %(h)d },\n" % \
			{ 
				"name": self.id,
				"overlay": "&Overlay_" + self.id if self.overlay is not None else "0",
				"item": self.id + "_items" if len(self.item_dict) > 0 else "0",
				"gate": self.id + "_gateways" if len(self.gate_dict) > 0 else "0",
				"npc": self.id + "_npcs" if len(self.npc_dict) > 0 else "0",
				"door": self.id + "_doors" if len(self.doors) > 0 else "0",
				"w": self.width,
				"h": self.height,
				"nitems": len(self.item_dict),
				"ngates": len(self.gate_dict),
				"nnpcs": len(self.npc_dict),
				"doorQuestId": self.quest.index if self.quest is not None else 0xff,
				"ndoors": len(self.doors)
			})
		


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

	def listtorchtiles(self):
		for x in range(8):
			for y in range(8):
				if istorch(self.tileat(x,y)) and (y == 0 or not istorch(self.tileat(x,y-1))):
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
			src.write("%s, " % hex(rowMask))
		src.write("},\n")
		# tiles
		src.write("        { ")
		for ty in range(8):
			#src.write("            ")
			for tx in range(8):
				src.write("%s, " % hex(self.tileat(tx,ty).lid))
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
		src.write("{ %s, %s, %s, %s }" % (hex(self.qbegin), hex(self.qend), hex(self.flagid), hex(self.room.lid)))

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
		src.write(", %s, %s, %s, %s }, " % (hex(mapid), hex(gateid), hex(x), hex(y)))
	
	def write_npc_to(self, src):
		src.write("{ ")
		self.write_trigger_to(src)
		x,y = self.local_position()
		src.write(", %s, %s, %s }, " % (hex(self.dialog.index), hex(x), hex(y)))



def istorch(tile): return "torch" in tile.props
def isobst(tile): return "obstacle" in tile.props or istorch(tile)
def iswall(tile): return "wall" in tile.props
def isdoor(tile): return "door" in tile.props
def isopen(tile): return not isdoor(tile) and not isobst(tile) and not iswall(tile)
def iswalkable(tile): return not iswall(tile) and not isobst(tile)
def ispath(tile): return "path" in tile.props

def portal_type(tile):
	if isdoor(tile):
		return PORTAL_DOOR
	elif iswall(tile) or isobst(tile):
		return PORTAL_WALL
	else:
		return PORTAL_OPEN
