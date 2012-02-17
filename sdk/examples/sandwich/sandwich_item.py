import lxml.etree, os, os.path, re, tmx, misc, Image

class ItemDatabase:
	def __init__(self, world, path):
		self.world = world
		self.path = path
		doc = lxml.etree.parse(path)
		items = [ Item(xml) for xml in doc.findall("item") ]
		self.items = [ None for _ in range(len(items)) ]
		for item in items:
			assert item.icon < len(items), "Icon out of range"
			self.items[item.icon] = item
		
		self.item_dict = dict((item.id, item) for item in items)



class Item:
	def __init__(self, xml):
		self.icon = int(xml.get("icon", ""))
		self.id = xml.get("id").lower()
		self.name = xml.find("name").text.strip()
		self.description = "\\n".join((line.strip() for line in xml.find("description").text.strip().splitlines()))
		print self.description
