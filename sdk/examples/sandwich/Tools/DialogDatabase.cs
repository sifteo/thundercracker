using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Xml;

namespace MapTool {

  public class DialogDatabase {

    //-------------------------------------------------------------------------
    // DATA
    //-------------------------------------------------------------------------

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

    //-------------------------------------------------------------------------
    // QUERYING
    //-------------------------------------------------------------------------

    public IEnumerable<string> ListNpcImagePaths() {
      // list unique npc images w/o repeats
      var hash = new HashSet<string>();
      foreach(var dialog in dialogs) {
        if (!hash.Contains(dialog.npc)) {
          yield return dialog.npc + ".png";
          hash.Add(dialog.npc);
        }
      }
    }

    public IEnumerable<string> ListDetailImagePaths() {
      // list unique detail images w/o repeats
      var hash = new HashSet<string>();
      foreach(var dialog in dialogs)
      foreach(var text in dialog.texts) {
        if (!hash.Contains(text.image)) {
          yield return text.image + ".png";
          hash.Add(text.image);
        }
      }
    }

    //-------------------------------------------------------------------------
    // LOADING
    //-------------------------------------------------------------------------

    public static DialogDatabase LoadFromXML() {

      var result = new DialogDatabase();
      var xml = new XmlDocument();
      xml.Load("dialog-database.xml");
      foreach(XmlNode node in xml.SelectNodes("dialog-database/dialog")) {
        var dialog = new Dialog() {
          id = node.Attributes["id"].InnerText.ToString(),
          npc = node.Attributes["npc"].InnerText.ToString()
        };
        if (!result.dialogs.TrueForAll(d => d.id != dialog.id)) {
          throw new Exception("Repeat dialog id: " + dialog.id);
        }
        foreach(XmlNode child in node.SelectNodes("text")) {
          dialog.texts.Add(new Text() {
            text = child.InnerText.ToString(),
            image = child.Attributes["image"].InnerText.ToString()
          });
        }
        result.dialogs.Add(dialog);
      }

      // validate images
      foreach(string path in result.ListNpcImagePaths()) {
        if (!File.Exists(path)) {
          throw new Exception("Dialog NPC Not found: " + path);
        } else using (var bmp = new Bitmap(path)) {
          if (bmp.Width == 0 || bmp.Width % 32 != 0 || bmp.Height == 0 || bmp.Height % 32 != 0) {
            throw new Exception("Dialog NPC Dimensions not 32px: " + path);
          }
        }
      }
      foreach(string path in result.ListDetailImagePaths()) {
          if (!File.Exists(path)) {
            throw new Exception("Dialog Detail Image Not Found: " + path);
          } else using(var bmp = new Bitmap(path)) {
            if (bmp.Width != 128 || bmp.Height != 128) {
              throw new Exception("Dialog Detail Image Size Invalid: " + path);
            }
          }
      }

      return result;
    }


  }
}

