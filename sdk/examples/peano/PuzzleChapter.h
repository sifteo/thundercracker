#pragma once

#include "Puzzle.h"

namespace TotalsGame {

    class PuzzleDatabase;
    
  class PuzzleChapter 
  {
  public:
      
    PuzzleDatabase *db;
    Guid guid;
    const char *id;
    const char *name;
    
	PuzzleChapter()
	{
		db = NULL;
		id = "";
		name = "";
		numPuzzles = 0;
        puzzles = (Puzzle*)puzzleBuffer;

	}
	/*
    public static PuzzleChapter CreateFromXML(XmlNode chapter) {
      var result = new PuzzleChapter() {
        id = chapter.Attributes["id"].InnerText,
        name = chapter.Attributes["name"].InnerText,
        guid = new Guid(chapter.Attributes["guid"].InnerText)
      };
      foreach(XmlNode puzzle in chapter.SelectNodes("puzzle")) {
        var puzzleInstance = PuzzleHelper.CreateFromXML(puzzle);
        puzzleInstance.chapter = result;
        result.Puzzles.Add(puzzleInstance);
      }
      return result;
    }
	*/

	int NumPuzzles() { return numPuzzles;}
	Puzzle *GetPuzzle(int index)
	{
		assert(index < numPuzzles);
		if(index < numPuzzles) 
			return puzzles+index;
		else
			return NULL;
	}
      
    int IndexOfPuzzle(Puzzle *p)
      {
          for(int i = 0; i < numPuzzles; i++)
          {
              if(puzzles+i == p)
                  return i;
          }
          return -1;
      }

      bool HasBeenSolved();
      bool CanBePlayedWithCurrentCubeSet();
      void SaveAsSolved();
      Puzzle *FirstPuzzleForCurrentCubeSet();

      
	private:
		enum {MAX_PUZZLES = 100};
      Puzzle *puzzles;
      char puzzleBuffer[sizeof(Puzzle) * MAX_PUZZLES];
		int numPuzzles;
  };

}

