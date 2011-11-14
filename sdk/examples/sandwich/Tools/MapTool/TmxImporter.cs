using System;
using System.Collections.Generic;
using System.IO;
using System.Xml;

namespace MapTool {

  public static class TmxImporter {

    public static TmxMap Load(string path) {

      string baseDir = Path.GetDirectoryName(path);

      if (!File.Exists(path)) {
        Console.WriteLine("TMX_LOAD_ERROR: Could Not File Tile Path: {0}", path);
        return null;
      }
      var dom = new XmlDocument();
      dom.Load(path);
      if (dom.SelectSingleNode("/map/layer/data").Attributes["encoding"].Value != "csv") {
        Console.WriteLine("TMX_LOAD_ERROR: Could not load non-CSV Tile Data");
        return null;
      }

      // load map props
      var map = dom.SelectSingleNode("/map");
      var result = new TmxMap() {
        name = Path.GetFileNameWithoutExtension(path),
        width = int.Parse(map.Attributes["width"].Value),
        height = int.Parse(map.Attributes["height"].Value),
        tilePixelWidth = int.Parse(map.Attributes["tilewidth"].Value),
        tilePixelHeight = int.Parse(map.Attributes["tileheight"].Value)
      };

      // load tile sets
      foreach(XmlNode node in dom.SelectNodes("/map/tileset")) {
        var tileSet = new TmxTileset() {
          map = result,
          name = node.Attributes["name"].Value,
          tilePixelWidth = int.Parse(node.Attributes["tilewidth"].Value),
          tilePixelHeight = int.Parse(node.Attributes["tileheight"].Value),
          firstGid = int.Parse(node.Attributes["firstgid"].Value)
        };
        var imgNode = node.SelectSingleNode("image");
        tileSet.image.path = Path.Combine(baseDir, imgNode.Attributes["source"].Value);
        if (!File.Exists(tileSet.image.path)) {
          Console.WriteLine("TMX_LOAD_ERROR: Could not find source image for tileset");
          return null;
        }
        tileSet.image.pixelWidth = int.Parse(imgNode.Attributes["width"].Value);
        tileSet.image.pixelHeight = int.Parse(imgNode.Attributes["height"].Value);
        tileSet.tiles = new TmxTile[
          tileSet.image.pixelWidth / tileSet.tilePixelWidth,
          tileSet.image.pixelHeight/tileSet.tilePixelHeight
        ];
        int tileId = tileSet.firstGid;
        for(int x=0; x<tileSet.TileCountX; ++x) {
          for(int y=0; y<tileSet.TileCountY; ++y) {
            tileSet.tiles[x,y] = new TmxTile() {
              tileSet = tileSet,
              gid = tileId,
              imageX = x,
              imageY = y
            };
            tileId++;
          }
        }
        foreach(XmlNode propNode in node.SelectNodes("tile/properties")) {
          var gid = tileSet.firstGid + int.Parse(propNode.ParentNode.Attributes["id"].Value);
          var tile = tileSet.GetTile(gid);
          foreach(XmlNode prop in propNode.ChildNodes) {
            tile.Add(prop.Attributes["name"].Value, prop.Attributes["value"].Value);
          }
        }

        result.tileSets.Add(tileSet.name, tileSet);
      }

      // load layers
      foreach(XmlNode node in dom.SelectNodes("/map/layer")) {
        var layer = new TmxLayer() {
          map = result,
          name = node.Attributes["name"].Value,
          tiles = new int[
            int.Parse(node.Attributes["width"].Value),
            int.Parse(node.Attributes["height"].Value)
          ]
        };
        if (node.Attributes["visible"] != null) {
          layer.visible = node.Attributes["visible"].Value != "0";
        }
        if (node.Attributes["opacity"] != null) {
          layer.opacity = float.Parse(node.Attributes["opacity"].Value);
        }
        int y = 0;
        foreach(var line in node.SelectSingleNode("data").InnerText.Split(new char[] {'\n' }, StringSplitOptions.RemoveEmptyEntries)) {
          int x = 0;
          foreach(var gid in line.Trim().Split(new char[] { ',' }, StringSplitOptions.RemoveEmptyEntries)) {
            layer.tiles[x,y] = int.Parse(gid);
            ++x;
          }
          ++y;
        }
        foreach(XmlNode propNode in node.SelectNodes("properties/property")) {
          layer.Add(propNode.Attributes["name"].Value, propNode.Attributes["value"].Value);
        }
        result.layers.Add(layer);
        result.nameToLayer.Add(layer.name, layer);
        if (layer.name == "background") {
          result.backgroundLayer = layer;
          result.backgroundTileSet = layer.GetTile(0,0).tileSet;
        }
      }

      // load objects
      foreach(XmlNode node in dom.SelectNodes("/map/objectgroup/object")) {
        var obj = new TmxObject() {
          name = node.Attributes["name"].Value,
          type = node.Attributes["type"].Value,
          pixelX = int.Parse(node.Attributes["x"].Value),
          pixelY = int.Parse(node.Attributes["y"].Value),
          pixelW = int.Parse(node.Attributes["width"].Value),
          pixelH = int.Parse(node.Attributes["height"].Value)
        };
        if (node.Attributes["gid"] != null) {
          obj.tile = result.GetTile(int.Parse(node.Attributes["gid"].Value));
        }
        foreach(XmlNode property in node.SelectNodes("properties/property")) {
          obj.Add(property.Attributes["name"].Value, property.Attributes["value"].Value);
        }
        result.objects.Add(obj);
      }
      return result;
    }

  }
}

