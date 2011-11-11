using System;
using System.Collections.Generic;
using System.Drawing;

namespace MapTool {

  public static class MapBuilder {

    //-------------------------------------------------------------------------
    // TILE EXTENSION METHODS
    //-------------------------------------------------------------------------

    public static bool IsObstacle(this TmxTile t) { return t.ContainsKey("obstacle"); }
    public static bool IsWall(this TmxTile t) { return t.ContainsKey("wall"); }
    public static bool IsDoor(this TmxTile t) { return t.ContainsKey("door"); }
    public static bool IsOpen(this TmxTile t) { return !t.IsDoor() && !t.IsObstacle() && !t.IsWall(); }

    public static Portal PortalType(this TmxTile t) {
      if (t.IsDoor()) {
        return Portal.UnlockedDoor;
      } else if (t.IsWall() || t.IsObstacle()) {
        return Portal.Walled;
      } else {
        return Portal.Open;
      }
    }

    public static Map Build(TmxMap tmap) {

      // identify the key layer
      if (tmap.backgroundLayer == null) {
        Console.WriteLine("BUILD_MAP_ERROR: Tiled Map does not contain a 'background' layer.");
        return null;
      }
      var layer = tmap.backgroundLayer;
      if (tmap.backgroundTileSet.TileCountX * tmap.backgroundTileSet.TileCountY >= 256) {
        Console.WriteLine("BUILD_MAP_ERROR: TileSet too large (must have <256 tiles)");
        return null;
      }

      var result = new Map(
        tmap.PixelWidth/128,
        tmap.PixelHeight/128
      ) {
        name = tmap.name,
        tmxData = tmap
      };

      // infer portal states
      for(int rx=0; rx<result.Width; ++rx) {
        for(int ry=0; ry<result.Height; ++ry) {
          var room = result.rooms[rx,ry];
          int pixelX = 128 * rx;
          int pixelY = 128 * ry;
          int tileX = pixelX / tmap.tilePixelWidth;
          int tileY = pixelY / tmap.tilePixelHeight;
          int tileSteps = 128 / tmap.tilePixelWidth;

          // top
          TmxTile tile = null;
          for(int i=0; i<tileSteps; i+=tile.TileWidth) {
            tile = layer.GetTile(tileX+i, tileY);
            if ((room.portals[0] = tile.PortalType()) != Portal.Walled) { break; }
          }

          // left
          tile = null;
          for(int i=0; i<tileSteps; i+=tile.TileHeight) {
            tile = layer.GetTile(tileX, tileY+i);
            if ((room.portals[1] = tile.PortalType()) != Portal.Walled) { break; }
          }

          // bottom
          tile = null;
          for(int i=0; i<tileSteps; i+=tile.TileWidth) {
            tile = layer.GetTile(tileX+i, tileY+tileSteps-1);
            if ((room.portals[2] = tile.PortalType()) != Portal.Walled) { break; }
          }

          // right
          tile = null;
          for(int i=0; i<tileSteps; i+=tile.TileWidth) {
            tile = layer.GetTile(tileX+tileSteps-1, tileY+i);
            if ((room.portals[3] = tile.PortalType()) != Portal.Walled) { break; }
          }

        }
      }

      // validate portals
      for(int ry=0; ry<result.Height; ++ry) {
        if (result.rooms[0, ry].portals[1] == Portal.Open || result.rooms[result.Width-1, ry].portals[3] == Portal.Open) {
          Console.WriteLine("BUILD_MAP_ERROR: Boundary Wall Open");
          return null;
        }
      }
      for(int rx=0; rx<result.Width; ++rx) {
        if (result.rooms[rx, 0].portals[0] == Portal.Open || result.rooms[rx, result.Height-1].portals[2] == Portal.Open) {
          Console.WriteLine("BUILD_MAP_ERROR: Boundary Wall Open");
          return null;
        }
      }
      for(int rx=0; rx<result.Width-1; ++rx) {
        for(int ry=0; ry<result.Height; ++ry) {
          if (result.rooms[rx,ry].portals[3] != result.rooms[rx+1,ry].portals[1]) {
            Console.WriteLine("BUILD_MAP_ERROR: Portal Type Mismatch between {0},{1} and {2},{3}", rx, ry, rx+1, ry);
            return null;
          }
        }
      }
      for(int rx=0; rx<result.Width; ++rx) {
        for(int ry=0; ry<result.Height-1; ++ry) {
          if (result.rooms[rx,ry].portals[2] != result.rooms[rx, ry+1].portals[0]) {
              Console.WriteLine("BUILD_MAP_ERROR: Portal Type Mismatch between {0},{1} and {2},{3}", rx, ry, rx, ry+1);
              return null;
          }
        }
      }

      // create object factories
      var triggerFactory = new TriggerFactory();

      // look through objects
      foreach(TmxObject obj in tmap.objects) {
        var room = result.RoomAtPixelLocation(obj.pixelX, obj.pixelY);
        if (room == null) {
          Console.WriteLine("BUILD_MAP_WARNING: Object Out of Bounds: " + obj.name);
          continue;
        }
        Trigger t = null;
        if ((t = triggerFactory.TryCreateTriggerFrom(obj)) != null) {
          if (room.trigger != null) {
            Console.WriteLine("BUILD_MAP_WARNING: Too Many Triggers in Room ({0}, {1})", room.x, room.y);
          } else {
            room.trigger = t;
            t.room = room;
            t.worldBounds = new Rectangle(
              obj.pixelX,
              obj.pixelY,
              obj.pixelW,
              obj.pixelH
            );
            t.name = obj.name;
          }
        }
      }
      return result;
    }

  }


}

