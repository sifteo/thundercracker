#!/usr/bin/env python
"""
script to convert json puzzles in chroma format to a .h file that will be built into the game.  
"""

import sys
import json

def main():  
    print sys.argv[1:]
    
    try:
        src = sys.argv[1]
        dest = sys.argv[2]
        print src
    except:
        print("Usage: python %s <json filename> <dest filename>" % __file__)
        print args
        exit(1)
        
    with open(src, 'r') as f:
        data = json.load(f)["puzzles"]
                              
        with open(dest, 'w') as fout:
            fout.write( '//generated file containing puzzle data from ' + src + '\n\n' )
            fout.write( '#include "puzzle.h"\n\n' )
            fout.write( "static const Puzzle s_puzzles[] =\n" )
            fout.write( '{\n' )

            cubeDataIndex = 0
            cubeDataStrs = []
	    chapterIndices = []
            
            #dump all puzzles
            for i,p in enumerate(data):
                fout.write( '\tPuzzle( \"' + p["title"] + '\", \"' + p["instructions"] + '\", ' );
                
		if "pre_title" in p:
		    chapterIndices.append(i)
		
                numcubes = 0
                #each puzzle has n cubes worth of data
                for j,cubedata in enumerate(p["data"]):
                    numcubes += 1
		    datastr = ""
                    datastr += '\tPuzzleCubeData(\n'
                    datastr += '\t\t(uint8_t [] ){\n'
                    datastr += '\t\t\t' 
                    for k,d in enumerate(cubedata):
                        datastr += str( d ) + ", "
                        
                    datastr += '\n\t\t\t}\n\t\t),\n'
		    cubeDataStrs.append( datastr )
                    
                fout.write( str( cubeDataIndex ) + ", " );
                fout.write( str( numcubes ) + ' ),\n' );
		cubeDataIndex += numcubes
            
            fout.write( '};\n\n' )
	    
	    #dump all the cubeDatastrings
	    fout.write( "static const PuzzleCubeData s_puzzledata[] =\n" )
	    fout.write( '{\n' )
	    
	    for i in cubeDataStrs:
		fout.write( i + '\n' )
	    
	    fout.write( '};\n' )
	    
	    fout.write( '\nstatic const uint8_t s_puzzleChapterIndices[] =\n{\n' )
	    
	    for i in chapterIndices:
		fout.write( str(i) + "," )
		
	    fout.write( "\n};\n" )
        

if __name__ == "__main__":
    main()
