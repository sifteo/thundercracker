#pragma once

#include "sifteo.h"
//#include "Guid.h"

namespace TotalsGame 
{

class Puzzle;

class SaveData
{
public:
    SaveData();

    void AddSolvedPuzzle(int chapter, int puzzle);
    void AddSolvedChapter(int chapter);
    void Save();

    bool IsPuzzleSolved(int chapter, int puzzle);
    bool IsChapterSolved(int chapter);

private:
    
    uint16_t solvedPuzzles[16];
    uint16_t solvedChapters;
    
#if 0
    static const int MAX_GUIDS = 100;//TODO is this enough?!

    Guid solvedGuids[MAX_GUIDS];
    int numSolvedGuids;
#endif    
    bool hasDoneTutorial;



    /*
    public readonly PuzzleDatabase Db;
    public readonly HashSet<Guid> solved = new HashSet<Guid>();
    public
    public SaveData(PuzzleDatabase database)
    {
      Db = database;
    }
*/
public:
    bool AllChaptersSolved()
    {/* TODO
      get {
        return !Db.Chapters.Exists( chapter =>
          chapter.CanBePlayedWithCurrentCubeSet() && !chapter.HasBeenSolved()
        );
      }*/
        return false;
    }

    bool IsChapterUnlockedWithCurrentCubeSet(int i)
    {
/*
      // the first chapter is always unlocked
      if (i == 0) { return true; }

      // if the chapter has been solved
      if (Db.Chapters[i].HasBeenSolved()) { return true; }

      // if the previous chapter (subject to the current cubeset) has been solved
      int j = i-1;
      while(j > 0 && !Db.Chapters[j].CanBePlayedWithCurrentCubeSet())
      {
        --j;
      }
      return Db.Chapters[j].HasBeenSolved();
      */return true;//TODO
    }

    void CompleteTutorial();

public:
    void Reset();

public:
    Puzzle *FindNextPuzzle() {return NULL;}
    /*
    public Puzzle FindNextPuzzle()
    {
      foreach(var chapter in Db.Chapters)
      {
        if (!chapter.HasBeenSolved())
        {
          var puzzle = chapter.Puzzles.Find(p => !p.HasBeenSolved() && p.tokens.Length <= Game.Inst.CubeSet.Count);
          if (puzzle != null)
          {
              return puzzle;
          }
        }
      }
      return null;
    }

    public void Load() {
      #if USE_TEST_DATA

      hasDoneTutorial = true;
      //foreach(var chapter in Db.Chapters) {
      //  solved.Add(chapter.guid);
      //}
      solved.Add(Db.Chapters[0].guid);

      #else
      if (Game.Inst.StoredData.IsValid)
      {
        var xml = Game.Inst.StoredData.Load();
        if (xml.Length > 0)
        {
          LoadXML(xml);
        }
      }
      #endif
    }

    public string ToXML()
    {
      var result = new StringBuilder();
      result.AppendFormat("<save hasDoneTutorial=\"{0}\">\n", hasDoneTutorial?"true":"false");
      result.AppendLine("<solved>");
      foreach(var guid in solved)
      {
        result.AppendLine(guid.ToString());
      }
      result.AppendLine("</solved>");
      result.AppendLine("</save>");
      return result.ToString();
    }

    public void LoadXML(string xml)
    {
      var doc = new XmlDocument();
      try {
        doc.LoadXml(xml);
        var save = doc.SelectSingleNode("save");
        hasDoneTutorial |= save.Attributes["hasDoneTutorial"].InnerText=="true";
        using (var textReader = new StringReader(save.SelectSingleNode("solved").InnerText)) {
          var line = string.Empty;
          while((line=textReader.ReadLine()) != null) {
            line = line.Trim();
            if (line.Length > 0) {
              solved.Add(new Guid(line));
            }
          }
        }
      } catch {
        Log.Warning("Bad Stored Data, Resetting");
        Reset();
      }
    }

    public void Save()
    {
      Game.Inst.StoredData.Store(ToXML());
    }
    */

};
}

