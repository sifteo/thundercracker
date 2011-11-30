using System;
using System.Collections.Generic;
using System.Drawing;

namespace MapTool {

  public struct Int2 {
    public int x;
    public int y;

    public Point ToPoint {
      get { return new Point(x, y); }
    }
  }

  public static class Spiral {

    public static IEnumerable<Int2> IntoMadness() {
      int i=0;
      Int2 result = new Int2() { x = 4, y = 4 };
      yield return result;
      result.y++;
      yield return result;
      result.x++; result.y--;
      yield return result;
      result.x--; result.y--;
      yield return result;
      result.x--; result.y++;
      yield return result;
      result.y++;
      yield return result;
      result.x+=2;
      yield return result;
      result.y-=2;
      yield return result;
      result.x-=2;
      yield return result;
      result.x += 1; result.y += 3;
      yield return result;
      result.x += 2; result.y -= 2;
      yield return result;
      result.x -= 2; result.y -= 2;
      yield return result;
      result.x -= 2; result.y += 2;
      yield return result;
      result.y ++;
      yield return result;
      result.x++; result.y ++;
      yield return result;
      result.x += 2;
      yield return result;
      result.x++; result.y--;
      yield return result;
      result.y-=2;
      yield return result;
      result.x--; result.y--;
      yield return result;
      result.x -= 2;
      yield return result;
      result.x--; result.y++;
      yield return result;
      result.y += 3;
      yield return result;
      result.x += 4;
      yield return result;
      result.y -= 4;
      yield return result;
      result.x -= 4;
      yield return result;
      /*
      result.x += 2; result.y += 5;
      yield return result;
      result.x+=3; result.y -= 3;
      yield return result;
      result.x -= 3; result.y -= 3;
      yield return result;
      result.x -= 3; result.y += 3;
      yield return result;
      */

    }

  }


}

