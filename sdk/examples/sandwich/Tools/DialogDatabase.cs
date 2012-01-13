using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Xml;

namespace MapTool {

  public class DialogDatabase {
    public class Text {
      public string image;
      public string text;
    }

    public class Dialog {
      public string id;
      public string npc;
      public List<Text> texts = new List<Text>();
    }

    public List<Dialog> dialogs = new List<Dialog>();

    public static DialogDatabase LoadFromXML() {
      var result = new DialogDatabase();
      var xml = new XmlDocument();
      xml.Load("dialog-database.xml");
      foreach(XmlNode node in xml.SelectNodes("dialog-database/dialog")) {
        var dialog = new Dialog() {
          id = node["id"].InnerText.ToString(),
          npc = node["npc"].InnerText.ToString()
        };
        if (!result.dialogs.TrueForAll(d => d.id != dialog.id)) {
          throw new Exception("Repeat dialog id: " + dialog.id);
        }
        if (!File.Exists(dialog.npc + ".png")) {
          throw new Exception(string.Format("Dialog NPC Not found: {0}.png", dialog.npc));
        } else using(var bmp = new Bitmap(dialog.npc+".png")) {
          if (!(bmp.Width > 0 && bmp.Height > 0 && bmp.Width % 32 == 0 && bmp.Height % 32 == 0)) {
            throw new Exception(string.Format("Dialog NPC Dimensions not 32px: {0}.png", dialog.npc));
          }
        }
        foreach(XmlNode child in node.SelectNodes("text")) {
          var text = new Text() {
            text = child.InnerText.ToString(),
            image = child["image"].InnerText.ToString()
          };
          if (!File.Exists(text.image + ".png")) {
            throw new Exception(string.Format("Dialog Image Not Found: {0}.png", text.image));
          } else using(var bmp = new Bitmap(text.image + ".png")) {
            if (bmp.Width != 128 || bmp.Height != 128) {
              throw new Exception(string.Format("Dialog Image Not Found: {0}.png", text.image));
            }
          }
          dialog.texts.Add(text);
        }
        result.dialogs.Add(dialog);
      }
      return result;
    }


  }
}

