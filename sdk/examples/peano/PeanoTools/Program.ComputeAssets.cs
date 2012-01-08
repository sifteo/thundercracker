using System;
using System.Drawing;

namespace PeanoTools {
	
	 static  partial class Program {

		static void ComputeAssets () {
      Console.WriteLine("[ Computing Assets ]");
      Console.WriteLine("- Loading Source Materials");
      using(var src = new SourceMaterials()) {
        using(var canvas = new Bitmap(128,128)) {
  
          Console.WriteLine("- Rendering Center Assets");
          using(var img = new Bitmap(8, 8 * 6)) {
            // iterate through all five levels
            for(int i=0; i<5; ++i) {
              img.FillRect(0, 8*i, 8, 8, src.Colors[i]);
            }
            img.FillRect(0, 8*5, 8, 8, src.BackgroundColor);
            img.Save("assets_center.png");
          }
  
          Console.WriteLine("- Rendering Horizontal/Vertical Assets");
          using (var img = new Bitmap(64, 8 * 32)) {
            // iterate through all the possibilities (2^5 = 32)
            for(BitSet bits = 0; bits.mask < 32; bits.mask++) {
              // draw background and bands back-to-front
              img.DrawBitmap(src.Background, 24, 0, 64, 8, 0, 8*bits.mask);
              for(int i=4; i>= 0; --i) {
                if (bits.Contains(i)) {
                  img.FillRect(16 - 4*i, 8*bits.mask, 32 + 8*i, 8, src.Colors[i]);
                }
              }
            }
            img.Save("assets_horizontal.png");
            img.RotateFlip(RotateFlipType.Rotate270FlipNone);
            img.Save("assets_vertical.png");
          }
  
          Console.WriteLine("- Rendering Major Joint Assets");
  
          // Cardinal Directions
          const int kPossibilities = 112; // precalculated by counting :P
          using(var img = new Bitmap(64, 16 * kPossibilities)) {
            int cnt = 0;
            // iterate through all the possibilities for the "union" mask
            for(BitSet union = 0; union.mask < 32; ++union.mask) {
              // render everything pointing up top
              canvas.Composite(src, union, union);
              img.DrawBitmap(canvas, 24, 8, 64, 16, 0, 16 * cnt);
              cnt++;
              // iterate through removing all the lower levels from the union
              int bitCount = union.Count;
              var bitSet = union;
              for(int i=0; i<bitCount; ++i) {
                bitSet = bitSet.WithoutLowestBit;
                canvas.Composite(src, union, bitSet);
                img.DrawBitmap(canvas, 24, 8, 64, 16, 0, 16 * cnt);
                cnt++;
              }
            }
            img.Save("assets_jointN.png");
            img.RotateFlip(RotateFlipType.Rotate270FlipNone);
            img.Save("assets_jointW.png");
            img.RotateFlip(RotateFlipType.Rotate270FlipNone);
            img.Save("assets_jointS.png");
            img.RotateFlip(RotateFlipType.Rotate270FlipNone);
            img.Save("assets_jointE.png");
          }
  
          // Diagonals
          using (var img = new Bitmap(16, 16 * 32)) {
            for(BitSet union=0; union.mask < 32; ++union.mask) {
              canvas.Composite(src, union, 0);
              img.DrawBitmap(canvas, 24, 24, 16, 16, 0, 16*union.mask);
            }
            img.Save("assets_jointNW.png");
            img.RotateFlip(RotateFlipType.Rotate270FlipNone);
            img.Save("assets_jointSW.png");
            img.RotateFlip(RotateFlipType.Rotate270FlipNone);
            img.Save("assets_jointSE.png");
            img.RotateFlip(RotateFlipType.Rotate270FlipNone);
            img.Save("assets_jointNE.png");
          }

          Console.WriteLine("- Rendering Minor Joint Assets");

          // Cardinal Directions
          using (var img = new Bitmap(32, 8*4)) {

            for(BitSet union=0; union.mask<4; ++union.mask) {
              canvas.Composite(src, union, 0);
              img.DrawBitmap(canvas, 40, 24, 32, 8, 0, 8*union.mask);
            }

            img.Save("assets_minorN.png");
            img.RotateFlip(RotateFlipType.Rotate270FlipNone);
            img.Save("assets_minorW.png");
            img.RotateFlip(RotateFlipType.Rotate270FlipNone);
            img.Save("assets_minorS.png");
            img.RotateFlip(RotateFlipType.Rotate270FlipNone);
            img.Save("assets_minorE.png");

          }

          // Diagonals
          using (var img = new Bitmap(8, 8*2)) {
            for(int i=0; i<2; ++i) {
              BitSet union = i<<4;
              canvas.Composite(src, union, 0);
              img.DrawBitmap(canvas, 16, 16, 8, 8, 0, 8*i);
            }
            img.Save("assets_minorNW.png");
            img.RotateFlip(RotateFlipType.Rotate270FlipNone);
            img.Save("assets_minorSW.png");
            img.RotateFlip(RotateFlipType.Rotate270FlipNone);
            img.Save("assets_minorSE.png");
            img.RotateFlip(RotateFlipType.Rotate270FlipNone);
            img.Save("assets_minorNE.png");
          }
        }
      }
      Console.WriteLine("...Success!");
		}

    // bitmap utilities

    public static void Composite(this Bitmap bmp, SourceMaterials src, BitSet union, BitSet top) {
      bmp.DrawBitmap(src.Background, 0, 0, 128, 128, 0, 0);
      for(int i=4; i>=0; --i) {
        if (union.Contains(i)) {
          bmp.DrawBitmap(src.Levels[i], 0, 0, 128, 128, 0, 0);
        }
        if (top.Contains(i)) {
          bmp.FillRect(56 - 16 - 4 * i, 0, 32 + 8 * i, 56, src.Colors[i]);
        }
      }
    }

    public static void FillRect(this Bitmap bmp, int x, int y, int w, int h, Color c) {
      for(int i=0; i<w; ++i) {
        for(int j=0; j<h; ++j) {
          bmp.SetPixel(x+i, y+j, c);
        }
      }
    }

    public static void DrawBitmap(this Bitmap bmp, Bitmap src, int sx, int sy, int sw, int sh, int x, int y) {
      for(int i=0; i<sw; ++i) {
        for(int j=0; j<sh; ++j) {
          var px = src.GetPixel(sx+i, sy+j);
          if (px.A == 255 && x+i >= 0 && x+i < bmp.Width && y+j >= 0 && j+j < bmp.Height) {
            bmp.SetPixel(x+i, y+j, px);
          }
        }
      }
    }

	}

}
