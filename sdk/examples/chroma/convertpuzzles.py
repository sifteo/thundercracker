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
            
            #dump all puzzles
            for i,p in enumerate(data):
                fout.write( '\tPuzzle( \"' + p["title"] + '\", \"' + p["instructions"] + '\",\n' );
                fout.write( '\t(PuzzleCubeData []){\n' );
                
                numcubes = 0
                #each puzzle has n cubes worth of data
                for j,cubedata in enumerate(p["data"]):
                    numcubes += 1
                    fout.write( '\t\tPuzzleCubeData(\n' );
                    fout.write( '\t\t\t(unsigned int [] ){\n' );
                    fout.write( '\t\t\t' );
                    for k,d in enumerate(cubedata):
                        fout.write( str( d ) + ", " );
                        
                    fout.write( '\n\t\t\t}\n\t\t),\n' );
                    
                fout.write( '\t\t},\n' );
                fout.write( '\t\t' + str( numcubes ) + '\n\t),\n' );
            
            fout.write( '};\n' )
        

if __name__ == "__main__":
    main()