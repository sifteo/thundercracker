#!/usr/bin/env python
"""
script to convert JSON puzzles in CubeBuddies format to a .h file that will be built into the game.  
"""

####################################################################################################
####################################################################################################

import sys
import json

####################################################################################################
# Utility
####################################################################################################

def MakeSep():
    sep = ''
    for i in range(100):
        sep += '/'
    sep += '\n'
    return sep        

def Id(container, key):
    return key.upper() + '_' + container[key].upper()

def BuddyToId(name):
    return 'BUDDY_' + name.upper()

def BoolToString(value):
    if value:
        return 'true'
    else:
        return 'false'

####################################################################################################
####################################################################################################

validated = True
buddies = ['gluv', 'suli', 'rike', 'boff', 'zorg', 'maro']
views = ['right', 'left', 'front']
sides = ['top', 'left', 'bottom', 'right']
parts = ['hair', 'eye_left', 'mouth', 'eye_right']

####################################################################################################
####################################################################################################

def CheckHasProperty(data, prop):
    if not data.has_key(prop):
        print 'Property Missing: "%s"' % prop
        global validated
        validated = False
        return False
    else:
        return True

def CheckIsUnsigned(data, prop):
    if data[prop] < 0:
        print 'Property Error: "%s" must be >= 0 (it is %d)' % (prop, data[prop])
        global validated
        validated = False
        return False
    else:
        return True

def CheckIsNotNull(data, prop):
    if data[prop] == None:
        print 'Propery Error: "%s" cannot be null (it is %s)' % (prop, data[prop])
        global validated
        validated = False
        return False
    else:
        return True

def CheckHasLength(data, prop):
    if len(data[prop]) == 0:
        print 'Property Error: "%s" cannot have a length of 0' % prop
        global validated
        validated = False
        return False
    else:
        return True

def CheckIsMember(l, v):
    if v not in l:
        print 'Property Error: "%s" must be one of the following %s' % (v, l)
        global validated
        validated = False
        return False
    else:
        return True
        
####################################################################################################
####################################################################################################

def CheckCutscene(puzzle, cutscene):
    if CheckHasProperty(puzzle, cutscene):
        if CheckHasProperty(puzzle[cutscene], 'buddies'):
            if CheckHasLength(puzzle[cutscene], 'buddies'):
                for buddy in puzzle[cutscene]['buddies']:
                    CheckIsMember(buddies, buddy)
        if CheckHasProperty(puzzle[cutscene], 'lines'):
            if CheckHasLength(puzzle[cutscene], 'lines'):
                for line in puzzle[cutscene]['lines']:
                    if CheckHasProperty(line, 'speaker'):
                        if line['speaker'] != 0 and line['speaker'] != 1:
                            print 'Property Error: "speaker" must be 0 or 1 (it is %d)' % line['speaker']
                            global validated
                            validated = False
                        CheckHasProperty(line, 'view')
                        CheckIsMember(views, line['view'])
                        CheckHasProperty(line, 'text')
                        CheckIsNotNull(line, 'text')

####################################################################################################
####################################################################################################

def CheckPieces(pieces, check_solve):
    global validated
    if len(pieces) != 4:
        print 'Property Error: pieces must have 4 keys: %s' % sides
        validated = False
    else:
        for side in pieces:
            if CheckIsMember(sides, side):
                if CheckHasProperty(pieces[side], 'buddy'):
                    CheckIsMember(buddies, pieces[side]['buddy'])
                if CheckHasProperty(pieces[side], 'part'):
                    CheckIsMember(parts, pieces[side]['part'])
                if check_solve:
                    if CheckHasProperty(pieces[side], 'solve'):
                        if pieces[side]['solve'] is not True and pieces[side]['solve'] is not False:
                            print 'Property Error: "solve" must be either true or false (it is ' + pieces[side]['solve'] + ')'
                            validated = False

####################################################################################################
####################################################################################################

def CheckBuddy(puzzle, buddy):
    if CheckHasProperty(puzzle, buddy):
        if CheckHasProperty(puzzle[buddy], 'pieces_start'):
            CheckPieces(puzzle[buddy]['pieces_start'], False)
        if CheckHasProperty(puzzle[buddy], 'pieces_end'):
            CheckPieces(puzzle[buddy]['pieces_end'], True)

####################################################################################################
####################################################################################################

def ValidateData(data):
    for puzzle in data:
        # book
        if CheckHasProperty(puzzle, 'book'):
            CheckIsUnsigned(puzzle, 'book')
        
        # title
        if CheckHasProperty(puzzle, 'title'):
            CheckIsNotNull(puzzle, 'title')
        
        # clue
        if CheckHasProperty(puzzle, 'clue'):
            CheckIsNotNull(puzzle, 'clue')
        
        # cutscene_environment
        if CheckHasProperty(puzzle, 'cutscene_environment'):
            CheckIsUnsigned(puzzle, 'cutscene_environment')
        
        # cutscene_start
        CheckCutscene(puzzle, 'cutscene_start')
        CheckCutscene(puzzle, 'cutscene_end')
        
        # shuffles
        if CheckHasProperty(puzzle, 'shuffles'):
            CheckIsUnsigned(puzzle, 'shuffles')
        
        # buddies
        if CheckHasProperty(puzzle, 'buddies'):
            for buddy in puzzle['buddies']:
                CheckBuddy(puzzle['buddies'], buddy)

####################################################################################################
####################################################################################################

def ValidatePuzzles(src):  
    with open(src, 'r') as f:
        j = json.load(f)
        
        # Bail if our version doesn't jive with the parser.
        # TODO: Version upgrades.
        version = j['version']
        if version != 1:
            print "%s is a not supported version." % src
            exit(1)
        
        data = j['puzzles']
        # TODO: validate top level
        
        ValidateData(data)
        if not validated:
            exit(1)
        
