#include "PuzzleHelper.h"
#include "Token.h"

namespace TotalsGame
{
    //-------------------------------------------------------------------------
    // ADVANCEMENT
    //-------------------------------------------------------------------------
#if 0
    public static Puzzle GetNext(this Puzzle puzzle) {
        if (puzzle.chapter == null) { return null; }
        var localIndex = puzzle.chapter.Puzzles.IndexOf(puzzle);
        if (localIndex < puzzle.chapter.Puzzles.Count-1) {
            return puzzle.chapter.Puzzles[localIndex+1];
        }
        if (puzzle.chapter.db == null) { return null; }
        var globalIndex = puzzle.chapter.db.Chapters.IndexOf(puzzle.chapter);
        if (globalIndex < puzzle.chapter.db.Chapters.Count-1) {
            return puzzle.chapter.db.Chapters[globalIndex+1].Puzzles[0];
        }
        return null;
    }

    public static Puzzle GetNext(this Puzzle puzzle, int maxCubeCount) {
        do {
            puzzle = puzzle.GetNext();
        } while(puzzle != null && puzzle.tokens.Length > maxCubeCount);
        return puzzle;
    }

    //-------------------------------------------------------------------------
    // GRACEFUL DIFFICULTY DOWNGRADE
    //-------------------------------------------------------------------------
#endif
    void PuzzleHelper::ComputeDifficulties(Puzzle *p) {
        for(int i = 0; i < p->GetNumTokens(); i++)
            ComputeDifficulties(p->GetToken(i));
    }

    void PuzzleHelper::ComputeDifficulties(Token *t) {
        Op r = t->opRight[(int)DifficultyHard];
        Op b = t->opBottom[(int)DifficultyHard];
        if (r == OpMultiply) {
            if (b == OpDivide) {
                // MD
                t->SetOpRight(DifficultyEasy, OpAdd);
                t->SetOpBottom(DifficultyEasy, OpSubtract);
                t->SetOpBottom(DifficultyMedium, OpAdd);
            } else {
                // M*
                t->SetOpRight(DifficultyEasy, b == OpAdd ? OpSubtract : OpAdd);
            }
        } else if (r == OpDivide) {
            if (b == OpMultiply) {
                // DM
                t->SetOpRight(DifficultyEasy, OpSubtract);
                t->SetOpBottom(DifficultyEasy, OpAdd);
                t->SetOpRight(DifficultyMedium, OpSubtract);
            } else {
                // D*
                t->SetOpRight(DifficultyEasy, b == OpAdd ? OpSubtract : OpAdd);
                t->SetOpRight(DifficultyMedium, OpMultiply);
            }
        } else {
            if (b == OpMultiply) {
                // *M
                t->SetOpBottom(DifficultyEasy, r == OpAdd ? OpSubtract : OpAdd);
            } else if (b == OpDivide) {
                // *D
                t->SetOpBottom(DifficultyEasy, r == OpAdd ? OpSubtract : OpAdd);
                t->SetOpBottom(DifficultyMedium, OpMultiply);
            }
        }

    }
#if 0
    static void SetRight(this Token t, Difficulty d, Op op) { t.opRight[(int)d] = op; }
    static void SetBottom(this Token t, Difficulty d, Op op) { t.opBottom[(int)d] = op; }

    //-------------------------------------------------------------------------
    // MISC UTILITIES
    //-------------------------------------------------------------------------

    static IEnumerable<TokenGroup> ListGroupsChildToParent(TokenGroup g) {
        if (g == null) { yield break; }
        foreach(var child in ListGroupsChildToParent(g.src as TokenGroup)) { yield return child; }
        foreach(var child in ListGroupsChildToParent(g.dst as TokenGroup)) { yield return child; }
        yield return g;
    }
#endif		

