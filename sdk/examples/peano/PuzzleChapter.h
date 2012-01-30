
namespace TotalsGame {

  class PuzzleChapter 
  {
    PuzzleDatabase *db = null;
    Guid guid;
    const char *id = "";
    const char *name = "";
    
	PuzzleChapter()
	{
		db = NULL;
		id = "";
		name = "";

	}

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


	size_t NumPuzzles() { return numPuzzles;}
	Puzzle *GetPuzzle(size_t index)
	{
		assert(index < numPuzzles);
		if(index < numPuzzles) 
			return puzzles+index;
		else
			return NULL;
	}

	private:
		enum {MAX_PUZZLES = 100};
		Puzzle puzzles[MAX_PUZZLES];
		size_t numPuzzles;
  };

}

