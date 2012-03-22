import lxml.etree, os, posixpath, re, tmx, misc, math
from sandwich_room import *
from itertools import product

class MapDatabase:
	def __init__(self, world, path):
		self.world = world
		self.path = path
		doc = lxml.etree.parse(path)
		maps = [ Map(self, xml) for xml in doc.findall("map")]
		maps_by_name = [ (m.id,m) for m in maps]
		maps_by_name.sort()
		self.maps = [ m for _,m in maps_by_name ]
		for i,m in enumerate(self.maps): m.index = i
		self.map_dict = dict((map.id, map) for map in self.maps)
		#validate maps
		assert len(self.maps) > 0
		# validate map links
		for map in self.maps:
			for gate in map.list_triggers_of_type(TRIGGER_GATEWAY):
				assert gate.target_map in self.map_dict, "gateway to unknown map: " + gate.target_map
				tmap = self.map_dict[gate.target_map]
				found = False
				for othergate in tmap.list_triggers_of_type(TRIGGER_GATEWAY):
					if othergate.id == gate.target_gate:
						found = True
						break
				assert found, "link to unknown map-gate: " + gate.target_gate
		# assign IDs to unique sokoblock assets
		self.sokoblock_assets = []
		for m in self.maps: 
			self.sokoblock_assets += [ block.asset for block in m.sokoblocks ]
		self.sokoblock_assets = list(set(self.sokoblock_assets))
		asset_to_id = dict( (asset,i) for i,asset in enumerate(self.sokoblock_assets) )
		for m in self.maps:
			for block in m.sokoblocks:
				block.asset_id = asset_to_id[block.asset]



class Door:
	def __init__(self, room):
		self.id = "_door_%s_%d" % (room.map.id, room.lid)
		self.room = room
		room.door = self
		self.flag = room.map.quest.add_flag_if_undefined(self.id) \
			if room.map.quest is not None \
			else room.map.world.quests.add_flag_if_undefined(self.id)

class AnimatedTile:
	def __init__(self, tile):
		self.tile = tile
		self.numframes = int(tile.props["animated"])
		assert self.numframes < 16, "tile animation too long (capacity 15)"

class Sokoblock:
	def __init__(self, map, obj):
		assert obj.pw/16 == 4 and obj.ph/16 == 4, "Sokoblocks must be 4x4 tiles"
		assert "asset" in obj.props, "Sokoblocks must have an asset property"
		self.obj = obj
		self.x = obj.px + 32
		self.y = obj.py + 32
		self.asset = obj.props["asset"]
		if self.asset.lower().endswith(".png"): self.asset = self.asset[0:-4]
		assert posixpath.exists(posixpath.join(map.world.dir, self.asset+".png"))

