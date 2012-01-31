#pragma once

#include "IExpression.h"

namespace TotalsGame {
  
  public class TokenGroup : IExpression 
  {
  public:

	  IExpression *GetSrc() {return src;}
	  IExpression *GetDst() {return dst;}
	  Vec2 GetSrcPos() {return srcPos;}
	  Vec2 GetDstPos() {return dstPos;}
	  Token *GetSrcToken() {return srcToken;}
	  Cube::Side GetSrcSide() {return srcSide;}
	  Token *GetDstToken() {return dstToken;}
    
	void *userData = NULL;

   Fraction GetValue() { return mValue; }
   ShapeMask GetMask() { return mMask; }
   int GetDepth() { return mDepth;}
   int GetCount() { return src.Count + dst.Count;}

    bool TokenAt(Vec2 p, Token **t);
  
    bool PositionOf(Token *t, Vec2 *p);
  
    bool Contains(Token *t);
    
    IExpression *GetSubExpressionContaining(Token *t);
	bool ContainsSubExpressions() {return true;}
  /*
    public IEnumerable<Token> Tokens {
      get {
        var token = src as Token;
        if (token == null) {
          foreach(var t in (src as TokenGroup).Tokens) { yield return t; }
        } else { yield return token; }
        token = dst as Token;
        if (token == null) {
          foreach(var t in (dst as TokenGroup).Tokens) { yield return t; }
        } else { yield return token; }
      }
    }
    */
    void SetCurrent(IExpression *exp);

    static TokenGroup *Connect(Token st, Int2 d, Token dt) { return Connect(st, st, d, dt, dt); }
    static TokenGroup *Connect(IExpression src, Token st, Int2 d, Token dt) { return Connect(src, st, d, dt, dt); }

    public static TokenGroup Connect(IExpression src, Token srcToken, Int2 d, IExpression dst, Token dstToken) {
      if (d.x < 0 || d.y < 0) { return Connect(dst, dstToken, -d, src, srcToken); }
      ShapeMask mask;
      Int2 d1, d2;
      Int2 sp, dp;
      src.PositionOf(srcToken, out sp);
      dst.PositionOf(dstToken, out dp);
      if (!ShapeMask.TryConcat(src.Mask, dst.Mask, sp + d - dp, out mask, out d1, out d2)) {
        return null;
      }
      Cube.Side srcSide = d.x == 1 ? Cube.Side.RIGHT : Cube.Side.BOTTOM;
      return new TokenGroup(
        src, d1, srcToken, srcSide,
        dst, d2, dstToken,
        OpHelper.Compute(src.Value, srcSide == Cube.Side.RIGHT ? srcToken.OpRight : srcToken.OpBottom, dst.Value),
        mask
      );
    }

    public void RecomputeValue() {
      if (src is TokenGroup) { (src as TokenGroup).RecomputeValue(); }
      if (dst is TokenGroup) { (dst as TokenGroup).RecomputeValue(); }
      mValue = OpHelper.Compute(src.Value, srcSide == Cube.Side.RIGHT ? srcToken.OpRight : srcToken.OpBottom, dst.Value);
    }

    internal TokenGroup(
      IExpression src, Token srcToken, Cube.Side srcSide,
      IExpression dst, Token dstToken
    ) {
      this.src = src;
      this.srcToken = srcToken;
      this.srcSide = srcSide;
      this.dst = dst;
      this.dstToken = dstToken;
      Int2 sp, dp;
      src.PositionOf(srcToken, out sp);
      dst.PositionOf(dstToken, out dp);
      Int2 d = Int2.Side(srcSide);
      ShapeMask.TryConcat(src.Mask, dst.Mask, sp + d - dp, out mMask, out srcPos, out dstPos);
      mValue = OpHelper.Compute(src.Value, srcSide == Cube.Side.RIGHT ? srcToken.OpRight : srcToken.OpBottom, dst.Value);
      mDepth = Math.Max(src.Depth, dst.Depth)+1;
    }
    
    internal TokenGroup(
      IExpression src, Int2 srcPos, Token srcToken, Cube.Side srcSide,
      IExpression dst, Int2 dstPos, Token dstToken,
      Fraction val,
      ShapeMask mask
    ) {
      this.src = src;
      this.srcPos = srcPos;
      this.srcToken = srcToken;
      this.srcSide = srcSide;
      this.dst = dst;
      this.dstPos = dstPos;
      this.dstToken = dstToken;
      mValue = val;
      mMask = mask;
      mDepth = Math.Max(src.Depth, dst.Depth)+1;
    }
  }
	
  private:
	      IExpression src;
    IExpression dst;
    Int2 srcPos;
    Int2 dstPos;
    Token srcToken;
    Cube.Side srcSide;
    Token dstToken;

	Fraction mValue; // mutable because it changes when the difficulty changes

	const ShapeMask mMask;
    const int mDepth;


  static class OpHelper {
    delegate Fraction OpImpl(Fraction u, Fraction v);
    static OpImpl[] kImpls = {
      (u,v) => u + v,
      (u,v) => u - v,
      (u,v) => u * v,
      (u,v) => u / v,
    };
    public static Fraction Compute(Fraction left, Op op, Fraction right) {
      return kImpls[(int)op](left, right);
    }
  }  
  
}
