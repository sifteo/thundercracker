import lxml.etree, os, os.path, re, tmx, misc, Image

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
		self.detail_images = dict((name, DialogDetailImage(os.path.join(world.dir, name+".png"))) for name in self.list_detail_image_names())

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

class DialogDetailImage:
	def __init__(self, path):
		self.image = Image.open(path)
		w,h = self.image.size
		assert w<=112 and h<=80, "NPC Details currently restricted to 112x80 crappy dimensions"
		# assert h<=80, "detail image is taller than 80px"
		# assert w%8 == 0 and h%8 == 0, "detail image dimensions not multiples of 8"
		# w = w>>3
		# h = h>>3
		# pix = self.image.load()
		# self.mask_pitch = h
		# self.mask = [ False for x in range(w) for y in range(h) ]
		# nonblank_count = 0
		# for row in range(h):
		# 	for col in range(w):
		# 		for y in range(8):
		# 			for x in range(8):
		# 				if pix[8*col + x, 8*row + y][3] != 0:
		# 					self.mask[self.mask_pitch * row + col] = True
		# 					nonblank_count += 1
		# 					break
		# 					break
		# 					break
		# assert nonblank_count<=144, "bg1 detail images can only contain 144 tiles"
		