def ConvertPuzzles(src, dest):  
    with open(src, 'r') as f:
        data = json.load(f)['puzzles']
        
        with open(dest, 'w') as fout:
            # Comment Separator
            sep = MakeSep()
            
            # File Header
            fout.write(sep)
            fout.write('// Generated by %s - Do not edit by hand!\n' % __file__)
            fout.write(sep)
            fout.write('\n')
            
            for i, puzzle in enumerate(data):
                # Puzzle Header
                fout.write(sep)
                fout.write('// Puzzle %d\n' % i)
                fout.write(sep)
                fout.write('\n')
                
                # Cutscene Buddies (Start)
                buddyIds = [BuddyToId(b) for b in puzzle['cutscene_start']['buddies']]
                fout.write('const BuddyId kCutsceneBuddiesStart%d[] = { %s };\n' % (i, ', '.join(buddyIds)))
                
                # Cutscene Lines (Start)
                fout.write('const CutsceneLine kCutsceneLinesStart%d[] =\n' % i)
                fout.write('{\n')
                for line in puzzle['cutscene_start']['lines']:
                    text = line['text'].replace('\n', '\\n')
                    vars = (line['speaker'], Id(line, 'view'), text)
                    fout.write('    CutsceneLine(%d, CutsceneLine::%s, "%s"),\n' % vars)
                fout.write('};\n')
                
                # Cutscene Buddies (End)
                buddyIds = [BuddyToId(b) for b in puzzle['cutscene_end']['buddies']]
                fout.write('const BuddyId kCutsceneBuddiesEnd%d[] = { %s };\n' % (i, ', '.join(buddyIds)))
                
                # Cutscene Lines (End)
                fout.write('const CutsceneLine kCutsceneLinesEnd%d[] =\n' % i)
                fout.write('{\n')
                for line in puzzle['cutscene_end']['lines']:
                    text = line['text'].replace('\n', '\\n')
                    vars = (line['speaker'], Id(line, 'view'), text)
                    fout.write('    CutsceneLine(%d, CutsceneLine::%s, "%s"),\n' % vars)
                fout.write('};\n')
                
                # Buddy IDs
                buddies = puzzle['buddies']
                buddyIds = [BuddyToId(b) for b in buddies]
                fout.write('const BuddyId kBuddies%d[] = { %s };\n' % (i, ', '.join(buddyIds)))
                
                # Pieces (Start)
                fout.write('const Piece kPiecesStart%d[][NUM_SIDES] =\n' % i)
                fout.write('{\n')
                for j, buddy in enumerate(buddies):
                    fout.write('    {\n')
                    for side in ['top', 'left', 'bottom', 'right']:
                        piece = puzzle['buddies'][buddy]['pieces_start'][side]
                        vars = (BuddyToId(piece['buddy']), Id(piece, 'part'))
                        fout.write('        Piece(%s, Piece::%s),\n' % vars)
                    fout.write('    },\n')
                fout.write('};\n')
                
                # Pieces (End)
                fout.write('const Piece kPiecesEnd%d[][NUM_SIDES] =\n' % i)
                fout.write('{\n')
                for j, buddy in enumerate(buddies):
                    fout.write('    {\n')
                    for side in ['top', 'left', 'bottom', 'right']:
                        piece = puzzle['buddies'][buddy]['pieces_end'][side]
                        vars = (BuddyToId(piece['buddy']), Id(piece, 'part'), BoolToString(piece['solve']))
                        fout.write('        Piece(%s, Piece::%s, %s),\n' % vars)
                    fout.write('    },\n')
                fout.write('};\n')
                fout.write('\n')
                
            # Puzzle Array Header
            fout.write(sep)
            fout.write('// Puzzles Array\n')
            fout.write(sep)
            fout.write('\n')
            
            # Puzzles Array
            fout.write('const Puzzle kPuzzles[] =\n')
            fout.write('{\n')
            for i, puzzle in enumerate(data):
                fout.write('    Puzzle(\n')
                fout.write('        %d,\n' % puzzle['book'])
                fout.write('        "%s",\n' % puzzle['title'])
                fout.write('        "%s",\n' % puzzle['clue'])
                fout.write('        kCutsceneBuddiesStart%d, arraysize(kCutsceneBuddiesStart%d),\n' % (i, i))
                fout.write('        kCutsceneLinesStart%d, arraysize(kCutsceneLinesStart%d),\n' % (i, i))
                fout.write('        kCutsceneBuddiesEnd%d, arraysize(kCutsceneBuddiesEnd%d),\n' % (i, i))
                fout.write('        kCutsceneLinesEnd%d, arraysize(kCutsceneLinesEnd%d),\n' % (i, i))
                fout.write('        %d,\n' % puzzle['cutscene_environment'])
                fout.write('        kBuddies%d, arraysize(kBuddies%d),\n' % (i, i))
                fout.write('        %d,\n' % puzzle['shuffles'])
                fout.write('        kPiecesStart%d, kPiecesEnd%d),\n' % (i, i))
            fout.write('};\n')

####################################################################################################
####################################################################################################

if __name__ == "__main__":
    if len(sys.argv) == 1 or len(sys.argv) > 3:
        print "Usage: python %s <json filename> [<dest filename>]" % __file__
        exit(1)
    if len(sys.argv) > 1:
        ValidatePuzzles(sys.argv[1])
    if len(sys.argv) > 2:
        ConvertPuzzles(sys.argv[1], sys.argv[2])
    
