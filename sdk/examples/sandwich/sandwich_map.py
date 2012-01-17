import lxml.etree
import os
import os.path
import re
import tmx
import misc

# constants
PORTAL_OPEN = 0
PORTAL_WALL = 1
PORTAL_DOOR = 2
PORTAL_LABELS = [ "PORTAL_OPEN", "PORTAL_WALL", "PORTAL_DOOR" ]
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
		if not os.path.exists(path[:-4]+"_blank.png"):
			raise Exception("Map missing blank image: %s_blank.png", self.id)
		self.raw = tmx.Map(path)
		if not "background" in self.raw.layer_dict:
			raise Exception("Map does not contain background layer: %s" % self.id)
		self.background = self.raw.layer_dict["background"]
		if self.background.gettileset().count >= 256:
			raise Exception("Map is too large (must have < 256 tiles): %s" % self.id)
		self.overlay = self.raw.layer_dict.get("overlay", None)
		self.width = self.raw.pw / 128
		self.height = self.raw.ph / 128
		self.count = self.width * self.height
		self.rooms = [Room(self, i) for i in range(self.count)]

		# infer portal states (part of me reeeally wants this to be more clever)
		for r in self.rooms:
			if r.isblocked():
				r.portals[0] = PORTAL_WALL
				r.portals[1] = PORTAL_WALL
				r.portals[2] = PORTAL_WALL
				r.portals[3] = PORTAL_WALL
				continue
			tx,ty = (8*r.x, 8*r.y)
			# top
			prevType = r.portals[0] = PORTAL_WALL
			for i in range(8):
				typ = portal_type(self.background.tileat(tx+i, ty))
				if typ == PORTAL_WALL:
					pass
				elif typ == PORTAL_OPEN:
					if prevType == PORTAL_OPEN:
						r.portals[0] = PORTAL_OPEN
						break
				else:
					r.portals[0] = typ
					break
				prevType = typ
			if r.portals[0] != PORTAL_WALL and (r.y == 0 or self.roomat(r.x, r.y-1).isblocked()):
				r.portals[0] = PORTAL_WALL
			# left
			prevType = r.portals[1] = PORTAL_WALL
			for i in range(8):
				typ = portal_type(self.background.tileat(tx, ty+i))
				if typ == PORTAL_WALL:
					pass
				elif typ == PORTAL_OPEN:
					r.portals[1] = PORTAL_OPEN
					break
				else:
					r.portals[i] =typ
					break
				prevType = typ
			if r.portals[1] != PORTAL_WALL and (r.x == 0 or self.roomat(r.x-1, r.y).isblocked()):
				r.portals[1] = PORTAL_WALL
			# bottom
			prevType = r.portals[2] = PORTAL_WALL
			for i in range(8):
				typ = portal_type(self.background.tileat(tx+i, ty+7))
				if typ == PORTAL_WALL:
					pass
				elif typ == PORTAL_OPEN:
					if prevType == PORTAL_OPEN:
						r.portals[2] = PORTAL_OPEN
						break
				else:
					r.portals[2] = typ
					break
				prevType = typ
			if r.portals[2] != PORTAL_WALL and (r.y == self.height-1 or self.roomat(r.x, r.y+1).isblocked()):
				r.portals[2] = PORTAL_WALL
			# right
			prevType = r.portals[3] = PORTAL_WALL
			for i in range(8):
				typ = portal_type(self.background.tileat(tx+7, ty+i))
				if typ == PORTAL_WALL:
					pass
				elif typ == PORTAL_OPEN:
					r.portals[3] = PORTAL_OPEN
				else:
					r.portals[3] = typ
					break
				prevType = typ
			if r.portals[3] != PORTAL_WALL and (r.x == self.width-1 or self.roomat(r.x+1, r.y).isblocked()):
				r.portals[3] = PORTAL_WALL
		# validate portals
		for y in range(self.height):
			if self.roomat(0,y).portals[1] == PORTAL_OPEN or self.roomat(self.width-1, y).portals[3] == PORTAL_OPEN:
				raise Exception("Boundary Wall Error in Map: %s" % self.id)
		for x in range(self.width):
			if self.roomat(x,0).portals[0] == PORTAL_OPEN or self.roomat(x, self.height-1).portals[2] == PORTAL_OPEN:
				raise Exception("Boundary Wall Error in Map: %s" % self.id)
		for x in range(self.width-1):
			for y in range(self.height):
				if self.roomat(x,y).portals[3] != self.roomat(x+1,y).portals[1]:
					raise Exception ("Portal Mismatch in Map: %s" % self.id)
		for x in range(self.width):
			for y in range(self.height-1):
				if self.roomat(x,y).portals[2] != self.roomat(x,y+1).portals[0]:
					raise Exception ("Portal Mismatch in Map: %s" % self.id)
		# unpack triggers
		for obj in self.raw.objects:
			if obj.type in KEYWORD_TO_TRIGGER_TYPE:
				room = self.roomatpx(obj.px, obj.py)
				trig = Trigger(room, obj)
				if trig.id in room.trigger_dict:
					raise Exception("Multiple triggers with the Same Name in the Same Room for map: " + self.id)
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

				
	
	def roomat(self, x, y): return self.rooms[x + y * self.width]
	def roomatpx(self, px, py): return self.roomat(px/128, py/128)
	def list_triggers_of_type(self, type): return (t for r in self.rooms for t in r.triggers if t.type == type)
	
	def write_source_to(self, src):
		src.write("//--------------------------------------------------------\n")
		src.write("// EXPORTED FROM %s.tmx\n" % self.id)
		src.write("//--------------------------------------------------------\n\n")
		src.write("static const uint8_t %s_xportals[] = { " % self.id)
		for y in range(self.height):
			for x in range(self.width):
				src.write("%s, " % hex(self.roomat(x,y).portals[SIDE_LEFT]))
			src.write("%s, " % hex(self.roomat(self.width-1,y).portals[SIDE_RIGHT]))
		src.write("};\n")
		src.write("static const uint8_t %s_yportals[] = { " % self.id)
		for x in range(self.width):
			for y in range(self.height):
				src.write("%s, " % hex(self.roomat(x,y).portals[SIDE_TOP]))
			src.write("%s, " % hex(self.roomat(x, self.height-1).portals[SIDE_BOTTOM]))
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
			"%(name)s_xportals, %(name)s_yportals, %(item)s, %(gate)s, %(npc)s, " \
			"%(nitems)d, %(ngates)d, %(nnpcs)d, %(w)d, %(h)d },\n" % \
			{ 
				"name": self.id,
				"overlay": "&Overlay_" + self.id if self.overlay is not None else "0",
				"item": self.id + "_items" if len(self.item_dict) > 0 else "0",
				"gate": self.id + "_gateways" if len(self.gate_dict) > 0 else "0",
				"npc": self.id + "_npcs" if len(self.npc_dict) > 0 else "0",
				"w": self.width,
				"h": self.height,
				"nitems": len(self.item_dict),
				"ngates": len(self.gate_dict),
				"nnpcs": len(self.npc_dict)
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
		src.write("        {\n")
		for ty in range(8):
			src.write("            ")
			for tx in range(8):
				src.write("%s, " % hex(self.tileat(tx,ty).lid))
			src.write("\n")
		src.write("        },\n")
		# overlay
		if self.hasoverlay():
			src.write("        %s_overlay_%d_%d, " % (self.map.id, self.x, self.y))
		else:
			src.write("        0, ")
		# centerx, centery, reserved
		cx,cy = self.center()
		src.write("%d, %d, \n" % (cx, cy))

		# torches - should be moved out to a flat list like items / etc
		#torchcount = 0
		#for tx,ty in self.listtorchtiles():
		#	if torchcount == 2:
		#		raise Exception("Too man torches in one room (capacity 2): "  + self.id)
		#	src.write("        %s,\n" % hex((tx<<4) + ty))
		#	torchcount+=1
		#for i in range(2-torchcount):
		#	src.write("        0xff,\n")
		src.write("    },\n")		

class Trigger:
	def __init__(self, room, obj):
		self.id = obj.name.lower()
		self.room = room
		self.raw = obj
		self.type = KEYWORD_TO_TRIGGER_TYPE[obj.type]
		# deterine quest state
		if "quest" in obj.props:
			self.quest = room.map.world.script.getquest(obj.props["quest"])
			self.minquest = self.quest
			self.maxquest = self.quest
			if "questflag" in obj.props:
				self.qflag = self.quest.flag_dict[obj.props["questflag"]]
		else:
			self.quest = None
			if "minquest" in obj.props:
				self.minquest = room.map.world.script.getquest(obj.props["minquest"])
			if "maxquest" in obj.props:
				self.maxquest = room.map.world.script.getquest(obj.props["maxquest"])
			if hasattr(self, "minquest") and hasattr(self, "maxquest"):
				if self.minquest.index > self.maxquest.index:
					raise Exception("MaxQuest > MinQuest for object in map: " + room.map.id)
		if "unlockflag" in obj.props:
			if not obj.props["unlockflag"] in room.map.world.script.unlockables_dict:
				raise Exception("Undefined unlock flag in map: " + room.map.id)
			self.unlockflag = room.map.world.script.unlockables_dict[obj.props["unlockflag"]]
		

		if self.type == TRIGGER_ITEM:
			self.itemid = int(obj.props["id"])
			if self.quest is not None:
				self.qflag = self.quest.flag_dict[self.id]
			else:
				self.unlockflag = room.map.world.script.unlockables_dict[self.id]
		elif self.type == TRIGGER_GATEWAY:
			m = EXP_GATEWAY.match(obj.props.get("target", ""))
			if m is None:
				raise Exception("Malformed Gateway Target in Map: " + room.map.id)
			self.target_map = m.group(1).lower()
			self.target_gate = m.group(2).lower()
		elif self.type == TRIGGER_NPC:
			did = obj.props["id"].lower()
			if not did in room.map.world.dialog.dialogs:
				raise Exception("Invalid Dialog ID in Map: " + room.map.id)
			self.dialog = room.map.world.dialog.dialogs[did]
		
				
		self.qbegin = self.minquest.index if hasattr(self, "minquest") else 0xff
		self.qend = self.maxquest.index if hasattr(self, "maxquest") else 0xff
		self.flagid = 0
		if hasattr(self, "qflag"): self.flagid = 1 + self.qflag.index
		elif hasattr(self, "unlockflag"): self.flagid = 33 + self.unlockflag.index


	
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
		x = self.raw.px + (self.raw.pw >> 1) - 128 * self.room.x
		y = self.raw.py + (self.raw.ph >> 1) - 127 * self.room.y
		src.write(", %s, %s, %s, %s }, " % (hex(mapid), hex(gateid), hex(x), hex(y)))
	
	def write_npc_to(self, src):
		src.write("{ ")
		self.write_trigger_to(src)
		src.write(", %d }, " % self.dialog.index)



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
		return PORTAL_WALL
	else:
		return PORTAL_OPEN
