# implementation of sandwich kingdom content specification

import lxml.etree
import os
import os.path
import re
import tmx
import misc

# constants
PORTAL_OPEN = 0
PORTAL_WALLED = 1
PORTAL_DOOR = 2
SIDE_TOP = 0
SIDE_LEFT = 1
SIDE_BOTTOM = 2
SIDE_RIGHT = 3
EXP_REGULAR = re.compile(r"^([a-z]+)$")
EXP_PLUS = re.compile(r"^([a-z]+)\+(\d+)$")
EXP_MINUS = re.compile(r"^([a-z]+)\-(\d+)$")
EXP_GATEWAY = re.compile(r"^(\w+):(\w+)$")
TRIGGER_GATEWAY = 0
TRIGGER_ITEM = 1
TRIGGER_NPC = 2
KEYWORD_TO_TRIGGER_TYPE = {
	"gateway": TRIGGER_GATEWAY,
	"item": TRIGGER_ITEM,
	"npc": TRIGGER_NPC
}

def load():
	return World(".")

def export():
	World(".").export()

class World:
	def __init__(self, dir):
		self.script = GameScript(self, os.path.join(dir, "game-script.xml"))
		self.dialog = DialogDatabase(self, os.path.join(dir, "dialog-database.xml"))
		self.maps = dict((path[0:-4], Map(self, os.path.join(dir, path))) for path in os.listdir(dir) if path.endswith(".tmx"))
		#validate maps
		if len(self.maps) == 0:
			raise Exception("No maps!")
		# validate map links
		for map in self.maps.itervalues():
			for gate in (t for t in map.list_triggers() if t.type == TRIGGER_GATEWAY):
				if not gate.target_map in self.maps:
					raise Exception("Link to non-existent map: " + gate.target_map)
				tmap = self.maps[gate.target_map]
				found = False
				for othergate in (t for t in tmap.list_triggers() if t.type == TRIGGER_GATEWAY):
					if othergate.name == gate.target_gate:
						found = True
						break
				if not found:
					raise Exception("Link to non-existent gate: " + gate.target_gate)


	def export():
		# todo
		pass


#------------------------------------------------------------------------------
# GAME SCRIPT
#------------------------------------------------------------------------------

class GameScript:
	def __init__(self, world, path):
		self.world = world
		self.path = path
		xml = lxml.etree.parse(path)
		self.quests = [Quest(i, elem) for i,elem in enumerate(xml.findall("quest"))]
		if len(self.quests) > 1:
			for index,quest in enumerate(self.quests[0:-1]):
				for otherQuest in self.quests[index+1:]:
					if quest.id == otherQuest.id:
						raise Exception("Duplicate Quest ID")
		self.quest_dict = dict((quest.id, quest) for quest in self.quests)
	
	def getquest(self, name):
		m = EXP_REGULAR.match(name)
		if m is not None:
			return self.quest_dict[name]
		m = EXP_PLUS.match(name)
		if m is not None:
			return self.quests[self.quest_dict[m.group(1)].index + int(m.group(2))]
		m = EXP_MINUS.match(name)
		if m is not None:
			return self.quests[self.quest_dict[m.group(1)].index - int(m.group(2))]
		raise Exception("Invalid Quest Identifier: " + name)


class Quest:
	def __init__(self, index, xml):
		self.index = index
		self.id = xml.get("id").lower()
		self.flags = [QuestFloat(i, elem) for i,elem in enumerate(xml.findall("flag"))]
		if len(self.flags) > 32:
			raise Exception("Too Many Flags for Quest: %s" % self.id)
		if len(self.flags) > 1:
			for index,flag in enumerate(self.flags[0:-1]):
				for otherFlag in self.flags[index+1:]:
					if flag.id == otherFlag.id:
						raise Exception("Duplicate Quest Flag ID")
		self.flag_dict = dict((flag.id,flag) for flag in self.flags)

		def getflag(self, name):
			if name in self.flag_dict:
				return self.flag_dict[name]
			raise Exception("Invalid Quest Flag Identifier: " + name)


class QuestFlag:
	def __init__(self, index, xml):
		self.index = index
		self.id = xml.get("id").lower()

#------------------------------------------------------------------------------
# DIALOG DATABASE
#------------------------------------------------------------------------------

