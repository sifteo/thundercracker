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
      var items = new List<Room>();
      foreach(var room in map.rooms) {
        if (room.trigger != null) {
          triggers.Add(room.trigger);
          stream.WriteLine(
            "static void {0}_trigger_{1}(int type) {{ {2} }}",
            map.name, room.Id, room.trigger.GeneratedCode()
          );
        }
        if (room.HasItem) {
          items.Add(room);
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
        foreach(var room in items) {
          stream.WriteLine("    {{ {0}, {1} }},", room.Id, room.item);
        }
        stream.WriteLine("    { -1, 0 }");
        stream.WriteLine("};");
      }

      // write overlays
      if (map.HasOverlay) {
        foreach(var room in map.rooms) {
          if (room.HasOverlay) {
            stream.WriteLine("static uint8_t {0}_overlay_{1}_{2}[] = {{", map.name, room.x, room.y);
            stream.Write("    ");
            for(int y=0; y<8; ++y) {
              for(int x=0; x<8; ++x) {
                var tile = room.GetOverlayTile(x,y);
                if (tile != null) {
                  stream.Write(
                    "0x{0:X2}, 0x{1:X2}, ",
                    Convert.ToByte((x<<4)|y),
                    Convert.ToByte(tile.LocalId)
                  );
                }
              }
            }
            stream.Write("0xff");
            stream.WriteLine("\n};");
          }
        }
      }

      // write rooms
      stream.WriteLine("static RoomData {0}_rooms[] = {{", map.name);
      for(int y=0; y<map.Height; ++y)
      for(int x=0; x<map.Width; ++x) {
        var room = map.rooms[x,y];

        stream.WriteLine("    {");
        // write center mask
        var center = room.Center;
        int centerMask = center.y;
        centerMask |= (center.x << 3);
        stream.WriteLine("        0x{0:X2},", centerMask);

        // write collision mask rows
        stream.WriteLine("        {");
        stream.Write("            ");
        for(int row=0; row<8; ++row) {
          int rowMask = 0;
          for(int col=0; col<8; ++col) {
            if (!room.GetTile(col, row).IsWalkable()) {
              rowMask |= (1<<col);
            }
          }
          stream.Write("0x{0:X2}, ", rowMask);
        }
        stream.WriteLine("\n        },");

        // write tiles
        stream.WriteLine("        {");
        for(int ty=0; ty<8; ++ty) {
          stream.Write("            ");
          for(int tx=0; tx<8; ++tx) {
            stream.Write("0x{0:X2}, ", Convert.ToByte(room.GetTile(tx, ty).LocalId));
          }
          stream.Write('\n');
        }
        stream.WriteLine("        },");

        // write torches
        int torchCount = 0;
        foreach(Int2 t in room.ListTorchTiles()) {
          if (torchCount == 2) {
            throw new Exception("Too Many Torches in One Room (capacity 2)");
          }
          stream.WriteLine("        0x{0:X2},", Convert.ToByte((t.x<<4) + t.y));
          torchCount++;
        }
        for(int i=torchCount; i<2; ++i) {
          stream.WriteLine("        0xff, ");
        }

        // write overlay
        if (room.HasOverlay) {
          stream.WriteLine("        {0}_overlay_{1}_{2}", map.name, room.x, room.y);
        } else {
          stream.WriteLine("        0");
        }


        stream.WriteLine("    },");
      }
      stream.WriteLine("};");

      // write data
      stream.WriteLine(
        "MapData {0}_data = {{ &TileSet_{0}, {5}, &Blank_{0}, {0}_rooms, {0}_xportals, {0}_yportals, {1}, {2}, {3}, {4} }};",
        map.name,
        triggerListName,
        itemListName,
        map.Width,
        map.Height,
        map.HasOverlay ? "&Overlay_" + map.name : "0"
      );
    }

    static string[] kPortalLabels = {
      "PORTAL_OPEN", "PORTAL_WALL", "PORTAL_DOOR"
    }; // order matters
  }


}
