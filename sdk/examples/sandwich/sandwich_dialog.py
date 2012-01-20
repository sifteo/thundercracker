import lxml.etree
import os
import os.path
import re
import tmx
import misc

class DialogDatabase:
	def __init__(self, world, path):
		self.world = world
		self.path = path
		doc = lxml.etree.parse(path)
		self.dialogs = [Dialog(i,node) for i,node in enumerate(doc.findall("dialog"))]
		if len(self.dialogs) > 1:
			for i,d in enumerate(self.dialogs[:-1]):
				for otherd in self.dialogs[i+1:]:
					assert d.id != otherd.id, "duplicate dialog id"
		# todo: validate dialog images
		self.dialog_dict = dict((d.id, d) for d in self.dialogs)

	def list_npc_image_names(self):
		hash = {}
		for dialog in self.dialogs:
			if not dialog.npc in hash:
				yield dialog.npc
				hash[dialog.npc] = True
		
	
	def list_detail_image_names(self):
		hash = {}
		for dialog in self.dialogs:
			for text in dialog.lines:
				if not text.image in hash:
					yield text.image
					hash[text.image] = True

class Dialog:
	def __init__(self, index, xml):
		self.id = xml.get("id").lower()
		self.index = index
		self.npc = xml.get("npc")
		self.lines = [DialogText(self, i, elem) for i,elem in enumerate(xml.findall("text"))]

class DialogText:
	def __init__(self, dialog, index, xml):
		self.dialog = dialog
		self.index = index
		self.text = "\\n".join(( line.strip() for line in xml.text.strip().splitlines() ))
		#print self.text
		# validate text length (prerender?)
		self.image = xml.get("image")
