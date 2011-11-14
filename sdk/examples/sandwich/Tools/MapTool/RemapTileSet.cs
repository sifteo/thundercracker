using System;
using System.Text;
using System.Xml;

namespace MapTool {
  public static class RemapTileSet {

    delegate int IntMap(int oldIndex);

    public static void Fix(string path) {
      var xml = new XmlDocument();
      xml.Load(path);

      XmlNode imageNode = xml.SelectSingleNode("/map/tileset/image");
      int imageWidth = int.Parse(imageNode.Attributes["width"].Value);
      //int imageHeight = int.Parse(imageNode.Attributes["height"].Value);

      int tileWidth = imageWidth / 16;
      int boundary = (imageWidth - 64) / 16;

      IntMap toNewIndex = oldIndex => {
        int oldX = oldIndex % tileWidth;
        int oldY = oldIndex / tileWidth;
        int newX = oldX >= boundary ? oldX-boundary : oldX + 4;
        return newX + oldY * tileWidth;
      };

      foreach(XmlNode tile in xml.SelectNodes("/map/tileset/tile")) {
        tile.Attributes["id"].Value = toNewIndex(
          int.Parse(tile.Attributes["id"].Value)
        ).ToString();
      }
      foreach(XmlNode data in xml.SelectNodes("/map/layer/data")) {
        var innerText = data.InnerText;
        var sb = new StringBuilder();
        sb.Append("\n");
        foreach(var line in innerText.Split(new char[] { '\n' }, StringSplitOptions.RemoveEmptyEntries)) {
          var tokens = line.Split(new char[] { ',' }, StringSplitOptions.RemoveEmptyEntries);
          for(int i=0; i<tokens.Length; ++i) {
            sb.Append(toNewIndex(int.Parse(tokens[i])-1)+1);
            sb.Append(",");
          }
          sb.Append("\n");
        }
        data.InnerText = sb.ToString();
      }

      Console.WriteLine(xml.InnerXml);

      xml.Save(path);
    }

  }
}

