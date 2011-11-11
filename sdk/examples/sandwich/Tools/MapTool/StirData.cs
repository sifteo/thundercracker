using System;
using System.Collections.Generic;

namespace MapTool {

  public class StirData {
    public Dictionary<string, UnpinnedAssetImage> unpinnedAssetImages = new Dictionary<string, UnpinnedAssetImage>();
    public Dictionary<string, PinnedAssetImage> pinnedAssetImages = new Dictionary<string, PinnedAssetImage>();

    public AssetImage this[string id] {
      get {
        if (unpinnedAssetImages.ContainsKey(id)) { return unpinnedAssetImages[id]; }
        if (pinnedAssetImages.ContainsKey(id)) { return pinnedAssetImages[id]; }
        return null;
      }
    }
  }

  public abstract class AssetImage {
    public string name;
    public int width;
    public int height;
    public int frames;
    public abstract ushort TileAt(int x, int y, int frame=0);
  }


  public class UnpinnedAssetImage : AssetImage {
    public ushort[] tiles;

    override public ushort TileAt(int x, int y, int frame=0) {
      return tiles[width * height * frame + x + y * width];
    }
  }

  public class PinnedAssetImage : AssetImage {
    public ushort firstTile;

    override public ushort TileAt(int x, int y, int frame=0) {
      return (ushort)(firstTile + width * height * frame + x + y * width);
    }
  }

}

