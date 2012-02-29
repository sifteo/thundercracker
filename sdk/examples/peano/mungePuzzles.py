import xml.dom.minidom
#import uuid
import struct

def op(text):
	return ["Add", "Subtract", "Multiply", "Divide"].index(text)

def side(text):
	return ["TOP", "LEFT", "BOTTOM", "RIGHT"].index(text)

def guidToInts(s):
	return struct.unpack("IIII", uuid.UUID(s).bytes)

#def writeGuid(s, f):
    #	guid = uuid.UUID(s)
	#bytes = guid.bytes
	#int32s = struct.unpack("IIII", bytes)
	#hexes = map(hex, int32s)
	#hexes = map(lambda a:a.replace("L",""), hexes)
    #f.write( "{"+hexes[0] + ", " + hexes[1] + ", " + hexes[2] + ", " + hexes[3]+"}" )

	

#write out the header file
header = open("puzzles.gen.h", "wt")
header.write("#pragma once\n")
header.write("#include \"assets.gen.h\"\n" )
header.write("#include \"sifteo.h\"\n" )

header.write("namespace TotalsGame\n{\nnamespace TheData\n{\n" )

header.write("struct PuzzleDef\n")
header.write("{\n")
header.write("\t/* const */ uint8_t *tokens;\n")
header.write("\t/* const */ uint8_t nTokens;\n")
header.write("\t/* const */ uint16_t *groups;\n")
header.write("\t/* const */ uint8_t nGroups;\n")
#header.write("\t/* const */ uint32_t guid[4];\n")
header.write("};\n")

header.write("struct ChapterDef\n")
header.write("{\n")
header.write("\t/* const */ PuzzleDef **puzzles;\n")
header.write("\t/* const */ uint8_t nPuzzles;\n")
header.write("\tconst AssetImage &icon;\n")
header.write("\tconst char *name;\n")
#header.write("\t/* const */ uint32_t guid[4];\n")
header.write("};\n")

header.write("struct EverythingDef\n")
header.write("{\n")
header.write("\t/* const */ ChapterDef **chapters;\n")
header.write("\t/* const */ uint8_t nChapters;\n")
header.write("};\n")

header.write("extern /* const */ EverythingDef everything;\n")

header.write("}\n}\n")

#do all the fancy stuff here

f = open("puzzles.gen.cpp", "wt")

f.write("#include \"puzzles.gen.h\"\n")

f.write("namespace TotalsGame\n{\nnamespace TheData\n{\n" )

document = xml.dom.minidom.parse("puzzles.xml")
chapters = document.getElementsByTagName("chapters")[0].getElementsByTagName("chapter")

chapterIndex = 0

for chapter in chapters:

	id = chapter.getAttribute("id")
	name = chapter.getAttribute("name")
	guid = chapter.getAttribute("guid")

	puzzleIndex = 0

	puzzles = chapter.getElementsByTagName("puzzle")
	for puzzle in puzzles:

		puzzleGuid = puzzle.getAttribute("guid")
		tokens = puzzle.getElementsByTagName("token")
		groups = puzzle.getElementsByTagName("group")

		f.write( "/* const */ uint8_t "+"c"+str(chapterIndex)+"p"+str(puzzleIndex)+"tokens[] =\n{\n\t" )
		for token in tokens:
			val = int(token.getAttribute("val"))
			opRight = op(token.getAttribute("opRight"))
			opBottom = op(token.getAttribute("opBottom"))
			packedByte = val | opRight<<4 | opBottom<<6
			f.write( hex(packedByte) + ", ", )
		f.write( "\n};\n" )

		f.write( "/* const */ uint16_t "+"c"+str(chapterIndex)+"p"+str(puzzleIndex)+"groups[] =\n{\n\t" )
		for group in groups:
			src = int(group.getAttribute("src"))
			srcToken = int(group.getAttribute("srcToken"))
			srcSide = side(group.getAttribute("srcSide"))
			dst = int(group.getAttribute("dst"))
			dstToken = int(group.getAttribute("dstToken"))
			packedShort = src*10+srcToken | srcSide<<7 | dst*10+dstToken<<9
			f.write( hex(packedShort) + ", ",  )
		f.write( "\n};\n" )


		f.write( "/* const */ PuzzleDef c" + str(chapterIndex) + "puzzle" + str(puzzleIndex) +" ="+ "\n{\n" )
		f.write( "\t c"+str(chapterIndex)+"p"+str(puzzleIndex)+"tokens"+ ", " )
		f.write( str(len(tokens)) + ", " )
		f.write( "c"+str(chapterIndex)+"p"+str(puzzleIndex)+"groups"+ ", " )
		f.write( str(len(groups)) + ", " )
#		writeGuid(puzzleGuid, f)
		f.write("\n};\n" )

		puzzleIndex = puzzleIndex + 1

	f.write( "/* const */ PuzzleDef *c" + str(chapterIndex) + "puzzles[]" +" ="+ "\n{\n\t" )
	for i in range(len(puzzles)):
		f.write("&c"+str(chapterIndex) + "puzzle" + str(i) +", ")
	f.write( "\n};\n" )


	f.write( "/* const */ ChapterDef chapter" + str(chapterIndex) + " =" + "\n{\n" )
	f.write( "\tc"+str(chapterIndex)+"puzzles" + ", ")
	f.write( str(len(puzzles)) + ", ")
	f.write( "icon_" + id + ", \"" + name + "\"" + ", ")
#	writeGuid(guid, f)
	f.write("\n};\n" )


	chapterIndex = chapterIndex + 1

f.write( "/* const */ ChapterDef *chapters[] =\n{\n\t" )
for i in range(len(chapters)):
	f.write("&chapter"+str(i) +", ")
f.write( "\n};\n" )

f.write( "/* const */ EverythingDef everything" + " =" + "\n{\n" )
f.write( "\tchapters"+", " )
f.write( str(len(chapters)) + "\n};\n" )


f.write("}\n}\n" )

f.close()
header.close()
