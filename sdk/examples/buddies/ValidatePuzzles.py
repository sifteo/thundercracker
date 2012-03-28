#!/usr/bin/env python
"""
script to validate JSON puzzles in CubeBuddies format.  
"""

####################################################################################################
# TODO: show stack on error
# TODO: double-click error report
####################################################################################################

import sys
import json

####################################################################################################
# Data
####################################################################################################

validated = True
buddies = ['gluv', 'suli', 'rike', 'boff', 'zorg', 'maro']
views = ['right', 'left', 'front']
sides = ['top', 'left', 'bottom', 'right']
parts = ['hair', 'eye_left', 'mouth', 'eye_right']

####################################################################################################
# Validators
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

def CheckHasLengthOf(data, name, length):
    if len(data) != length:
        print 'Property Error: "%s" must have a length of %d (it is %d)' % (name, length, len(data))
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

def CheckIsBinary(data, prop):
    if data[prop] != 0 and data[prop] != 1:
        print 'Property Error: "%s" must be 0 or 1 (it is %d)' % (prop, data[prop])
        global validated
        validated = False
        return False
    else:
        return True

def CheckIsBoolean(data, prop):
    if data[prop] is not True and data[prop] is not False:
        print 'Property Error: "' + prop + '" must be either true or false (it is ' + data[prop] + ')'
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
                        CheckIsBinary(line, 'speaker')
                    if CheckHasProperty(line, 'view'):
                        CheckIsMember(views, line['view'])
                    if CheckHasProperty(line, 'text'):
                        CheckIsNotNull(line, 'text')

####################################################################################################
####################################################################################################

def CheckPieces(pieces, check_solve):
    if CheckHasLengthOf(pieces, 'pieces', 4):
        for side in pieces:
            if CheckIsMember(sides, side):
                if CheckHasProperty(pieces[side], 'buddy'):
                    CheckIsMember(buddies, pieces[side]['buddy'])
                if CheckHasProperty(pieces[side], 'part'):
                    CheckIsMember(parts, pieces[side]['part'])
                if check_solve:
                    if CheckHasProperty(pieces[side], 'solve'):
                        CheckIsBoolean(pieces[side], 'solve')

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
        
        # TODO: Version upgrades.
        
        # Bail if our version doesn't jive with the parser.
        if not j.has_key('version') or j['version'] != 1:
            print "Version Error: %s is a not supported version." % src
            exit(1)
        
        if not j.has_key('puzzles') or len(j['puzzles']) == 0:
            print "Data Error: Ain't got none."
            exit(1)
        
        ValidateData(j['puzzles'])
        if not validated:
            exit(1)

####################################################################################################
####################################################################################################

if __name__ == "__main__":
    if len(sys.argv[1:]) != 1:
        print "Usage: python %s <json filename>" % __file__
        exit(1)
    else:
        ValidatePuzzles(sys.argv[1])
    
