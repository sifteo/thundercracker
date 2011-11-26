using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Text;

namespace Misc {
  public static class BatchPearlSprite {

    public static void Exec() {

      // write stand.png
      using (var stand = new Bitmap("/Users/max/Downloads/sandwich/princess-stand.png")) {
        using (var result = new Bitmap(stand.Height, stand.Width)) {
          int nFrames = stand.Width / 32;
          for(int frame=0; frame<nFrames; ++frame) {
            Copy32FromTo(stand, frame, 0, result, 0, frame);
          }
          result.Save("/Users/max/git/thundercracker/sdk/examples/sandwich/stand.png");
        }
      }

      // write walk.png
      {
        var directions = new string[] { "n", "w", "s", "e" };
        Bitmap result = null;
        for(int dir=0; dir<4; ++dir) {
          using(var strip = new Bitmap(string.Format(
            "/Users/max/Downloads/sandwich/princess-run-{0}.png", directions[dir]
          ))) {
            int nFrames = strip.Height / 32;
            if (result == null) { result = new Bitmap(32*nFrames, 32*4); }
            for(int frame=0; frame<nFrames; ++frame) {
              Copy32FromTo(strip, 0, frame, result, frame, dir);
            }
            result.Save("/Users/max/git/thundercracker/sdk/examples/sandwich/walk.png");
          }
        }

      }

    }

    static void Copy32FromTo(Bitmap src, int srcx, int srcy, Bitmap dst, int dstx, int dsty) {
      for(int y=0; y<32; ++y) {
        for(int x=0; x<32; ++x) {
          dst.SetPixel(32*dstx+x, 32*dsty+y, src.GetPixel(32*srcx+x, 32*srcy+y));
        }
      }
    }
  }
}

