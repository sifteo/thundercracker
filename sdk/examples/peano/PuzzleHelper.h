#pragma once

#include "TokenGroup.h"
#include "Puzzle.h"
#include "Game.h"

namespace TotalsGame {

class PuzzleHelper {
public:
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
    // XML STUFF
    //-------------------------------------------------------------------------

    public static Puzzle CreateFromXML(XmlNode puzzleNode) {
        var tokenNodes = puzzleNode.SelectNodes("token");
        var result = new Puzzle(tokenNodes.Count);
        {
            var attr = puzzleNode.Attributes["guid"];
            if (attr != null) { result.guid = new Guid(attr.InnerText); }
        }
        for(int i=0; i<tokenNodes.Count; ++i) {
            var t = tokenNodes[i];
            result.tokens[i].val = int.Parse(t.Attributes["val"].InnerText);
            result.tokens[i].OpRight = (Op)Enum.Parse(typeof(Op), t.Attributes["opRight"].InnerText);
            result.tokens[i].OpBottom = (Op)Enum.Parse(typeof(Op), t.Attributes["opBottom"].InnerText);
        }
        var groupNodes = puzzleNode.SelectNodes("group");
        if (groupNodes.Count > 0) {
            var groups = new TokenGroup[groupNodes.Count];
            for(int i=0; i<groupNodes.Count; ++i) {
                XmlNode gn = groupNodes[i];
                var srcId = int.Parse(gn.Attributes["src"].InnerText);
                var dstId = int.Parse(gn.Attributes["dst"].InnerText);
                IExpression src;
                if (srcId < result.tokens.Length) { src = result.tokens[srcId]; }
                else { src = groups[srcId - result.tokens.Length]; }
                IExpression dst;
                if (dstId < result.tokens.Length) { dst = result.tokens[dstId]; }
                else { dst = groups[dstId - result.tokens.Length]; }
                groups[i] = new TokenGroup(
                            src, result.tokens[int.Parse(gn.Attributes["srcToken"].InnerText)],
                            (Cube.Side)Enum.Parse(typeof(Cube.Side), gn.Attributes["srcSide"].InnerText),
                            dst, result.tokens[int.Parse(gn.Attributes["dstToken"].InnerText)]
                            );
            }
            result.target = groups[groups.Length-1];
        }
        result.ComputeDifficulties();
        return result;
    }
    
    public static string ToXML(this Puzzle p) {
        var result = new StringBuilder();
        result.AppendFormat("<puzzle guid=\"{0}\">\n", p.guid.ToString());
        foreach(var t in p.tokens) {
            result.AppendFormat(
                        "\t<token val=\"{0}\" opRight=\"{1}\" opBottom=\"{2}\" />\n",
                        t.val,
                        t.opRight,
                        t.opBottom
                        );
        }
        var li = new List<TokenGroup>(ListGroupsChildToParent(p.target));
        foreach(var grp in li) {
            int srcId = grp.src is Token ? Array.IndexOf(p.tokens, grp.src as Token) : p.tokens.Length + li.IndexOf(grp.src as TokenGroup);
            int dstId = grp.dst is Token ? Array.IndexOf(p.tokens, grp.dst as Token) : p.tokens.Length + li.IndexOf(grp.dst as TokenGroup);
            result.AppendFormat(
                        "\t<group src=\"{0}\" srcToken=\"{1}\" srcSide=\"{2}\" dst=\"{3}\" dstToken=\"{4}\"/>\n",
                        srcId,
                        Array.IndexOf(p.tokens, grp.srcToken),
                        grp.srcSide,
                        dstId,
                        Array.IndexOf(p.tokens, grp.dstToken)
                        );
        }
        result.Append("</puzzle>\n");
        return result.ToString();
    }

    //-------------------------------------------------------------------------
    // GRACEFUL DIFFICULTY DOWNGRADE
    //-------------------------------------------------------------------------

    public static void ComputeDifficulties(this Puzzle p) {
        foreach(var token in p.tokens) {
            token.ComputeDifficulties();
        }
    }

    static void ComputeDifficulties(this Token t) {
        var r = t.opRight[(int)Difficulty.Hard];
        var b = t.opBottom[(int)Difficulty.Hard];
        if (r == Op.Multiply) {
            if (b == Op.Divide) {
                // MD
                t.SetRight(Difficulty.Easy, Op.Add);
                t.SetBottom(Difficulty.Easy, Op.Subtract);
                t.SetBottom(Difficulty.Medium, Op.Add);
            } else {
                // M*
                t.SetRight(Difficulty.Easy, b == Op.Add ? Op.Subtract : Op.Add);
            }
        } else if (r == Op.Divide) {
            if (b == Op.Multiply) {
                // DM
                t.SetRight(Difficulty.Easy, Op.Subtract);
                t.SetBottom(Difficulty.Easy, Op.Add);
                t.SetRight(Difficulty.Medium, Op.Subtract);
            } else {
                // D*
                t.SetRight(Difficulty.Easy, b == Op.Add ? Op.Subtract : Op.Add);
                t.SetRight(Difficulty.Medium, Op.Multiply);
            }
        } else {
            if (b == Op.Multiply) {
                // *M
                t.SetBottom(Difficulty.Easy, r == Op.Add ? Op.Subtract : Op.Add);
            } else if (b == Op.Divide) {
                // *D
                t.SetBottom(Difficulty.Easy, r == Op.Add ? Op.Subtract : Op.Add);
                t.SetBottom(Difficulty.Medium, Op.Multiply);
            }
        }

    }

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

    static TokenGroup *ConnectRandomly(IExpression *grp1, IExpression *grp2, bool secondTime = false)
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

    static Puzzle *SelectRandomTokens(Difficulty difficulty, int nTokens)
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

};

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

