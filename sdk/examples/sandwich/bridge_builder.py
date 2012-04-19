import os, os.path, lxml, lxml.etree, Image, sys, misc, tmx, itertools

def bake_tileset(imgpath):
	# verify image selected
	try:
		img = Image.open(imgpath)
	except:
		print "Selected file is not an image:", imgpath
	# map image to tmxs
	dname,fname = os.path.split(imgpath)
	tiled_maps = ( tmx.Map(os.path.join(dname, path)) for path in os.listdir(dname) if path.lower().endswith(".tmx") )
	tiled_maps = ( mp for mp in tiled_maps if "background" in mp.layer_dict and "overlay" in mp.layer_dict ) 
	tiled_maps = [ mp for mp in tiledd_maps if mp.layer_dict["background"].gettileset().imgpath == fname ] 

	if len(tiled_maps) == 0:
		print "Selected file is not a background image:", imgpath

	# verify that all bgs, fgs match
	for m0,m1 in itertools.product(tiled_maps, tiled_maps):
		if m0.layer_dict["overlay"].imgpath != m1.layer_dict["overlay"]:
			pass



	# render all the maps
	# append missing rendered tiles to bg and update maps
	# save images and paths
	


if __name__ == "__main__":
	test_path = "/Users/max/src/thundercracker/sdk/examples/sandwich/Content/skytemple_background.png"
	if os.path.exists(test_path):
		bake_tileset(test_path)
	else:
		import Tkinter, tkFileDialog
		Tkinter.Tk().withdraw() # hide the main tk window
		file_path = tkFileDialog.askopenfilename(
			title		= "Open Interactions Image", 
			filetypes	= [("interactions image",".png")]
		)
		if len(file_path) > 0: bake_tileset(file_path) 
