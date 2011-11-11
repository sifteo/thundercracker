using System;

namespace MapTool {
  public static class ConsoleDrawing {

    public static void DrawBackgroundLayerMask(this TmxMap map) {
      if (!map.nameToLayer.ContainsKey("background")) { return; }
      var layer = map.nameToLayer["background"];
      for(int y=0; y<layer.Height; ++y) {
        for(int x=0; x<layer.Width; ++x) {
          var tile = layer.GetTile(x,y);
          if (tile.IsObstacle()) {
            Console.Write("O");
          } else if (tile.IsDoor()) {
            Console.Write("D");
          } else if (tile.IsWall()) {
            Console.Write("W");
          } else {
            Console.Write(" ");
          }
        }
        Console.WriteLine("");
      }
    }

    public static void DrawPortals(this Map map) {
      for(int ry=0; ry<map.Height; ++ry) {
        for(int rx=0; rx<map.Width; ++rx) {
          Console.Write("+");
          switch(map.rooms[rx,ry].portals[0]) {
            case Portal.Open: Console.Write(" "); break;
            case Portal.LockedDoor: Console.Write("L"); break;
            case Portal.UnlockedDoor: Console.Write("U"); break;
            default: Console.Write("-"); break;
          }
        }
        Console.Write("+\n");
        for(int rx=0; rx<map.Width; ++rx) {
          switch(map.rooms[rx,ry].portals[1]) {
            case Portal.Open: Console.Write(" "); break;
            case Portal.LockedDoor: Console.Write("L"); break;
            case Portal.UnlockedDoor: Console.Write("U"); break;
            default: Console.Write("|"); break;
          }
          Console.Write(" ");
        }
        switch(map.rooms[map.Width-1,ry].portals[3]) {
          case Portal.Open: Console.Write(" \n"); break;
          case Portal.LockedDoor: Console.Write("L\n"); break;
          case Portal.UnlockedDoor: Console.Write("U\n"); break;
          default: Console.Write("|\n"); break;
        }
      }
      for(int rx=0; rx<map.Width; ++rx) {
        Console.Write("+");
        switch(map.rooms[rx,map.Height-1].portals[2]) {
          case Portal.Open: Console.Write(" "); break;
          case Portal.LockedDoor: Console.Write("L"); break;
          case Portal.UnlockedDoor: Console.Write("U"); break;
          default: Console.Write("-"); break;
        }
      }
      Console.WriteLine("+");
    }

  }
}

