using System;
using System.IO;
using System.Text;
using System.Collections.Generic;


namespace MapTool {

  public static class MapExporter {

    public static void GenerateHeader(this Map map, TextWriter stream) {
      stream.WriteLine(string.Format("extern MapData {0}_data;", map.name));
    }

    public static void GenerateSource(this Map map, TextWriter stream) {

      stream.WriteLine("//------------------------------------------------------------------------------");
      stream.WriteLine("// EXPORTED FROM {0}.tmx", map.name);
      stream.WriteLine("//------------------------------------------------------------------------------\n");

      // write xportals
      stream.WriteLine("static uint8_t {0}_xportals[] = {{", map.name);
      for(int y=0; y<map.Height; ++y) {
        stream.Write("    ");
        for(int x=0; x<map.Width; ++x) {
          stream.Write( kPortalLabels[(int)map.rooms[x,y].portals[(int)Side.Left]] );
          stream.Write(", ");
        }
        stream.Write( kPortalLabels[(int)map.rooms[map.Width-1, y].portals[(int)Side.Right]] );
        stream.Write(",\n");
      }
      stream.WriteLine(" };");

      // write yportals
      stream.WriteLine("static uint8_t {0}_yportals[] = {{", map.name);
      for(int x=0; x<map.Width; ++x) {
        stream.Write("    ");
        for(int y=0; y<map.Height; ++y) {
          stream.Write( kPortalLabels[(int)map.rooms[x,y].portals[(int)Side.Top]] );
          stream.Write(", ");
        }
        stream.Write( kPortalLabels[(int)map.rooms[x, map.Height-1].portals[(int)Side.Bottom]] );
        stream.Write(",\n");
      }
      stream.WriteLine("};");

      // find triggers, items
      var triggers = new List<Trigger>();
      var items = new List<Trigger>();
      foreach(var room in map.rooms) {
        if (room.trigger != null) {
          triggers.Add(room.trigger);
          stream.WriteLine(
            "static void {0}_trigger_{1}(int type) {{ {2} }}",
            map.name, room.Id, room.trigger.GeneratedCode()
          );
          if (room.trigger.HasItem) {
            items.Add(room.trigger);
          }
        }
      }

      // write triggers
      var triggerListName = "0";
      if (triggers.Count > 0) {
        triggerListName = map.name + "_triggers";
        stream.WriteLine("static TriggerData {0}[] = {{", triggerListName);
        foreach(var trigger in triggers) {
          stream.WriteLine("    {{ {0}, {1}_trigger_{0} }},", trigger.room.Id, map.name);
        }
        stream.WriteLine("    { -1, 0 } ");
        stream.WriteLine("};");
      }

      // write items
      var itemListName = "0";
      if (items.Count > 0) {
        itemListName = map.name + "_items";
        stream.WriteLine("static ItemData {0}[] = {{", itemListName);
        foreach(var item in items) {
          stream.WriteLine("    {{ {0}, {1} }},", item.room.Id, item.ItemId);
        }
        stream.WriteLine("    { -1, 0 }");
        stream.WriteLine("};");
      }

      // write rooms
      stream.WriteLine("static RoomData {0}_rooms[] = {{", map.name);
      for(int y=0; y<map.Height; ++y) {
        for(int x=0; x<map.Width; ++x) {
          stream.WriteLine("    {");
          // write collision mask rows
          stream.WriteLine("        {");
          stream.Write("            ");
          for(int row=0; row<8; ++row) {
            int rowMask = 0;
            for(int col=0; col<8; ++col) {
              var tile = map.tmxData.backgroundLayer.GetTile(8 * x + col, 8 * y + row);
              if (tile.IsWalkable()) {
                rowMask |= (1<<col);
              }
            }
            // todo
            stream.Write("0x{0:X2}, ", rowMask);
          }
          stream.WriteLine("\n        },");
          // write tiles
          stream.WriteLine("        {");
          for(int ty=0; ty<8; ++ty) {
            stream.Write("            ");
            for(int tx=0; tx<8; ++tx) {
              var tile = map.tmxData.backgroundLayer.GetTile(8 * x + tx, 8 * y + ty);
              stream.Write("0x{0:X2}, ", Convert.ToByte(tile.LocalId));
            }
            stream.Write('\n');
          }
          stream.WriteLine("        }");
          stream.WriteLine("    },");
        }
      }
      stream.WriteLine("};");

      // write data
      stream.WriteLine(
        "MapData {0}_data = {{ &TileSet_{0}, {0}_rooms, {0}_xportals, {0}_yportals, {1}, {2}, {3}, {4} }};",
        map.name, triggerListName, itemListName, map.Width, map.Height
      );
    }

    static string[] kPortalLabels = {
      "PORTAL_OPEN", "PORTAL_WALL", "PORTAL_DOOR", "PORTAL_LOCK"
    }; // order matters
  }


}

/*

-------------------------------------------------------------------------------
C++ Interface
-------------------------------------------------------------------------------

#define PORTAL_OPEN  0
#define PORTAL_WALL  1
#define PORTAL_DOOR 2
#define PORTAL_LOCK  3

typedef void (*trigger_func)();

struct TriggerData {
    trigger_func callback;
    int room;
};

struct MapData {
    const Sifteo::AssetImage* tileset;
    uint8_t* tiles; // every 64 tiles represents an 8x8 room of 16px tiles
    uint8_t* xportals; // vertical portals between rooms (x,y) and (x+1,y)
    uint8_t* yportals; // horizontal portals between rooms (x,y) and (x,y+1)
    TriggerData* triggers; // is null-terminated
    uint8_t width;
    uint8_t height;
}

-------------------------------------------------------------------------------
Example Output
-------------------------------------------------------------------------------

uint8_t myMap_xportals[] = { ... };
uint8_t myMap_yportals[] = { ... };
uint8_y myMap_tiles[] = { ... };
MapData myMap_data = { &TileSet_myMap, myMap_tiles, myMap_xportals, myMap_yportals, 8, 8  };

*/