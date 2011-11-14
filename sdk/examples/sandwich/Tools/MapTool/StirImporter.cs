using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;

namespace MapTool {

  public static class StirImporter {
    public static StirData LoadFromCpp(string basePath) {
      if (!File.Exists(basePath+".h") || !File.Exists(basePath+".cpp")) {
        Console.WriteLine("Could not location h/cpp source for path: " + basePath);
        return null;
      }
      var result = new StirData();
      // first scan the header for dictionary keys
      using(var hdr = new StreamReader(basePath+".h")) {
        var assetImageRx = new Regex(@"extern const Sifteo::AssetImage (\w*);");
        var pinnedAssetImageRx = new Regex(@"extern const Sifteo::PinnedAssetImage (\w*);");
        var line = string.Empty;
        while((line=hdr.ReadLine()) != null) {
          if (assetImageRx.IsMatch(line)) {
            // found an AssetImage Declaration
            var name = assetImageRx.Match(line).Groups[1].Value;
            var assetImage = new UnpinnedAssetImage() { name = name };
            result.unpinnedAssetImages.Add(name, assetImage);
            Console.WriteLine("AssetImage      \t" + name);
          } else if (pinnedAssetImageRx.IsMatch(line)) {
            // found a PinnedAssetImage Declaration
            var name = pinnedAssetImageRx.Match(line).Groups[1].Value;
            var pinnedAssetImage = new PinnedAssetImage() { name = name };
            result.pinnedAssetImages.Add(name, pinnedAssetImage);
            Console.WriteLine("PinnedAssetImage\t" + name);
          }
        }
      }

      // now scan the source file for dictionary values
      using (var src = new StreamReader(basePath+".cpp")) {
        var assetImageRx = new Regex(@"Sifteo::AssetImage (\w*) = {");
        var pinnedAssetImageRx = new Regex(@"Sifteo::PinnedAssetImage (\w*) = {");
        var tilesRx = new Regex(@"static const uint16_t (\w*)_tiles");
        var digitRx = new Regex(@"\d+");
        var line = string.Empty;
        var scratchpad = new List<ushort>();
        while((line=src.ReadLine()) != null) {
          while((line=src.ReadLine())!=null) {
            if (assetImageRx.IsMatch(line)) {
              // found a AssetImage Value
              var name = assetImageRx.Match(line).Groups[1].Value;
              var assetImage = result.unpinnedAssetImages[name];
              assetImage.width = int.Parse(digitRx.Match(src.ReadLine()).Value);
              assetImage.height = int.Parse(digitRx.Match(src.ReadLine()).Value);
              assetImage.frames = int.Parse(digitRx.Match(src.ReadLine()).Value);
            } else if (pinnedAssetImageRx.IsMatch(line)) {
              // found a PinnedAssetImage Value
              var name = pinnedAssetImageRx.Match(line).Groups[1].Value;
              var pinnedAssetImage = result.pinnedAssetImages[name];
              pinnedAssetImage.width = int.Parse(digitRx.Match(src.ReadLine()).Value);
              pinnedAssetImage.height = int.Parse(digitRx.Match(src.ReadLine()).Value);
              pinnedAssetImage.frames = int.Parse(digitRx.Match(src.ReadLine()).Value);
              pinnedAssetImage.firstTile = ushort.Parse(digitRx.Match(src.ReadLine()).Value);
            } else if (tilesRx.IsMatch(line)) {
              // found a AssetImage Tile Buffer
              var name = tilesRx.Match(line).Groups[1].Value;
              var assetImage = result.unpinnedAssetImages[name];
              while(!(line=src.ReadLine()).Contains(";")) {
                if (line.Contains("0x")) {
                  var tokens = line.Trim().Split(new char[] {','}, StringSplitOptions.RemoveEmptyEntries);
                  foreach(string token in tokens) {
                    scratchpad.Add(Convert.ToUInt16(token.Substring(2), 16));
                  }
                }
              }
              assetImage.tiles = scratchpad.ToArray();
              scratchpad.Clear();
            }
          }
        }
      }
      return result;
    }
  }
}

