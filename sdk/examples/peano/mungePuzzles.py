import xml.dom.minidom
import uuid
import struct

def op(text):
	return ("Add", "Subtract", "Multiply", "Divide").index(text)

def side(text):
	return ("TOP", "LEFT", "BOTTOM", "RIGHT").index(text)

def guidToInts(s):
	return struct.unpack("IIII", uuid.UUID(s).bytes)

document = xml.dom.minidom.parse("puzzles.xml")
chapters = document.getElementsByTagName("chapters")[0].getElementsByTagName("chapter")
for chapter in chapters:

	id = chapter.getAttribute("id")
	name = chapter.getAttribute("name")
	guid = chapter.getAttribute("guid")

	print id, name, guid

	puzzles = chapter.getElementsByTagName("puzzle")
	for puzzle in puzzles:

		puzzleGuid = puzzle.getAttribute("guid")
		intguid = map(hex, guidToInts(puzzleGuid))
		print puzzleGuid, intguid

		tokens = puzzle.getElementsByTagName("token")
		groups = puzzle.getElementsByTagName("group")

		print "uint8_t tokens[]={",
		for token in tokens:
			val = int(token.getAttribute("val"))
			opRight = op(token.getAttribute("opRight"))
			opBottom = op(token.getAttribute("opBottom"))
			packedByte = val | opRight<<4 | opBottom<<6
			print hex(packedByte), ", ", 
		print "}"

		print "uint16_t groups[]={",
		for group in groups:
			src = int(group.getAttribute("src"))
			srcToken = int(group.getAttribute("srcToken"))
			srcSide = side(group.getAttribute("srcSide"))
			dst = int(group.getAttribute("dst"))
			dstToken = int(group.getAttribute("dstToken"))
			packedShort = src*10+srcToken | srcSide<<7 | dst*10+dstToken<<9
			print hex(packedShort), ", ",
		print "}"
