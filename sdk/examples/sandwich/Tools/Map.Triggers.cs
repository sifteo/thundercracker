using System;
using System.Collections.Generic;
using System.Drawing;

namespace MapTool {

  //---------------------------------------------------------------------------
  // BASE TRIGGER INTERFACE
  //---------------------------------------------------------------------------

  public abstract class Trigger {
    public string name;
    public Room room;
    public Rectangle worldBounds;

    public abstract string GeneratedCode();

    public delegate Trigger Builder(Dictionary<string,string> props);
  }

  //---------------------------------------------------------------------------
  // ABSTRACT TRIGGER FACTORY (w/ hard-coded product registration)
  //---------------------------------------------------------------------------

  public class TriggerFactory {
    Dictionary<string,Trigger.Builder> mBuilders = new Dictionary<string, Trigger.Builder>();

    public TriggerFactory() {
      mBuilders.Add("Gateway", TeleportTrigger.CreateFromProperties);
    }

    public Trigger TryCreateTriggerFrom(TmxObject obj) {
      Trigger.Builder builder;
      if (!mBuilders.TryGetValue(obj.type, out builder)) {
        return null;
      }
      return builder(obj);
    }
  }

  //---------------------------------------------------------------------------
  // CONCRETE TRIGGERS
  //---------------------------------------------------------------------------

  public class TeleportTrigger : Trigger {
    public string targetMap;
    public string targetTeleportTrigger;

    public static TeleportTrigger CreateFromProperties(Dictionary<string,string> props) {
      var target = props["target"];
      var index = target.IndexOf(':');
      if (index == -1) {
        return new TeleportTrigger() {
          targetMap = "",
          targetTeleportTrigger = target
        };
      } else {
        return new TeleportTrigger() {
          targetMap = target.Substring(0, index),
          targetTeleportTrigger = target.Substring(index+1)
        };
      }
    }

    public int PixelX { get { return worldBounds.X + worldBounds.Width/2; } }
    public int PixelY { get { return worldBounds.Y + worldBounds.Height/2; } }

    override public string GeneratedCode() {
      var map = targetMap.Length > 0 ? room.map.database.NameToMap[targetMap] : room.map;
      TeleportTrigger tt = null;
      foreach(var r in map.rooms) {
        var p = r.trigger as TeleportTrigger;
        if (p != null && p.name == targetTeleportTrigger) {
          tt = p;
          break;
        }
      }
      return string.Format(
        "\n    if (type == TRIGGER_TYPE_ACTIVE) {{" +
        "\n        pGame->WalkTo(Vec2({0}, {1}));" +
        "\n        pGame->TeleportTo({2}_data, Vec2({3}, {4}));" +
        "\n    }}\n",
        PixelX, PixelY,
        targetMap, tt.PixelX, tt.PixelY
      );
    }
  }

}

