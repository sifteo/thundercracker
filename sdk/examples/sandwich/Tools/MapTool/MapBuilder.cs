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
    public static bool IsWalkable(this TmxTile t) { return !t.IsWall() && !t.IsObstacle(); }
    public static bool IsPath(this TmxTile t) { return t.ContainsKey("path"); }

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

      // TODO: MAKE SURE PORTAL OPENINGS ARE TWO-WIDE VERTICALLY!

      // infer portal states
      for(int rx=0; rx<result.Width; ++rx) {
        for(int ry=0; ry<result.Height; ++ry) {
          var room = result.rooms[rx,ry];

          if(room.IsBlocked) {
            room.portals[0] = Portal.Walled;
            room.portals[1] = Portal.Walled;
            room.portals[2] = Portal.Walled;
            room.portals[3] = Portal.Walled;
            continue;
          }

          int tileX = 8 * rx;
          int tileY = 8 * ry;

          // top
          var prevType = room.portals[0] = Portal.Walled;
          for(int i=0; i<8; i++) {
            var type = layer.GetTile(tileX+i, tileY).PortalType();
            if (type == Portal.Walled) {
            } else if (type == Portal.Open) {
              if (prevType == Portal.Open) {
                room.portals[0] = Portal.Open;
                break;
              }
            } else {
              room.portals[0] = type;
              break;
            }
            prevType = type;
          }
          // double-check top
          if (room.portals[0] != Portal.Walled && (room.y == 0 || result.rooms[room.x, room.y-1].IsBlocked)) {
            room.portals[0] = Portal.Walled;
          }

          // left
          prevType = room.portals[1] = Portal.Walled;
          for(int i=0; i<8; i++) {
            var type = layer.GetTile(tileX, tileY+i).PortalType();
            if (type == Portal.Walled) {
            } else if (type == Portal.Open) {
              //if (prevType == Portal.Open) {
                room.portals[1] = Portal.Open;
                break;
              //}
            } else {
              room.portals[1] = type;
              break;
            }
            prevType = type;
          }
          // double-check left
          if (room.portals[1] != Portal.Walled && (room.x == 0 || result.rooms[room.x-1, room.y].IsBlocked)) {
            room.portals[1] = Portal.Walled;
          }

          // bottom
          prevType = room.portals[2] = Portal.Walled;
          for(int i=0; i<8; i++) {
            var type = layer.GetTile(tileX+i, tileY+7).PortalType();
            if (type == Portal.Walled) {
            } else if (type == Portal.Open) {
              if (prevType == Portal.Open) {
                room.portals[2] = Portal.Open;
                break;
              }
            } else {
              room.portals[2] = type;
              break;
            }
            prevType = type;          }
          // double-check bottom
          if (room.portals[2] != Portal.Walled && (room.y == result.Height-1 || result.rooms[room.x, room.y+1].IsBlocked)) {
            room.portals[2] = Portal.Walled;
          }

          // right
          prevType = room.portals[3] = Portal.Walled;
          for(int i=0; i<8; i++) {
            var type = layer.GetTile(tileX+7, tileY+i).PortalType();
            if (type == Portal.Walled) {
            } else if (type == Portal.Open) {
              //if (prevType == Portal.Open) {
                room.portals[3] = Portal.Open;
                break;
              //}
            } else {
              room.portals[3] = type;
              break;
            }
            prevType = type;
          }
          // double-check right
          if (room.portals[3] != Portal.Walled && (room.x == result.Width-1 || result.rooms[room.x+1, room.y].IsBlocked)) {
            room.portals[3] = Portal.Walled;
          }

        }
      }

      // look through BasicLocks and convert those portals into locks
      foreach(var obj in tmap.objects) {
        if (obj.type == "BasicLock") {
          if (obj.pixelW > obj.pixelH) {
            // this is an X-Portal
            var lroom = result.rooms[obj.pixelX/128, obj.pixelY/128];
            var rroom = result.rooms[obj.pixelX/128+1, obj.pixelY/128];
            if (lroom.portals[3] != Portal.UnlockedDoor || rroom.portals[1] != Portal.UnlockedDoor) {
              Console.WriteLine("BUILD_MAP_ERROR: Non-Door annotated with a lock");
              return null;
            }
            lroom.portals[3] = Portal.LockedDoor;
            rroom.portals[1] = Portal.LockedDoor;
          } else {
            // this is a Y-Portal
            var troom = result.rooms[obj.pixelX/128, obj.pixelY/128];
            var broom = result.rooms[obj.pixelX/128, obj.pixelY/128+1];
            if (troom.portals[2] != Portal.UnlockedDoor || broom.portals[0] != Portal.UnlockedDoor) {
              Console.WriteLine("BUILD_MAP_ERROR: Non-Door annotated with a lock");
              return null;
            }
            troom.portals[2] = Portal.LockedDoor;
            broom.portals[0] = Portal.LockedDoor;
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

