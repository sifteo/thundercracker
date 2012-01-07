using System;
using System.Drawing;

namespace PeanoTools {
	
	 static  partial class Program {

		static void ComputeAssets () {
      Console.WriteLine("[ Computing Assets ]");
      Console.WriteLine("- Loading Source Materials");
      using(var src = new SourceMaterials()) {

        Console.WriteLine("- Rendering Center Assets");
        using(var img = new Bitmap(8, 8 * 5)) {
          for(int i=0; i<5; ++i) {
            img.FillRect(0, 8*i, 8, 8, src.Colors[i]);
          }
          img.Save("assets_center.png");
        }

        Console.WriteLine("- Rendering Horizontal/Vertical Assets");
        using (var img = new Bitmap(8 * 8, 8)) {
          
        }

        // TODO

        Console.WriteLine("- Rendering Major Joint Assets");
        // TODO

        Console.WriteLine("- Rendering Minor Joint Assets");
        // TODO

      }
      Console.WriteLine("...Success!");
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
          if (px.A == 255) { bmp.SetPixel(x+i, y+j, px); }
        }
      }
    }

	}

}
