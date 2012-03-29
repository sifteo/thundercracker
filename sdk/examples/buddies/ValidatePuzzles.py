#!/usr/bin/env python
"""
script to validate JSON puzzles in CubeBuddies format.  
"""

####################################################################################################
# TODO: double-click error report
# TODO: version upgrades
# TODO: find a better alternative to the global prop_stack
####################################################################################################

import sys
import json

####################################################################################################
# Data
####################################################################################################

validated = True
prop_stack = []

version_current = 2
buddies = ['gluv', 'suli', 'rike', 'boff', 'zorg', 'maro']
views = ['right', 'left', 'front']
sides = ['top', 'left', 'bottom', 'right']
parts = ['hair', 'eye_left', 'mouth', 'eye_right']

####################################################################################################
# Upgrades
####################################################################################################

def Upgrade1_2(j):
    print 'UPGRADE: Version 1 => 2'
    
    # Upgrade Version
    j['version'] = 2
    
    # Change buddies{} to buddies[] and have dicts with a name in there
    for p in j['puzzles']:
        buddies = []
        for b in p['buddies']:
            buddies.append({'name' : b, 'pieces_start' : p['buddies'][b]['pieces_start'], 'pieces_end' : p['buddies'][b]['pieces_end']})
        p['buddies'] = buddies
    
    # Create a books array
    j['books'] = []
    
    # Find number of books currently in use
    max_book = 0
    for p in j['puzzles']:
        if p['book'] > max_book:
            max_book = p['book']
    
    # Create placeholder books for them
    for i in range(max_book + 1):
        j['books'].append({'title' : "Book Desc %d" % i, 'puzzles' : []})
    
    # Insert the puzzles into the books they belong too (and kill the old book attribute)
    for p in j['puzzles']:
        book = p['book']
        del p['book']
        j['books'][book]['puzzles'].append(p)
    
    # Delete the original puzzles
    del j['puzzles']
    return j

upgrades = [None, Upgrade1_2]

####################################################################################################
# Utility
####################################################################################################

def StackString(stack):
    return ''.join(['[' + s + ']' for s in stack])

####################################################################################################
# Validators
####################################################################################################

def CheckHasProperty(data, prop):
    if not data.has_key(prop):
        print 'ERROR: %s is missing.' % StackString(prop_stack + [prop])
        global validated
        validated = False
        return False
    else:
        return True

def CheckIsUnsigned(data, prop):
    if data[prop] < 0:
        print 'ERROR: %s must be >= 0 (it is %d).' % (StackString(prop_stack), data[prop])
        global validated
        validated = False
        return False
    else:
        return True

def CheckIsNotNull(data, prop):
    if data[prop] == None:
        print 'ERROR: %s cannot be null (it is %s).' % (StackString(prop_stack), data[prop])
        global validated
        validated = False
        return False
    else:
        return True

def CheckHasLength(data, prop):
    if len(data[prop]) == 0:
        print 'ERROR: %s cannot have a length of 0.' % StackString(prop_stack)
        global validated
        validated = False
        return False
    else:
        return True

def CheckHasLengthOf(data, prop, length):
    if len(data[prop]) != length:
        print 'ERROR: %s must have a length of %d (it is %d).' % (StackString(prop_stack), length, len(data[prop]))
        global validated
        validated = False
        return False
    else:
        return True

def CheckIsMember(prop, values):
    if prop not in values:
        print 'ERROR: %s must be one of the following %s.' % (StackString(prop_stack), values)
        global validated
        validated = False
        return False
    else:
        return True

def CheckIsBinary(data, prop):
    if data[prop] != 0 and data[prop] != 1:
        print 'ERROR: %s must be 0 or 1 (it is %d).' % (StackString(prop_stack), data[prop])
        global validated
        validated = False
        return False
    else:
        return True

def CheckIsBoolean(data, prop):
    if data[prop] is not True and data[prop] is not False:
        print 'ERROR:' + StackString(prop_stack) + ' must be either true or false (it is ' + data[prop] + ')'
        validated = False
        return False
    else:
        return True

def CheckTextSize(data, prop, num_char, num_lines):
    s = data[prop]
    lines = s.count('\n') + 1
    max_char = 0
    for line in s.split('\n'):
        if len(line) > max_char:
            max_char = len(line)
    if lines > num_lines or max_char > num_char:
        print 'ERROR: %s must cannot have more than %d lines of %d characters (it has %d lines and max %d characters each).' % (StackString(prop_stack), num_lines, num_char, lines, max_char)
        global validated
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
                        CheckTextSize(line, 'text', 14, 3)
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

def CheckBuddy(buddy):
    if CheckHasProperty(buddy, 'name'):
        prop_stack.append('name')
        CheckIsNotNull(buddy, 'name')
        CheckIsMember(buddy['name'], buddies)
        prop_stack.pop()
    if CheckHasProperty(buddy, 'pieces_start'):
        CheckPieces(buddy, 'pieces_start', False)
    if CheckHasProperty(buddy, 'pieces_end'):
        CheckPieces(buddy, 'pieces_end', True)

####################################################################################################
####################################################################################################

def ValidateData(data):
    if CheckHasProperty(data, 'books'):
        global prop_stack
        prop_stack.append('books')
        for i_book, book in enumerate(data['books']):
            prop_stack.append('%d' % i_book)
            
            # title
            if CheckHasProperty(book, 'title'):
                prop_stack.append('title')
                CheckIsNotNull(book, 'title')
                CheckTextSize(book, 'title', 14, 1)
                prop_stack.pop()
            
            # puzzles
            if CheckHasProperty(book, 'puzzles'):
                prop_stack.append('puzzles')
                for i_puzzle, puzzle in enumerate(book['puzzles']):
                    prop_stack.append('%d' % i_puzzle)
                    
                    # title
                    if CheckHasProperty(puzzle, 'title'):
                        prop_stack.append('title')
                        CheckIsNotNull(puzzle, 'title')
                        CheckTextSize(puzzle, 'title', 14, 2)
                        prop_stack.pop()
                    
                    # clue
                    if CheckHasProperty(puzzle, 'clue'):
                        prop_stack.append('clue')
                        CheckIsNotNull(puzzle, 'clue')
                        CheckTextSize(puzzle, 'clue', 14, 5)
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
                        for i_buddy, buddy in enumerate(puzzle['buddies']):
                            prop_stack.append('%d' % i_buddy)
                            CheckBuddy(buddy)
                            prop_stack.pop()
                        prop_stack.pop()
                    prop_stack.pop()
                prop_stack.pop()
            prop_stack.pop()
        prop_stack.pop()
####################################################################################################
####################################################################################################

def ValidatePuzzles(src):  
    with open(src, 'r') as f:
        j = json.load(f)
        
        # Bail if our version doesn't jive with the parser.
        if not j.has_key('version') or j['version'] > version_current:
            print "Version Error: %s is a not supported version." % src
            return False
        
        if not j.has_key('puzzles') or len(j['puzzles']) == 0:
            print "Data Error: Ain't got none."
            return False
        
        version = j['version']
        if version < version_current:
            for i in range(version_current - version):
                j = upgrades[version + i](j)
            f.close()
            f = open(src, 'w')
            json.dump(j, f, sort_keys = True, indent = 2)
            f.close()
        
        ValidateData(j)
        
        if not validated:
            return False
        
    return True

####################################################################################################
####################################################################################################

if __name__ == "__main__":
    if len(sys.argv[1:]) != 1:
        print "Usage: python %s <json filename>" % __file__
        exit(1)
    else:
        if ValidatePuzzles(sys.argv[1]):
            exit(0)
        else:
            exit(1)
