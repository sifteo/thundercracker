using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Text;

namespace MapTool {

  public enum Portal {
    Open=0, Walled=1, UnlockedDoor=2, LockedDoor=3
  }

  public enum Side {
    Top=0, Left=1, Bottom=2, Right=3
  }

  public class Room {
    public readonly Portal[] portals = { Portal.Walled, Portal.Walled, Portal.Walled, Portal.Walled };
    public readonly Map map;
    public readonly int x;
    public readonly int y;
    public int Id { get { return x + map.Width * y; } }
    public Trigger trigger = null;
    public string item = "";

    public bool HasItem { get { return item.Length > 0 && item != "ITEM_NONE"; } }

    public bool HasOverlay {
      get {
        if (!map.HasOverlay) { return false; }
        for(int y=0; y<8; ++y) {
          for(int x=0; x<8; ++x) {
            var tile = this.GetOverlayTile(x, y);
            if (tile != null) {
              return true;
            }
          }
        }
        return false;
      }
    }

    public Room(Map map, int x, int y) {
      this.map = map;
      this.x = x;
      this.y = y;
    }

    public Int2 Center {
      get {
        foreach(var loc in Spiral.IntoMadness()) {
          if (//GetTile(loc.x-1, loc.y-1).IsWalkable() &&
              GetTile(loc.x-1, loc.y).IsWalkable() &&
              //GetTile(loc.x, loc.y-1).IsWalkable() &&
              GetTile(loc.x, loc.y).IsWalkable()) {
            return Adjust(loc);
          }
        }
        return new Int2() { x=0, y=0 };

      }
    }

    public Int2 Adjust(Int2 loc) {
      // if you're half-on a two-wide path, snap to it
      if (GetTile(loc.x-1, loc.y).IsPath()) {
        if (!GetTile(loc.x, loc.y).IsPath() && GetTile(loc.x-2, loc.y).IsPath()) {
          loc.x--;
        }
      } else if (GetTile(loc.x, loc.y).IsPath() && GetTile(loc.x+1, loc.y).IsPath()) {
        loc.x++;
      }
      return loc;
    }

    public bool IsBlocked {
      get { return Center.x == 0; }
    }

    public TmxTile GetTile(int x, int y) {
      return map.tmxData.backgroundLayer.GetTile(8 * this.x + x, 8 * this.y + y);
    }

    public TmxTile GetOverlayTile(int x, int y) {
      return map.tmxData.overlayLayer.GetTile(8 * this.x + x, 8 * this.y + y);
    }
  }

  public class Map {
    public MapDatabase database;
    public string name;
    public readonly Room[,] rooms;
    public int Width { get { return rooms.GetLength(0); } }
    public int Height { get { return rooms.GetLength(1); } }
    public TmxMap tmxData;

    public Map (int w, int h) {
      rooms = new Room[w,h];
      for(int x=0; x<w; ++x) {
        for(int y=0; y<h; ++y) {
          rooms[x,y] = new Room(this, x, y);
        }
      }
    }

    public bool HasOverlay {
      get { return tmxData.overlayLayer != null; }
    }

    public Room RoomAtPixelLocation(int pixelX, int pixelY) {
      pixelX /= 128;
      pixelY /= 128;
      return rooms[pixelX, pixelY];
    }

  }

}

