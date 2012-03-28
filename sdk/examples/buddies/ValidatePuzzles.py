#!/usr/bin/env python
"""
script to validate JSON puzzles in CubeBuddies format.  
"""

####################################################################################################
# TODO: double-click error report
# TODO: version upgrades
####################################################################################################

import sys
import json

####################################################################################################
# Data
####################################################################################################

validated = True
prop_stack = []

buddies = ['gluv', 'suli', 'rike', 'boff', 'zorg', 'maro']
views = ['right', 'left', 'front']
sides = ['top', 'left', 'bottom', 'right']
parts = ['hair', 'eye_left', 'mouth', 'eye_right']

####################################################################################################
# Utility
####################################################################################################

def StackString(stack):
    return stack[0] + ''.join(['[' + s + ']' for s in stack[1:]])

####################################################################################################
# Validators
####################################################################################################

def CheckHasProperty(data, prop):
    if not data.has_key(prop):
        print '%s is missing.' % StackString(prop_stack + [prop])
        global validated
        validated = False
        return False
    else:
        return True

def CheckIsUnsigned(data, prop):
    if data[prop] < 0:
        print '%s must be >= 0 (it is %d).' % (StackString(prop_stack), data[prop])
        global validated
        validated = False
        return False
    else:
        return True

def CheckIsNotNull(data, prop):
    if data[prop] == None:
        print '%s cannot be null (it is %s).' % (StackString(prop_stack), data[prop])
        global validated
        validated = False
        return False
    else:
        return True

def CheckHasLength(data, prop):
    if len(data[prop]) == 0:
        print '%s cannot have a length of 0.' % StackString(prop_stack)
        global validated
        validated = False
        return False
    else:
        return True

def CheckHasLengthOf(data, prop, length):
    if len(data[prop]) != length:
        print '%s must have a length of %d (it is %d).' % (StackString(prop_stack), length, len(data[prop]))
        global validated
        validated = False
        return False
    else:
        return True

def CheckIsMember(prop, values):
    if prop not in values:
        print '%s must be one of the following %s.' % (StackString(prop_stack), values)
        global validated
        validated = False
        return False
    else:
        return True

def CheckIsBinary(data, prop):
    if data[prop] != 0 and data[prop] != 1:
        print '%s must be 0 or 1 (it is %d).' % (StackString(prop_stack), data[prop])
        global validated
        validated = False
        return False
    else:
        return True

def CheckIsBoolean(data, prop):
    if data[prop] is not True and data[prop] is not False:
        print StackString(prop_stack) + ' must be either true or false (it is ' + data[prop] + ')'
        validated = False
        return False
    else:
        return True

####################################################################################################
####################################################################################################

def CheckCutscene(puzzle, cutscene):
    if CheckHasProperty(puzzle, cutscene):
        prop_stack.append(cutscene)
        if CheckHasProperty(puzzle[cutscene], 'buddies'):
            prop_stack.append('buddies')
            if CheckHasLength(puzzle[cutscene], 'buddies'):
                for i, buddy in enumerate(puzzle[cutscene]['buddies']):
                    prop_stack.append('%d' % i)
                    CheckIsMember(buddy, buddies)
                    prop_stack.pop()
            prop_stack.pop()
        if CheckHasProperty(puzzle[cutscene], 'lines'):
            prop_stack.append('lines')
            if CheckHasLength(puzzle[cutscene], 'lines'):
                for i, line in enumerate(puzzle[cutscene]['lines']):
                    prop_stack.append('%d' % i)
                    if CheckHasProperty(line, 'speaker'):
                        prop_stack.append('speaker')
                        CheckIsBinary(line, 'speaker')
                        prop_stack.pop()
                    if CheckHasProperty(line, 'view'):
                        prop_stack.append('view')
                        CheckIsMember(line['view'], views)
                        prop_stack.pop()
                    if CheckHasProperty(line, 'text'):
                        prop_stack.append('text')
                        CheckIsNotNull(line, 'text')
                        prop_stack.pop()
                    prop_stack.pop()
            prop_stack.pop()
        prop_stack.pop()

####################################################################################################
####################################################################################################

def CheckPieces(data, prop, check_solve):
    prop_stack.append(prop)
    if CheckHasLengthOf(data, prop, 4):
        for side in sides:
            if CheckHasProperty(data[prop], side):
                prop_stack.append(side)
                if CheckHasProperty(data[prop][side], 'buddy'):
                    prop_stack.append('buddy')
                    CheckIsMember(data[prop][side]['buddy'], buddies)
                    prop_stack.pop()
                if CheckHasProperty(data[prop][side], 'part'):
                    prop_stack.append('part')
                    CheckIsMember(data[prop][side]['part'], parts)
                    prop_stack.pop()
                if check_solve:
                    if CheckHasProperty(data[prop][side], 'solve'):
                        prop_stack.append('solve')
                        CheckIsBoolean(data[prop][side], 'solve')
                        prop_stack.pop()
                prop_stack.pop()
    prop_stack.pop()

####################################################################################################
####################################################################################################

def CheckBuddy(puzzle, buddy):
    if CheckHasProperty(puzzle, buddy):
        prop_stack.append(buddy)
        if CheckHasProperty(puzzle[buddy], 'pieces_start'):
            CheckPieces(puzzle[buddy], 'pieces_start', False)
        if CheckHasProperty(puzzle[buddy], 'pieces_end'):
            CheckPieces(puzzle[buddy], 'pieces_end', True)
        prop_stack.pop()

####################################################################################################
####################################################################################################

def ValidateData(data):
    for i, puzzle in enumerate(data):
        global prop_stack
        prop_stack.append('%d' % i)
    
        # book
        if CheckHasProperty(puzzle, 'book'):
            prop_stack.append('book')
            CheckIsUnsigned(puzzle, 'book')
            prop_stack.pop()
        
        # title
        if CheckHasProperty(puzzle, 'title'):
            prop_stack.append('title')
            CheckIsNotNull(puzzle, 'title')
            prop_stack.pop()
        
        # clue
        if CheckHasProperty(puzzle, 'clue'):
            prop_stack.append('clue')
            CheckIsNotNull(puzzle, 'clue')
            prop_stack.pop()
        
        # cutscene_environment
        if CheckHasProperty(puzzle, 'cutscene_environment'):
            prop_stack.append('cutscene_environment')
            CheckIsUnsigned(puzzle, 'cutscene_environment')
            prop_stack.pop()
        
        # cutscene_start
        CheckCutscene(puzzle, 'cutscene_start')
        CheckCutscene(puzzle, 'cutscene_end')
        
        # shuffles
        if CheckHasProperty(puzzle, 'shuffles'):
            prop_stack.append('shuffles')
            CheckIsUnsigned(puzzle, 'shuffles')
            prop_stack.pop()
        
        # buddies
        if CheckHasProperty(puzzle, 'buddies'):
            prop_stack.append('buddies')
            for buddy in puzzle['buddies']:
                CheckBuddy(puzzle['buddies'], buddy)
            prop_stack.pop()
        
        prop_stack.pop()

####################################################################################################
####################################################################################################

def ValidatePuzzles(src):  
    with open(src, 'r') as f:
        j = json.load(f)
        
        # Bail if our version doesn't jive with the parser.
        if not j.has_key('version') or j['version'] != 1:
            print "Version Error: %s is a not supported version." % src
            exit(1)
        
        if not j.has_key('puzzles') or len(j['puzzles']) == 0:
            print "Data Error: Ain't got none."
            exit(1)
        
        global prop_stack
        prop_stack.append('puzzles')
        
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
    