class DialogDatabase:
	def __init__(self, world, path):
		self.world = world
		self.path = path
		doc = lxml.etree.parse(path)
		dialogs = [Dialog(node) for node in doc.findall("dialog")]
		if len(dialogs) > 1:
			for i,d in enumerate(dialogs[:-1]):
				for otherd in dialogs[i+1:]:
					if d.id == otherd.id:
						raise Exception("Duplicate Dialog ID")
		# todo: validate dialog images
		self.dialogs = dict((d.id, d) for d in dialogs)

	def list_npc_image_paths(self):
		hash = {}
		for dialog in self.dialogs:
			if not dialog.npc in hash:
				yield dialog.npc + ".png"
				hash[dialog.npc] = True
		
	
	def list_detail_image_paths(self):
		hash = {}
		for dialog in self.dialogs:
			for text in dialog.texts:
				if not text.image in hash:
					yield text.image + ".png"
					hash[text.image] = True

class Dialog:
	def __init__(self, xml):
		self.id = xml.get("id").lower()
		self.npc = xml.get("npc")
		self.texts = [DialogText(self, i, elem) for i,elem in enumerate(xml.findall("text"))]

class DialogText:
	def __init__(self, dialog, index, xml):
		self.dialog = dialog
		self.index = index
		self.text = xml.text
		self.image = xml.get("image")


#------------------------------------------------------------------------------
# MAP DATABASE
#------------------------------------------------------------------------------

class Map:
	def __init__(self, world, path):
		self.world = world
		self.name = os.path.basename(path)[:-4]
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
		# unpack triggers
		for obj in self.raw.objects:
			if obj.type in KEYWORD_TO_TRIGGER_TYPE:
				room = self.roomatpx(obj.px, obj.py)
				trig = Trigger(room, obj)
				if trig.name in room.triggers:
					raise Exception("Multiple triggers with the Same Name in the Same Room")
				room.triggers[trig.name] = trig
		# validate triggers
		# todo

				
	
	def roomat(self, x, y): return self.rooms[x + y * self.width]
	def roomatpx(self, px, py): return self.roomat(px/128, py/128)
	
	def list_triggers(self):
		for r in self.rooms:
			for t in r.triggers.itervalues():
				yield t

class Room:
	def __init__(self, map, lid):
		self.map = map
		self.lid = lid
		self.x = lid % map.width
		self.y = lid / map.width
		self.portals = [ PORTAL_WALLED, PORTAL_WALLED, PORTAL_WALLED, PORTAL_WALLED ]
		self.triggers = {}

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

# Trigger Types (screw inheretance - they all "quack" like a trigger)

class Trigger:
	def __init__(self, room, obj):
		self.name = obj.name
		self.room = room
		self.raw = obj
		self.type = KEYWORD_TO_TRIGGER_TYPE[obj.type]
		# deterine quest state
		if "quest" in obj.props:
			self.quest = room.map.world.script.getquest(obj.props["quest"])
			if "questflag" in obj.props:
				self.qflag = self.quest.getflag(obj.props["questflag"])
		else:
			if "minquest" in obj.props:
				self.minquest = room.map.world.script.getquest(obj.props["minquest"])
			if "maxquest" in obj.props:
				self.maxquest = room.map.world.script.getquest(obj.props["maxquest"])
			if hasattr(self, "minquest") and hasattr(self, "maxquest"):
				if self.minquest.index > self.maxquest.index:
					raise Exception("MaxQuest > MinQuest for object in map: " + room.map.name)
		if self.type == TRIGGER_ITEM:
			self.itemid = int(obj.props["id"])
			# validate?
		elif self.type == TRIGGER_GATEWAY:
			m = EXP_GATEWAY.match(obj.props.get("target", ""))
			if m is None:
				raise Exception("Malformed Gateway Target in Map: " + room.map.name)
			self.target_map = m.group(1)
			self.target_gate = m.group(2)
		elif self.type == TRIGGER_NPC:
			did = obj.props["id"]
			if not did in room.map.world.dialog.dialogs:
				raise Exception("Invalid Dialog ID in Map: " + room.map.name)
			self.dialog = room.map.world.dialog.dialogs[did]

#------------------------------------------------------------------------------
# HELPERS
#------------------------------------------------------------------------------

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
