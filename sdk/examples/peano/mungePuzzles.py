import xml.dom.minidom
#import uuid
import struct

def tokensName(chapter, puzzle):
	return "c"+str(chapter)+"p"+str(puzzle)+"tokens"

def groupsName(chapter, puzzle):
	return "c"+str(chapter)+"p"+str(puzzle)+"groups"

def puzzlesName(chapter):
	return "c" + str(chapterIndex) + "puzzles" 



def op(text):
	return ["Add", "Subtract", "Multiply", "Divide"].index(text)

def side(text):
	return ["TOP", "LEFT", "BOTTOM", "RIGHT"].index(text)


#write out the header file
header = open("puzzles.gen.h", "wt")
header.write("#pragma once\n")
header.write("#include \"assets.gen.h\"\n" )
header.write("#include \"sifteo.h\"\n" )

header.write("namespace TotalsGame\n{\nnamespace TheData\n{\n" )

header.write("struct PuzzleDef\n")
header.write("{\n")
header.write("\t/* const */ uint8_t *tokens;\n")
header.write("\t/* const */ uint16_t *groups;\n")
header.write("\t/* const */ uint8_t nTokens;\n")
header.write("\t/* const */ uint8_t nGroups;\n")
header.write("};\n")

header.write("struct ChapterDef\n")
header.write("{\n")
header.write("\t/* const */ PuzzleDef *puzzles;\n")
header.write("\tconst char *name;\n")
header.write("\tconst Sifteo::AssetImage &icon;\n")
header.write("\t/* const */ uint8_t nPuzzles;\n")
header.write("};\n")

header.write("struct EverythingDef\n")
header.write("{\n")
header.write("\t/* const */ ChapterDef *chapters;\n")
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

chapterArray = ""

chapterIndex = 0

for chapter in chapters:

	id = chapter.getAttribute("id")
	name = chapter.getAttribute("name")
	guid = chapter.getAttribute("guid")

	puzzleArray = ""

	puzzleIndex = 0

	puzzles = chapter.getElementsByTagName("puzzle")
	for puzzle in puzzles:

		puzzleGuid = puzzle.getAttribute("guid")
		tokens = puzzle.getElementsByTagName("token")
		groups = puzzle.getElementsByTagName("group")

		f.write( "/* const */ uint8_t "+ tokensName(chapterIndex, puzzleIndex) + "[] =\n{\n\t" )
		for token in tokens:
			val = int(token.getAttribute("val"))
			opRight = op(token.getAttribute("opRight"))
			opBottom = op(token.getAttribute("opBottom"))
			packedByte = val | opRight<<4 | opBottom<<6
			f.write( hex(packedByte) + ", ", )
		f.write( "\n};\n" )

		f.write( "/* const */ uint16_t "+ groupsName(chapterIndex, puzzleIndex) + "[] =\n{\n\t" )
		for group in groups:
			src = int(group.getAttribute("src"))
			srcToken = int(group.getAttribute("srcToken"))
			srcSide = side(group.getAttribute("srcSide"))
			dst = int(group.getAttribute("dst"))
			dstToken = int(group.getAttribute("dstToken"))
			packedShort = src*10+srcToken | srcSide<<7 | dst*10+dstToken<<9
			f.write( hex(packedShort) + ", ",  )
		f.write( "\n};\n" )

		puzzleArray += "\t" + "{\n" 
		puzzleArray += "\t\t" + tokensName(chapterIndex, puzzleIndex) + ",\n"
		puzzleArray += "\t\t" + groupsName(chapterIndex, puzzleIndex) + ",\n"
		puzzleArray += "\t\t" + str(len(tokens)) + ",\n"
		puzzleArray += "\t\t" + str(len(groups)) + "\n"
		puzzleArray += "\t" + "},\n"

		puzzleIndex = puzzleIndex + 1

	f.write( "/* const */ PuzzleDef " + puzzlesName(chapter) + "[] =\n{\n" + puzzleArray + "};\n" )

	chapterArray += "\t" + "{\n" 
	chapterArray += "\t\t" + puzzlesName(chapterIndex) + ",\n"
	chapterArray += "\t\t" + "\"" + name + "\",\n"
	chapterArray += "\t\t" + "icon_" + id + ",\n" 
	chapterArray += "\t\t" + str(len(puzzles)) + "\n"
	chapterArray += "\t" + "}, "

	chapterIndex = chapterIndex + 1



f.write( "/* const */ ChapterDef chapters[] = \n{\n" + chapterArray + "\n};\n" )


f.write( "/* const */ EverythingDef everything" + " =" + "\n{\n" )
f.write( "\tchapters"+", " )
f.write( str(len(chapters)) + "\n};\n" )


f.write("}\n}\n" )

f.close()
header.close()
