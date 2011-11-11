using System;
using System.Collections.Generic;

namespace MapTool {
  public class MapDatabase {
    public readonly Dictionary<string,Map> NameToMap = new Dictionary<string, Map>();

    public void RegisterMap(Map map) {
      NameToMap.Add(map.name, map);
      map.database = this;
    }
  }
}

