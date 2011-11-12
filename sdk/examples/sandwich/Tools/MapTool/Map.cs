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

    public Room(Map map, int x, int y) {
      this.map = map;
      this.x = x;
      this.y = y;
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

    public Room RoomAtPixelLocation(int pixelX, int pixelY) {
      pixelX /= 128;
      pixelY /= 128;
      return rooms[pixelX, pixelY];
    }

  }

}

