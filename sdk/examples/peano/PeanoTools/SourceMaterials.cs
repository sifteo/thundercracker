using System;
using System.Drawing;

namespace PeanoTools {

  public class SourceMaterials : IDisposable {

    public Bitmap Background;
    public Bitmap BackgroundLit;
    public Bitmap[] Levels = new Bitmap[5];
    public Color[] Colors = new Color[5];
    public Color BackgroundColor;

    public SourceMaterials () {
      Background = new Bitmap("background.png");
      BackgroundLit = new Bitmap("background_lit.png");
      BackgroundColor = Background.GetPixel(56, 56);
      for(int i=0; i<5; i++) {
        var bmp = new Bitmap(string.Format("level{0}.png", i));
        Colors[i] = bmp.GetPixel(bmp.Width>>1, bmp.Height>>1);
        Levels[i] = new Bitmap(128, 128);
        Levels[i].FillRect(0, 0, 128, 128, Color.Transparent);
        Levels[i].DrawBitmap(bmp, 0, 0, bmp.Width, bmp.Height, 56 - (bmp.Width>>1), 56 - (bmp.Height>>1));
      }
    }

    public void Dispose() {
      Background.Dispose();
      BackgroundLit.Dispose();
      for(int i=0; i<5; ++i) {
        Levels[i].Dispose();
      }
    }

  }

}

