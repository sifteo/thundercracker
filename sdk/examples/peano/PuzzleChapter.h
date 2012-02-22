#pragma once

#include "Puzzle.h"
#include "sifteo.h"


namespace TotalsGame {

    class PuzzleDatabase;
    
  class PuzzleChapter 
  {
  public:
      
    PuzzleDatabase *db;
    Guid guid;
    //const char *id;
    const PinnedAssetImage *idImage;
    const char *name;
    
    PuzzleChapter();
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

    int NumPuzzles();
    Puzzle *GetPuzzle(int index);
      
    int IndexOfPuzzle(Puzzle *p);

      bool HasBeenSolved();
      bool CanBePlayedWithCurrentCubeSet();
      void SaveAsSolved();
      Puzzle *FirstPuzzleForCurrentCubeSet();

      
	private:
      Puzzle *puzzles;
		int numPuzzles;
  };

}

