import lxml.etree, os, os.path, re, tmx, misc, Image

class ItemDatabase:
	def __init__(self, world, path):
		self.world = world
		self.path = path
		doc = lxml.etree.parse(path)
		self.items = [ Item(xml, i) for i,xml in enumerate(doc.findall("item")) ]
		self.item_dict = dict((item.id, item) for item in self.items)

	def write(self, src):
		src.write("const InventoryData gInventoryData[] = { \n")
		for item in self.items:
			src.write("  { \"%s\", \"%s\" },\n" % (item.name, item.description))
		src.write("};\n\n")

class Item:
	def __init__(self, xml, index):
		self.id = xml.get("id").lower()
		self.name = xml.find("name").text.strip()
		self.description = "\\n".join((line.strip() for line in xml.find("description").text.strip().splitlines()))
		self.numeric_id = index + 1
		#print self.description
