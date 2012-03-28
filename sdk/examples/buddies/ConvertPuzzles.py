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

buddies = ['gluv', 'suli', 'rike', 'boff', 'zorg', 'maro']
views = ['right', 'left', 'front']
sides = ['top', 'left', 'bottom', 'right']
parts = ['hair', 'eye_left', 'mouth', 'eye_right']

####################################################################################################
####################################################################################################

def HasProperty(data, prop):
    if not data.has_key(prop):
        print 'Property Missing: "%s"' % prop
        return False
    else:
        return True

def IsUnsigned(data, prop):
    if data[prop] < 0:
        print 'Property Error: "%s" must be >= 0 (it is %d)' % (prop, data[prop])
        return False
    else:
        return True

def IsNotNull(data, prop):
    if data[prop] == None:
        print 'Propery Error: "%s" cannot be null (it is %s)' % (prop, data[prop])
        return False
    else:
        return True

def HasLength(data, prop):
    if len(data[prop]) == 0:
        print 'Property Error: "%s" cannot have a length of 0' % prop
        return False
    else:
        return True

def IsMember(l, v):
    if v not in l:
        print 'Propery Error: "%s" must be one of the following %s' % (v, l)
    else:
        return True

def ValidateCutscene(puzzle, cutscene):
    if not HasProperty(puzzle, cutscene):
        return False
    if not HasProperty(puzzle[cutscene], 'buddies'):
        return False
    if not HasLength(puzzle[cutscene], 'buddies'):
        return False
    for buddy in puzzle[cutscene]['buddies']:
        if not IsMember(buddies, buddy):
            return False
    if not HasProperty(puzzle[cutscene], 'lines'):
        return False
    if not HasLength(puzzle[cutscene], 'lines'):
        return False
    for line in puzzle[cutscene]['lines']:
        if not HasProperty(line, 'speaker'):
            return False
        if line['speaker'] != 0 and line['speaker'] != 1:
            print 'Property Error: "speaker" must be 0 or 1 (it is %d)' % line['speaker']
            return False
        if not HasProperty(line, 'view'):
            return False
        if not IsMember(views, line['view']):
            return False
        if not HasProperty(line, 'text'):
            return False
        if not IsNotNull(line, 'text'):
            return False
    return True

####################################################################################################
####################################################################################################

def ValidatePieces(pieces, check_solve):
    if len(pieces) != 4:
        print 'Property Error: pieces must have 4 keys: %s' % sides
        return False
    for side in pieces:
        if not IsMember(sides, side):
            return False
        if not HasProperty(pieces[side], 'buddy'):
            return False
        if not IsMember(buddies, pieces[side]['buddy']):
            return False
        if not HasProperty(pieces[side], 'part'):
            return False
        if not IsMember(parts, pieces[side]['part']):
            return False
        if check_solve:
            if not HasProperty(pieces[side], 'solve'):
                return False
            if pieces[side]['solve'] is not True and pieces[side]['solve'] is not False:
                print 'Property Error: "solve" must be either true or false (it is ' + pieces[side]['solve'] + ')'
                return False
    return True

####################################################################################################
####################################################################################################

def ValidateBuddy(puzzle, buddy):
    if not HasProperty(puzzle, buddy):
        return False
    if not HasProperty(puzzle[buddy], 'pieces_start'):
        return False
    if not ValidatePieces(puzzle[buddy]['pieces_start'], False):
        return False
    if not HasProperty(puzzle[buddy], 'pieces_end'):
        return False
    if not ValidatePieces(puzzle[buddy]['pieces_end'], True):
        return False
    return True

####################################################################################################
####################################################################################################

def ValidateData(data):
    for puzzle in data:
        # book
        if not HasProperty(puzzle, 'book'):
            return False
        if not IsUnsigned(puzzle, 'book'):
            return False
        
        # title
        if not HasProperty(puzzle, 'title'):
            return False
        if not IsNotNull(puzzle, 'title'):
            return False
        
        # clue
        if not HasProperty(puzzle, 'clue'):
            return False
        if not IsNotNull(puzzle, 'clue'):
            return False
        
        # cutscene_environment
        if not HasProperty(puzzle, 'cutscene_environment'):
            return False
        if not IsUnsigned(puzzle, 'cutscene_environment'):
            return False
        
        # cutscene_start
        if not ValidateCutscene(puzzle, 'cutscene_start'):
            return False
        
        # cutscene_end
        if not ValidateCutscene(puzzle, 'cutscene_end'):
            return False
        
        # shuffles
        if not HasProperty(puzzle, 'shuffles'):
            return False
        if not IsUnsigned(puzzle, 'shuffles'):
            return False
        
        # buddies
        if not HasProperty(puzzle, 'buddies'):
            return False
        for buddy in puzzle['buddies']:
            if not ValidateBuddy(puzzle['buddies'], buddy):
                return False
        
    return True

####################################################################################################
####################################################################################################

def ConvertPuzzles(src, dest):  
    with open(src, 'r') as f:
        j = json.load(f)
        
        # Bail if our version doesn't jive with the parser.
        # TODO: Version upgrades.
        version = j['version']
        if version != 1:
            print "%s is a not supported version." % src
            exit(1)
        
        data = j['puzzles']
        
        if not ValidateData(data):
            exit(1)
        
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
    try:
        src = sys.argv[1]
        dest = sys.argv[2]
    except:
        print "Usage: python %s <json filename> <dest filename>" % __file__
        exit(1)
    
    ConvertPuzzles(sys.argv[1], sys.argv[2])
    
