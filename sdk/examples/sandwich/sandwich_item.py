import lxml.etree, os, os.path, re, tmx, misc, Image

class ItemDatabase:
	def __init__(self, world, path):
		self.world = world
		self.path = path
		doc = lxml.etree.parse(path)
		self.items = [ Item(i,xml) for i,xml in enumerate(doc.findall("item")) ]
		self.item_dict = dict((item.id, item) for item in self.items)

class Item:
	def __init__(self, index, xml):
		self.index = index
		self.icon = int(xml.get("icon", ""))
		self.id = xml.get("id").lower()
		self.name = xml.find("name").text.strip()
		self.description = "\\n".join((line.strip() for line in xml.find("description").text.strip().splitlines()))
		print self.description
