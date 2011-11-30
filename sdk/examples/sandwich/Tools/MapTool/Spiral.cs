using System;
using System.Collections.Generic;

namespace MapTool {

  public struct Int2 {
    public int x;
    public int y;
  }

  public static class Spiral {

    public static IEnumerable<Int2> IntoMadness() {
      int i=0;
      Int2 result = new Int2() { x = 4, y = 4 };
      yield return result;
      result.y++;
      yield return result;
      result.x++;
      yield return result;
      for(i=0; i<2; ++i) {
        result.y--;
        yield return result;
      }
      for(i=0; i<2; ++i) {
        result.x--;
        yield return result;
      }
      for(i=0; i<3; ++i) {
        result.y++;
        yield return result;
      }
      for(i=0; i<3; ++i) {
        result.x++;
        yield return result;
      }
      for(i=0; i<4; ++i) {
        result.y--;
        yield return result;
      }
      for(i=0; i<4; ++i) {
        result.x--;
        yield return result;
      }
      for(i=0; i<4; ++i) {
        result.y++;
        yield return result;
      }
    }

  }


}

