using System;
using System.Drawing;

namespace MapTool {
  public static class MapDrawing {

    public static void RenderPreview(this Map map, string outPath) {
      var tileImage = new Bitmap(map.tmxData.backgroundTileSet.image.path);
      var resultImage = new Bitmap(map.tmxData.PixelWidth, map.tmxData.PixelHeight);
      for(int ty=0; ty<map.tmxData.height; ++ty) {
        for(int tx=0; tx<map.tmxData.width; ++tx) {
          var tile = map.tmxData.backgroundLayer.GetTile(tx, ty);
          Blit(tileImage, tile.imageX, tile.imageY, resultImage, tx, ty);
        }
      }

      using(var g = Graphics.FromImage(resultImage)) {
        // draw idle positions
        var pearl = new Bitmap(32, 32);
        var pearlStrip = new Bitmap("stand.png");
        for(int r=0; r<32; ++r) {
          for(int c=0; c<32; ++c) {
            pearl.SetPixel(c, r, pearlStrip.GetPixel(c, r+64));
          }
        }
        pearlStrip.Dispose();
        foreach(var room in map.rooms) {
          var center = room.Center;
          if (center.x != 0) {
            center.x += 8 * room.x;
            center.y += 8 * room.y;
            center.x *= 16;
            center.y *= 16;
            g.DrawImage(pearl, center.x - 16, center.y - 16);
          }
        }
        pearl.Dispose();

        // draw frames
        var pen = new Pen(Color.Black, 2);
        foreach(var room in map.rooms) {
          g.DrawRectangle(pen, 8*16*room.x, 8*16*room.y, 8*16, 8*16);
        }


        foreach(var loc in Spiral.IntoMadness()) {
          g.DrawEllipse(pen, loc.x * 16-2, loc.y * 16-2, 4, 4);
        }
        
      }

      resultImage.Save(outPath);
      tileImage.Dispose();
      resultImage.Dispose();
    }

    public static void Blit(Bitmap src, int stx, int sty, Bitmap dst, int dtx, int dty) {
      for(int px=0; px<16; ++px) {
        for(int py=0; py<16; ++py) {
          dst.SetPixel(16*dtx+px, 16*dty+py, src.GetPixel(16*stx+px, 16*sty+py));
        }
      }
    }

  }
}

