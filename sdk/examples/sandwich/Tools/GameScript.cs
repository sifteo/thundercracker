using System;
using System.Collections.Generic;
using System.Xml;

namespace MapTool {

  public class GameScript {

    public class State {
      public string id;
      public List<Flag> flags = new List<Flag>(32);
    }

    public class Flag {
      public string id;
    }

    public List<State> states = new List<State>();

    public static GameScript LoadFromXML() {
      var result = new GameScript();
      var xml = new XmlDocument();
      xml.Load("game-script.xml");
      foreach(XmlNode state in xml.SelectNodes("game-script/state")) {
        var sid = state.Attributes["id"].InnerText.ToString();
        if (!result.states.TrueForAll(s => s.id != sid)) {
          throw new Exception(string.Format("Repeat state ID in game script: {0}", sid));
        }
        var stateInst = new State() { id = sid };
        foreach(XmlNode flag in state.SelectNodes("flag")) {
          var fid = flag.Attributes["id"].InnerText.ToString();
          if (!stateInst.flags.TrueForAll(f => f.id != fid)) {
            throw new Exception(string.Format("Repeat flag IDs in game state: {0}", stateInst.id));
          }
          stateInst.flags.Add(new Flag() { id = fid });
          if (stateInst.flags.Count > 32) {
            throw new Exception(string.Format("Too many flags for game state: {0}", stateInst.id));
          }
        }
        result.states.Add(stateInst);
      }
      return result;
    }

  }
}

