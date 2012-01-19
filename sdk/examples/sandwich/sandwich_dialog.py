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
		dialogs = [Dialog(i,node) for i,node in enumerate(doc.findall("dialog"))]
		if len(dialogs) > 1:
			for i,d in enumerate(dialogs[:-1]):
				for otherd in dialogs[i+1:]:
					assert d.id != otherd.id, "duplicate dialog id"
		# todo: validate dialog images
		self.dialogs = dict((d.id, d) for d in dialogs)

	def list_npc_image_names(self):
		hash = {}
		for dialog in self.dialogs.itervalues():
			if not dialog.npc in hash:
				yield dialog.npc
				hash[dialog.npc] = True
		
	
	def list_detail_image_names(self):
		hash = {}
		for dialog in self.dialogs.itervalues():
			for text in dialog.texts:
				if not text.image in hash:
					yield text.image
					hash[text.image] = True

class Dialog:
	def __init__(self, index, xml):
		self.id = xml.get("id").lower()
		self.index = index
		self.npc = xml.get("npc")
		self.texts = [DialogText(self, i, elem) for i,elem in enumerate(xml.findall("text"))]

class DialogText:
	def __init__(self, dialog, index, xml):
		self.dialog = dialog
		self.index = index
		self.text = xml.text
		# validate text length (prerender?)
		self.image = xml.get("image")