    TokenGroup *PuzzleHelper::ConnectRandomly(IExpression *grp1, IExpression *grp2, bool secondTime)
    {
        if (grp1 == NULL || grp2 == NULL)
        {
            return NULL;
        }

        Connection outConnects[10];
        int numOutConnects;
        Connection inConnects[10];
        int numInConnects;
        grp1->GetMask().ListOutConnections(outConnects, &numOutConnects, 10);
        grp2->GetMask().ListInConnections(inConnects, &numInConnects, 10);
        //shuffle them up
        for(int i = 0; i < numOutConnects; i++)
        {
            int a = Game::rand.randrange(numOutConnects);
            Connection temp = outConnects[i];
            outConnects[i] = outConnects[a];
            outConnects[a] = temp;
        }

        for(int i = 0; i < numInConnects; i++)
        {
            int a = Game::rand.randrange(numInConnects);
            Connection temp = inConnects[i];
            inConnects[i] = inConnects[a];
            inConnects[a] = temp;
        }


        for(int oci = 0; oci < numOutConnects; oci++)
        {
            for(int ici = 0; ici < numInConnects; ici++)
            {
                Connection &oc = outConnects[oci];
                Connection &ic = inConnects[ici];

                if (oc.Matches(ic))
                {
                    Token *ot, *it;
                    grp1->TokenAt(oc.pos, &ot);
                    grp2->TokenAt(ic.pos, &it);
                    TokenGroup *result = TokenGroup::Connect(grp1, ot, oc.dir, grp2, it);
                    if (result != NULL)
                    {
                        return result;
                    }
                }
            }
        }
        if (!secondTime)
        {
            return ConnectRandomly(grp2, grp1, true);
        }

        return NULL;
    }

    Puzzle *PuzzleHelper::SelectRandomTokens(Difficulty difficulty, int nTokens)
    {
        Puzzle *result = new Puzzle(nTokens);

        for(int i = 0; i < result->GetNumTokens(); i++)
        {
            Token *token = result->GetToken(i);
            if (difficulty == DifficultyEasy)
            {
                int r = Game::rand.randrange(6) + 1;
                token->val = r;

                if (Game::rand.randrange(2) == 0)
                {
                    token->SetOpRight(OpAdd);
                    token->SetOpBottom(OpSubtract);
                }
                else
                {
                    token->SetOpRight(OpSubtract);
                    token->SetOpBottom(OpAdd);
                }
            }
            else
            {
                int maxOp = difficulty == DifficultyMedium ? 3 : 4;
                int firstChoice = Game::rand.randrange(maxOp);
                int secondChoice = Game::rand.randrange(maxOp-1);
                if (secondChoice == firstChoice)
                {
                    secondChoice = (secondChoice + 1) % maxOp;
                }
                token->SetOpRight((Op) firstChoice);
                token->SetOpBottom((Op) secondChoice);
                token->val = Game::rand.randrange(9) + 1;
            }
        }
        if (difficulty == DifficultyHard)
        {
            if (result->GetDifficulty() != DifficultyHard)
            {
                // need to throw one or two divisions in there
                int numToAdd = 1 + Game::rand.randrange(2);
                while(numToAdd > 0)
                {
                    int i=Game::rand.randrange(nTokens);
                    Token *token = NULL;
                    do
                    {
                        token = result->GetToken(i);
                        i = (i+1)%nTokens;
                    } while(token->GetOpRight() == OpDivide || token->GetOpBottom() == OpDivide);
                    if (Game::rand.randrange(2) == 0)
                    {
                        token->SetOpRight(OpDivide);
                    }
                    else
                    {
                        token->SetOpBottom(OpDivide);
                    }
                    numToAdd--;
                }
            }
        }
        else if (difficulty == DifficultyMedium)
        {
            if (result->GetDifficulty() != DifficultyMedium)
            {
                // need to throw one or two multiplies in there
                int numToAdd = 1 + Game::rand.randrange(2);
                while(numToAdd > 0)
                {
                    int i=Game::rand.randrange(nTokens);
                    Token *token = NULL;
                    do
                    {
                        token = result->GetToken(i);
                        i = (i+1)%nTokens;
                    } while(token->GetOpRight() == OpMultiply || token->GetOpBottom() == OpMultiply);
                    if (Game::rand.randrange(2) == 0) {
                        token->SetOpRight(OpMultiply);
                    } else {
                        token->SetOpBottom(OpMultiply);
                    }
                    numToAdd--;
                }
            }
        }
        return result;
    }



#if 0
public static class ListHelper {
    
    public static void Shuffle<T>(this IList<T> list, Random rng) {
        int n = list.Count;
        while (n > 1) {
            n--;
            int k = rng.Next(n + 1);
            T value = list[k];
            list[k] = list[n];
            list[n] = value;
        }
    }
    
    public static T Pick<T>(this IList<T> list, Random seed) {
        var i = seed.Next() % list.Count;
        var result = list[i];
        list.RemoveAt(i);
        return result;
    }
    


}
#endif
}

