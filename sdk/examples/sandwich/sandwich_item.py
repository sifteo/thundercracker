import lxml.etree, os, posixpath, re, tmx, misc, Image

STORAGE_TYPE_INVENTORY = 0
STORAGE_TYPE_EQUIP =  1
NAME_TO_STORAGE_TYPE = {
	"inventory": STORAGE_TYPE_INVENTORY,
	"equipment": STORAGE_TYPE_EQUIP
}

ITEM_TRIGGER_NONE = 0
ITEM_TRIGGER_KEY = 1
ITEM_TRIGGER_BOOT = 2
ITEM_TRIGGER_BOMB = 3

NAME_TO_ITEM_TRIGGER = {
	"key": ITEM_TRIGGER_KEY,
	"boot": ITEM_TRIGGER_BOOT,
	"bomb": ITEM_TRIGGER_BOMB,
	"": ITEM_TRIGGER_NONE
}

class ItemDatabase:
	def __init__(self, world, path):
		self.world = world
		self.path = path
		doc = lxml.etree.parse(path)
		self.items = [ Item(i,xml) for i,xml in enumerate(doc.findall("item")) ]
		self.item_dict = dict((item.id, item) for item in self.items)


	def write(self, src):
		src.write("const ItemTypeData gItemTypeData[] = { \n")
		for item in self.items:
			src.write("  { \"%s\", 0x%x, 0x%x },\n" % (item.description, item.storage_type, item.trigger_type))
		src.write("};\n\n")

class Item:
	def __init__(self, numeric_id, xml):
		self.numeric_id = numeric_id
		self.id = xml.get("id").lower()
		self.name = xml.find("name").text.strip()
		self.description = "\\n".join((line.strip() for line in xml.find("description").text.strip().splitlines()))
		self.storage_type = NAME_TO_STORAGE_TYPE[xml.get("storage").lower()]
		self.trigger_type = NAME_TO_ITEM_TRIGGER[xml.get("trigger", "").lower()]
		
		
