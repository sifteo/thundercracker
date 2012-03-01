#include "Game.h"
#include "PuzzleDatabase.h"
#include "puzzles.gen.h"
#include "Token.h"
#include "TokenGroup.h"
#include "PuzzleHelper.h"


namespace TotalsGame
{
namespace Database
{
int NumChapters()
{
    return TheData::everything.nChapters;
}

int NumPuzzlesInChapter(int chapter)
{
    return TheData::everything.chapters[chapter]->nPuzzles;
}

int NumTokensInPuzzle(int chapter, int puzzle)
{
    return TheData::everything.chapters[chapter]->puzzles[puzzle]->nTokens;
}
#if 0
Guid GuidForPuzzle(int chapter, int puzzle)
{
    if(chapter == -1 || puzzle == -1)
        return Guid::Empty;
    return Guid(TheData::everything.chapters[chapter]->puzzles[puzzle]->guid);
}
#endif
    
const PinnedAssetImage &ImageForChapter(int chapter)
{
    return TheData::everything.chapters[chapter]->icon;
}

const char *NameOfChapter(int chapter)
{
    return TheData::everything.chapters[chapter]->name;
}

Puzzle *GetPuzzleInChapter(int chapter, int puzzle)
{
    int numTokens = NumTokensInPuzzle(chapter, puzzle);
    Puzzle *result = new Puzzle(numTokens, chapter, puzzle);
    for(int i=0; i<numTokens; ++i) {
        int token = TheData::everything.chapters[chapter]->puzzles[puzzle]->tokens[i];
        int val = token & 0xf;
        Op opRight = (Op)((token >> 4) & 0x3);
        Op opLeft = (Op)((token >> 6) & 0x3);
        result->GetToken(i)->val = val;
        result->GetToken(i)->SetOpRight(opRight);
        result->GetToken(i)->SetOpBottom(opLeft);
    }

    int numGroups = TheData::everything.chapters[chapter]->puzzles[puzzle]->nGroups;
    if (numGroups > 0) {
        TokenGroup *groups[Game::NUMBER_OF_CUBES] = {0};
        for(int i=0; i<numGroups; ++i) {
            int group = TheData::everything.chapters[chapter]->puzzles[puzzle]->groups[i];

            int x_srcId = (group & 0x7f)/10;
            int x_src = (group & 0x7f)%10;
            int x_dstId = ((group>>9) & 0x7f)/10;
            int x_dst = ((group>>9) & 0x7f)%10;
            int x_side = (group>>7) & 0x3;

            IExpression *src;
            if (x_srcId < numTokens) { src = result->GetToken(x_srcId); }
            else { src = groups[x_srcId - numTokens]; }
            IExpression *dst;
            if (x_dstId < numTokens) { dst = result->GetToken(x_dstId); }
            else { dst = groups[x_dstId - numTokens]; }
            groups[i] = new TokenGroup(
                        src, result->GetToken(x_src),
                        (Cube::Side)x_side,
                        dst, result->GetToken(x_dst)
                        );
        }
        result->target = groups[numGroups-1];
    }
    PuzzleHelper::ComputeDifficulties(result);
    return result;

}

bool HasPuzzleBeenSolved(int chapter, int puzzle)
{
    return Game::GetInstance().saveData.IsPuzzleSolved(chapter, puzzle);//TheData::everything.chapters[chapter]->puzzles[puzzle]->guid);
}

bool CanBePlayedWithCurrentCubeSet(int chapter)
{
    //todo
    return -1 != FirstPuzzleForCurrentCubeSetInChapter(chapter);
}

void SaveChapterAsSolved(int chapter)
{
    Game::GetInstance().saveData.AddSolvedChapter(chapter);//TheData::everything.chapters[chapter]->guid);
      Game::GetInstance().saveData.Save();
}

void SavePuzzleAsSolved(int chapter, int puzzle)
{
    Game::GetInstance().saveData.AddSolvedPuzzle(chapter, puzzle);//TheData::everything.chapters[chapter]->puzzles[puzzle]->guid);
      Game::GetInstance().saveData.Save();
}

int FirstPuzzleForCurrentCubeSetInChapter(int chapter)
{
    for(int i = 0; i < TheData::everything.chapters[chapter]->nPuzzles; i++)
    {
        if(TheData::everything.chapters[chapter]->puzzles[i]->nTokens <= Game::NUMBER_OF_CUBES)
            return i;
    }
    return -1;
}
}
}



