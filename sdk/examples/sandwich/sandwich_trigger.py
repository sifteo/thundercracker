import lxml.etree, os, posixpath, re, tmx, misc, math

EXP_ACTION_TARGET = re.compile(r"^(\w+):(\w+)$")
EXP_LOCATION = re.compile(r"^(\d+),(\d+)$")

TRIGGER_KEYWORDS = [ "gateway", "item", "npc", "door" ]
KEYWORD_TO_TRIGGER_TYPE = dict((name,i) for i,name in enumerate(TRIGGER_KEYWORDS))

EVENT_NONE = 0
EVENT_ADVANCE_QUEST_AND_REFRESH = 1
EVENT_ADVANCE_QUEST_AND_TELEPORT = 2
EVENT_OPEN_DOOR = 3

KEYWORD_TO_TRIGGER_EVENT = {
	"advancequestandrefresh": EVENT_ADVANCE_QUEST_AND_REFRESH,
	"advancequestandteleport": EVENT_ADVANCE_QUEST_AND_TELEPORT,
	"opendoor": EVENT_OPEN_DOOR
}

def compute_trigger_event_id(target, obj):
	if "ontrigger" in obj.props:
		triggerEventName = obj.props["ontrigger"]
		m = EXP_ACTION_TARGET.match(triggerEventName)
		if m is not None:
			triggerEventName = m.group(1).lower()
			target.event_id_str = m.group(2).lower()
		else:
			triggerEventName = triggerEventName.lower()
			target.event_id_str = ""
		assert triggerEventName in KEYWORD_TO_TRIGGER_EVENT
		target.event = KEYWORD_TO_TRIGGER_EVENT[triggerEventName]
	else:
		target.event = EVENT_NONE
		target.event_id_str = ""

def resolve_trigger_event_id(target, map):
	if target.event == EVENT_OPEN_DOOR:
		door_dict = map.trig_dict["door"]
		assert target.event_id_str in door_dict, "undefined door: " + target.event_id_str
		target.event_id = door_dict[target.event_id_str].index
	else:
		target.event_id = 0


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
			self.quest = room.map.world.quests.getquest(obj.props["quest"])
			self.minquest = self.quest
			self.maxquest = self.quest
			if "questflag" in obj.props:
				self.qflag = self.quest.flag_dict[obj.props["questflag"]]
		else:
			self.minquest = room.map.world.quests.getquest(obj.props["minquest"]) if "minquest" in obj.props else None
			self.maxquest = room.map.world.quests.getquest(obj.props["maxquest"]) if "maxquest" in obj.props else None
			if self.minquest is not None and self.maxquest is not None:
				assert self.minquest.index <= self.maxquest.index, "MaxQuest > MinQuest for object in map: " + room.map.id
		if self.quest is None and self.minquest is None and self.maxquest is None and room.map.quest is not None:
			self.quest = room.map.quest
			self.minquest = room.map.quest
			self.maxquest = room.map.quest
			self.qflag = self.quest.add_flag_if_undefined(obj.props["questflag"]) if "questflag" in obj.props else None
		if self.quest is None and "unlockflag" in obj.props:
			self.unlockflag = room.map.world.quests.add_flag_if_undefined(obj.props["unlockflag"])
		# type-specific initialization
		
		if obj.type == "item":
			itemid = obj.props["id"]
			assert itemid in room.map.world.items.item_dict, "Item is undefined (" + itemid + " ) in map: " + room.map.id
			self.item = room.map.world.items.item_dict[itemid]
			self.alloc_flag()
		
		elif obj.type == "gateway":
			m = EXP_ACTION_TARGET.match(obj.props.get("target", ""))
			assert m is not None, "Malformed Gateway Target in Map: " + room.map.id
			self.target_map = m.group(1).lower()
			self.target_gate = m.group(2).lower()
		
		elif obj.type == "npc":
			did = obj.props["id"].lower()
			assert did in room.map.world.dialogs.dialog_dict, "Invalid Dialog ID (" + did + ") in Map: " + room.map.id
			self.dialog = room.map.world.dialogs.dialog_dict[did]
			self.optional = obj.props.get("required", "false").lower() == "true"

		elif obj.type == "door":
			if "key" in obj.props:
				itemid = obj.props["key"].lower()
				assert itemid in room.map.world.items.item_dict, "Item is undefined (" + itemid + " ) in map: " + room.map.id
				self.key_item = room.map.world.items.item_dict[itemid]
				assert self.key_item.can_be_a_key(), "Only equipment can be used as keys"
			else:
				self.key_item = None
			self.alloc_flag()

				
		self.qbegin = self.minquest.index if self.minquest is not None else 0xff
		self.qend = self.maxquest.index if self.maxquest is not None else 0xff
		if self.qflag is not None: self.flagid = self.qflag.gindex
		elif self.unlockflag is not None: self.flagid = self.unlockflag.gindex
		else: self.flagid = 0

		compute_trigger_event_id(self, obj)


	def alloc_flag(self):
		if self.quest is not None:
			if self.qflag is None:
				self.qflag = self.quest.add_flag_if_undefined(self.id)
		elif self.unlockflag is None:
			self.unlockflag = self.room.map.world.quests.add_flag_if_undefined(self.id)

	def resolve_trigger_event_id(self):
		resolve_trigger_event_id(self, self.room.map)

	def is_active_for(self, quest):
		if self.qbegin != 0xff and self.qbegin < quest.index: return False
		if self.qend != 0xff and self.qend > quest.index: return False
		return True

	def local_position(self):
		return (self.raw.px + (self.raw.pw >> 1) - 128 * self.room.x, 
				self.raw.py + (self.raw.ph >> 1) - 128 * self.room.y)

	
	def write_trigger_to(self, src):
		src.write("{0x%x,0x%x,0x%x,0x%x,0x%x,0x%x}" % (
			self.qbegin, 
			self.qend, 
			self.flagid, 
			self.room.lid, 
			self.event, 
			self.event_id
		))

	def write_item_to(self, src):
		src.write("{")
		self.write_trigger_to(src)
		src.write(",0x%x}," % self.item.numeric_id)
	
	def write_gateway_to(self, src):
		src.write("{")
		self.write_trigger_to(src)
		target_map = self.room.map.world.maps.map_dict[self.target_map]
		mapid = target_map.index
		if self.target_gate in target_map.trig_dict["gateway"]:
			targettype = 0
			targetid = target_map.trig_dict["gateway"][self.target_gate].index
		else:
			targettype = 1
			targetid = target_map.location_dict[self.target_gate].rid
		x,y = self.local_position()
		src.write(",0x%x,0x%x,0x%x,0x%x,0x%x}," % (mapid, targettype, targetid, x, y))
	
	def write_npc_to(self, src):
		src.write("{")
		self.write_trigger_to(src)
		x,y = self.local_position()
		src.write(",0x%x,0x%x,0x%x,0x%x}," % (self.dialog.index, 1 if self.optional else 0, x, y))

	def write_door_to(self, src):
		src.write("{")
		self.write_trigger_to(src)
		src.write(",0x%x}," % (0xff if self.key_item is None else self.key_item.numeric_id))