class Map:
	def __init__(self, db, xml):
		world = db.world
		path = posixpath.join(world.dir, xml.get("id")+".tmx")
		print "Reading Map: ", path
		self.world = world
		self.id = posixpath.basename(path)[:-4].lower()
		self.readable_name = xml.findtext("name")
		self.raw = tmx.Map(path)
		assert "background" in self.raw.layer_dict, "Map does not contain background layer: " + self.id
		self.background = self.raw.layer_dict["background"]
		assert self.background.gettileset().count < 256, "Map is too large (must have < 256 tiles): " + self.id
		# validate tiles
		for tile in (self.raw.gettile(tid) for tid in self.background.tiles):
			if "bridge" in tile.props: 
				assert iswalkable(tile), "unwalkable bridge tile detected in map: " + self.id
		self.animatedtiles = [ AnimatedTile(t) for t in self.background.gettileset().tiles if "animated" in t.props ]
		self.overlay = self.raw.layer_dict.get("overlay", None)

		self.background_id = posixpath.basename(self.background.gettileset().imgpath)
		if self.overlay is not None:
			self.overlay_id = posixpath.basename(self.overlay.gettileset().imgpath)

		self.width = self.raw.pw / 128
		self.height = self.raw.ph / 128
		self.count = self.width * self.height
		assert self.count > 0, "map is empty: " + self.id
		assert self.count <= 81, "too many rooms in map (max 72): " + self.id
		self.quest = world.quests.getquest(self.raw.props["quest"]) if "quest" in self.raw.props else None
		self.rooms = [Room(self, i) for i in range(self.count)]
		# infer portals
		for r in self.rooms:
			if r.isblocked(): continue
			tx,ty = (8*r.x, 8*r.y)
			dd = str(PORTAL_DOOR)+str(PORTAL_DOOR)
			oo = str(PORTAL_OPEN)+str(PORTAL_OPEN)
			if r.y != 0 and not self.roomat(r.x, r.y-1).isblocked():
				ports = "".join(( str(portal_type(self.background.tileat(tx+i, ty))) for i in range(1,7) ))
				r.portals[SIDE_TOP] = PORTAL_DOOR if dd in ports else PORTAL_OPEN if oo in ports else PORTAL_WALL
			if r.x != 0 and not self.roomat(r.x-1, r.y).isblocked():
				ports = [ portal_type(self.background.tileat(tx, ty+i)) for i in range(1,7) ]
				r.portals[SIDE_LEFT] = PORTAL_OPEN if PORTAL_OPEN in ports else PORTAL_WALL
			if r.y != self.height-1 and not self.roomat(r.x, r.y+1).isblocked():
				ports = "".join(( str(portal_type(self.background.tileat(tx+i, ty+7))) for i in range(1,7) ))
				r.portals[SIDE_BOTTOM] = PORTAL_DOOR if dd in ports else PORTAL_OPEN if oo in ports else PORTAL_WALL
			if r.x != self.width-1 and not self.roomat(r.x+1, r.y).isblocked():
				ports = [ portal_type(self.background.tileat(tx+7, ty+i)) for i in range(1,7) ]
				r.portals[SIDE_RIGHT] = PORTAL_OPEN if PORTAL_OPEN in ports else PORTAL_WALL
		# validate portals
		for r in self.rooms:
			assert r.portals[SIDE_LEFT] != PORTAL_DOOR, "Horizontal Door in Map: " + self.id
			assert r.portals[SIDE_RIGHT] != PORTAL_DOOR, "Horizontal Door in Map: " + self.id
		for x,y in product( range(self.width-1), range(self.height) ):
			assert self.roomat(x,y).portals[3] == self.roomat(x+1,y).portals[1], "Portal Mismatch in Map: " + self.id + ", room: " + str(x) + "," + str(y)
		for x,y in product( range(self.width), range(self.height-1) ):
			assert self.roomat(x,y).portals[2] == self.roomat(x,y+1).portals[0], "Portal Mismatch in Map: " + self.id + ", room: " + str(x) + "," + str(y)
		# find subdivisions
		for r in self.rooms: 
			r.find_subdivisions()
			if r.subdiv_type != SUBDIV_NONE:
				assert r.its_a_trap == False, "traps cannot be placed in subdivisions"
		self.diagRooms = [ r for r in self.rooms if r.subdiv_type == SUBDIV_DIAG_POS or r.subdiv_type == SUBDIV_DIAG_NEG ]
		self.bridgeRooms = [ r for r in self.rooms if r.subdiv_type == SUBDIV_BRDG_HOR or r.subdiv_type == SUBDIV_BRDG_VER]
		# no vertical bridges, for now
		#for bridge in self.bridgeRooms:
		#	assert bridge.subdiv_type == SUBDIV_BRDG_HOR, "no vertical bridges for now, dudes: " + self.id

		# validate animated tile capacity (max 4 per room)
		for r in self.rooms:
			assert len([(x,y) for x in range(8) for y in range(8) if "animated" in r.tileat(x,y).props]) <= 4, "too many tiles in room in map: " + self.id
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
		self.ambientType = 1 if "ambient" in self.raw.props else 0
		self.trapped_rooms = [ room for room in self.rooms if room.its_a_trap ]
		# find sokoblocks
		self.sokoblocks = [ Sokoblock(self, obj) for obj in self.raw.objects if obj.type == "sokoblock" ]
		assert len(self.sokoblocks) <= 8
		# find me some lava tiles
		self.lava_tiles = [ t for t in self.background.gettileset().tiles if "lava" in t.props ]
		assert len(self.lava_tiles) <= 8

	
	def roomat(self, x, y): return self.rooms[x + y * self.width]
	def roomatpx(self, px, py): return self.roomat(px/128, py/128)
	def list_triggers_of_type(self, type): return (t for r in self.rooms for t in r.triggers if t.type == type)

	def write_source_to(self, src):
		src.write("//--------------------------------------------------------\n")
		src.write("// EXPORTED FROM %s.tmx\n" % self.id)
		src.write("//--------------------------------------------------------\n\n")
		src.write("static const uint8_t %s_xportals[] = {" % self.id)

		byte = 0
		cnt = 0
		for y,x in product( range(self.height), range(self.width-1) ):
			if self.roomat(x,y).portals[SIDE_RIGHT] != PORTAL_WALL:
				byte = byte | (1 << cnt)
			cnt = cnt + 1
			if cnt == 8:
				cnt = 0
				src.write("0x%x," % byte)
				byte = 0
		if cnt > 0: src.write("0x%x," % byte)
		src.write("};\n")
		src.write("static const uint8_t %s_yportals[] = {" % self.id)
		byte = 0
		cnt = 0
		for x,y in product( range(self.width), range(self.height-1) ):
			if self.roomat(x,y).portals[SIDE_BOTTOM] != PORTAL_WALL:
				byte = byte | (1 << cnt)
			cnt = cnt + 1
			if cnt == 8:
				cnt = 0
				src.write("0x%x," % byte)
				byte = 0
		if cnt > 0: src.write("0x%x," % byte)
		src.write("};\n")
		if len(self.item_dict) > 0:
			src.write("static const ItemData %s_items[] = {" % self.id)
			for item in self.list_triggers_of_type(TRIGGER_ITEM):
				item.write_item_to(src)
			src.write("};\n")
		
		if len(self.gate_dict):
			src.write("static const GatewayData %s_gateways[] = {" % self.id)
			for gate in self.list_triggers_of_type(TRIGGER_GATEWAY):
				gate.write_gateway_to(src)
			src.write("};\n")

		if len(self.npc_dict) > 0:
			src.write("static const NpcData %s_npcs[] = {" % self.id)
			for npc in self.list_triggers_of_type(TRIGGER_NPC):
				npc.write_npc_to(src)
			src.write("};\n")
		
		if len(self.trapped_rooms) > 0:
			src.write("static const TrapdoorData %s_trapdoors[] = {" % self.id)
			for room in self.trapped_rooms:
				src.write("{0x%x,0x%x}," % (room.lid, room.trapRespawnRoomId))
			src.write("};\n")
		
		if len(self.doors) > 0:
			src.write("static const DoorData %s_doors[] = {" % self.id)
			for door in self.doors:
				src.write("{0x%x,0x%x}," % (door.room.lid, door.flag.gindex))
			src.write("};\n")

		if len(self.animatedtiles) > 0:
			src.write("static const AnimatedTileData %s_animtiles[] = {" % self.id)
			for atile in self.animatedtiles:
				src.write("{0x%x,0x%x}," % (atile.tile.lid, atile.numframes))
			src.write("};\n")

		if len(self.sokoblocks) > 0:
			src.write("static const SokoblockData %s_sokoblocks[] = {" % self.id)
			for b in self.sokoblocks:
				src.write("{0x%x,0x%x,0x%x}," % (b.x, b.y, b.asset_id))
			src.write("};\n")

		if len(self.lava_tiles) > 0:
			src.write("static const TileSetID %s_lavatiles[] = {" % self.id)
			for tile in self.lava_tiles:
				src.write("0x%x," % tile.lid)
			src.write("0x0};\n")

		if self.overlay is not None:
			src.write("static const uint8_t %s_overlay_rle[] = {" % self.id)
			emptycount = 0
			for t in (r.overlaytileat(tid%8,tid/8) for r in self.rooms for tid in range(64)):
				if t is not None:
					while emptycount >= 0xff:
						src.write("0xff,0xff,")
						emptycount -= 0xff
					if emptycount > 0:
						src.write("0xff,0x%x," % emptycount)
						emptycount = 0
					src.write("0x%x, " % t.lid)
				else:
					emptycount += 1
			while emptycount >= 0xff:
				src.write("0xff,0xff,")
				emptycount -= 0xff
			if emptycount > 0:
				src.write("0xff,0x%x," % emptycount)
				emptycount = 0
			src.write("};\n")
		if len(self.diagRooms) > 0:
			src.write("static const DiagonalSubdivisionData %s_diag[] = {" % self.id)
			for r in self.diagRooms:
				is_pos = 0 if r.subdiv_type == SUBDIV_DIAG_NEG else 1
				cx,cy = r.secondary_center()
				src.write("{0x%x,0x%x,0x%x,0x%x}," % (is_pos, r.lid, cx, cy))
			src.write("};\n")
		if len(self.bridgeRooms) > 0:
			src.write("static const BridgeSubdivisionData %s_bridges[] = {" % self.id)
			for r in self.bridgeRooms:
				is_hor = 0 if r.subdiv_type == SUBDIV_BRDG_VER else 1
				cx,cy = r.secondary_center()
				src.write("{0x%x,0x%x,0x%x,0x%x}," % (is_hor, r.lid, cx, cy))
			src.write("};\n")

		src.write("static const RoomData %s_rooms[] = {\n" % self.id)
		for y in range(self.height):
			for x in range(self.width):
				room = self.roomat(x,y)
				room.write_source_to(src)
		src.write("};\n")

	
	def write_decl_to(self, src):
		src.write(
			"    { \"%(readable_name)s\", " \
			"&TileSet_%(bg)s, " \
			"%(overlay)s, " \
			"%(name)s_rooms, " \
			"%(overlay_rle)s, " \
			"%(name)s_xportals, " \
			"%(name)s_yportals, " \
			"%(item)s, " \
			"%(gate)s, " \
			"%(npc)s, " \
			"%(trapdoor)s, " \
			"%(door)s, " \
			"%(animtiles)s, " \
			"%(lavatiles)s, " \
			"%(diagsubdivs)s, " \
			"%(bridgesubdivs)s, " \
			"%(sokoblocks)s, "
			"0x%(nitems)x, " \
			"0x%(ngates)x, " \
			"0x%(nnpcs)x, " \
			"0x%(ntrapdoors)x, " \
			"0x%(doorQuestId)x, " \
			"0x%(ndoors)x, " \
			"0x%(nanimtiles)x, " \
			"0x%(ndiags)x, " \
			"0x%(nbridges)x, " \
			"0x%(nsokoblocks)x, " \
			"0x%(ambient)x, " \
			"0x%(w)x, " \
			"0x%(h)x },\n" % \
			{ 
				"readable_name": self.readable_name,
				"name": self.id,
				"bg": posixpath.splitext(self.background_id)[0],
				"overlay": "&Overlay_" + posixpath.splitext(self.overlay_id)[0] if self.overlay is not None else "0",
				"overlay_rle": self.id + "_overlay_rle" if self.overlay is not None else "0",
				"item": self.id + "_items" if len(self.item_dict) > 0 else "0",
				"gate": self.id + "_gateways" if len(self.gate_dict) > 0 else "0",
				"npc": self.id + "_npcs" if len(self.npc_dict) > 0 else "0",
				"trapdoor": self.id + "_trapdoors" if len(self.trapped_rooms) > 0 else "0",
				"door": self.id + "_doors" if len(self.doors) > 0 else "0",
				"animtiles": self.id + "_animtiles" if len(self.animatedtiles) > 0 else "0",
				"diagsubdivs": self.id + "_diag" if len(self.diagRooms) > 0 else "0",
				"bridgesubdivs": self.id + "_bridges" if len(self.bridgeRooms) > 0 else "0",
				"w": self.width,
				"h": self.height,
				"nitems": len(self.item_dict),
				"ngates": len(self.gate_dict),
				"nnpcs": len(self.npc_dict),
				"ntrapdoors": len(self.trapped_rooms),
				"doorQuestId": self.quest.index if self.quest is not None else 0xff,
				"ndoors": len(self.doors),
				"nanimtiles": len(self.animatedtiles),
				"ndiags": len(self.diagRooms),
				"nbridges": len(self.bridgeRooms),
				"ambient": self.ambientType,
				"sokoblocks": self.id + "_sokoblocks" if len(self.sokoblocks) > 0 else "0",
				"nsokoblocks": len(self.sokoblocks),
				"lavatiles": self.id + "_lavatiles" if len(self.lava_tiles) > 0 else "0"
			})


def portal_type(tile):
	if "door" in tile.props:
		return PORTAL_DOOR
	elif "wall" in tile.props or "obstacle" in tile.props:
		return PORTAL_WALL
	else:
		return PORTAL_OPEN
