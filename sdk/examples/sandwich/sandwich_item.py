import lxml.etree, os, os.path, re, tmx, misc, Image

STORAGE_TYPE_INVENTORY = 0
STORAGE_TYPE_KEY =  1
STORAGE_TYPE_EQUIP =  2
NAME_TO_STORAGE_TYPE = {
	"inventory": STORAGE_TYPE_INVENTORY,
	"key": STORAGE_TYPE_KEY,
	"equipment": STORAGE_TYPE_EQUIP
}

class ItemDatabase:
	def __init__(self, world, path):
		self.world = world
		self.path = path
		doc = lxml.etree.parse(path)
		self.items = [ Item(xml) for xml in doc.findall("item") ]
		for i,item in enumerate(self.items):
			item.numeric_id = i
		for st in NAME_TO_STORAGE_TYPE.itervalues():
			for i,item in enumerate((item for item in self.items if item.storage_type == st)):
				item.storage_id = i
		self.item_dict = dict((item.id, item) for item in self.items)


	def write(self, src):
		src.write("const InventoryData gInventoryData[] = { \n")
		for item in self.items:
			src.write("  { \"%s\", 0x%x, 0x%x },\n" % (item.description, item.storage_type, item.storage_id))
		src.write("};\n\n")

class Item:
	def __init__(self, xml):
		self.id = xml.get("id").lower()
		self.name = xml.find("name").text.strip()
		self.description = "\\n".join((line.strip() for line in xml.find("description").text.strip().splitlines()))
		typeid = xml.get("type").lower()
		assert typeid in NAME_TO_STORAGE_TYPE, "undefined item type: " + typeid
		self.storage_type = NAME_TO_STORAGE_TYPE[xml.get("type")]
		
		
