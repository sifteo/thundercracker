using System;
using System.Collections.Generic;
using System.IO;

namespace MapTool {

  public class TmxMap {
    public string name;
    public int width;
    public int height;
    public int tilePixelWidth;
    public int tilePixelHeight;

    public TmxTileset backgroundTileSet;
    public TmxLayer backgroundLayer;

    public Dictionary<string,TmxTileset> tileSets = new Dictionary<string, TmxTileset>();
    public Dictionary<string,TmxLayer> nameToLayer = new Dictionary<string, TmxLayer>();
    public List<TmxLayer> layers = new List<TmxLayer>();
    public List<TmxObject> objects = new List<TmxObject>();

    public int PixelWidth { get { return tilePixelWidth * width; } }
    public int PixelHeight { get { return tilePixelHeight * height; } }

    public TmxTile GetTile(int gid) {
      if (gid != 0) {
        foreach(var ts in tileSets.Values) {
          var result = ts.GetTile(gid);
          if (result != null) {
            return result;
          }
        }
      }
      return null;
    }

  }

  public class ImageData {
    public string path;
    public int pixelWidth;
    public int pixelHeight;
  }

  public class TmxTileset {
    // Assuming all tilesets are tightly packed (no margin or spacing) because that's
    // how stir is going to want them anyway.
    public TmxMap map;
    public int firstGid;
    public string name;
    public int tilePixelWidth;
    public int tilePixelHeight;
    public ImageData image = new ImageData();

    public TmxTile[,] tiles;

    public int TileCountX { get { return tiles.GetLength(0); } }
    public int TileCountY { get { return tiles.GetLength(1); } }
    public int TileCount { get { return TileCountX * TileCountY; } }

    public int LocalTileId(int x, int y) {
      return y * TileCountX + x;
    }

    public bool Contains(int tileId) {
      return tileId >= firstGid && tileId < firstGid + TileCount;
    }

    public TmxTile GetTile(int gid) {
        gid -= firstGid;
        if (gid >= 0 && gid < TileCount) {
          return tiles[gid % TileCountX, gid / TileCountX];
        }
        return null;
    }
  }

  public class TmxTile : Dictionary<string,string> {
    public int gid;
    public TmxTileset tileSet;
    public int imageX;
    public int imageY;
    public int ImagePixelX { get { return tileSet.tilePixelWidth * imageX; } }
    public int ImagePixelY { get { return tileSet.tilePixelHeight * imageY; } }
    public int TilePixelWidth { get { return tileSet.tilePixelWidth; } }
    public int TilePixelHeight { get { return tileSet.tilePixelHeight; } }
    public int TileWidth { get { return tileSet.tilePixelWidth / tileSet.map.tilePixelWidth; } }
    public int TileHeight { get { return tileSet.tilePixelHeight / tileSet.map.tilePixelHeight; } }

    public int LocalId {
      get { return tileSet.LocalTileId(imageX, imageY); }
    }

  }

  public class TmxLayer : Dictionary<string,string> {
    public TmxMap map;
    public string name;
    public int[,] tiles;
    public bool visible = true;
    public float opacity = 1f;
    public int Width { get { return tiles.GetLength(0); } }
    public int Height { get { return tiles.GetLength(1); }}
    public TmxTile GetTile(int x, int y) {

      return map.GetTile(tiles[x,y]);
    }
  }

  public class TmxObject : Dictionary<string,string> {
    public string name;
    public string type;
    public int pixelX;
    public int pixelY;
    public int pixelW;
    public int pixelH;
    public TmxTile tile;

    public int TileX { get { return pixelX / 16; } }
    public int TileY { get { return pixelY / 16; } }
    public int TileWidth { get { return Math.Min(1, pixelW / 16); } }
    public int TileHeight { get { return Math.Min(1, pixelH / 16); } }


  }


}

