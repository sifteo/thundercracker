using System;
using System.Collections.Generic;
using System.Linq;
using System.Drawing;
using System.IO;

namespace PNGtoMask {
  class Program {

    enum OutputBits {
      AllTransparent = 0,
      AllOpaque = 1,
      SomeTransparent = 2,
    }

    static void Main(string[] args) {
      // Open a Stream and decode a PNG image
      string inputFileName = @"..\\..\\..\\wc_transition.png";
      List<byte> outputList = new List<byte>();
      using (Bitmap image = new Bitmap(inputFileName, false)) {

        int numTiles = 16; // assumes 128x128 full screen image
        int frameSize = 128;
        const int TILE_SIZE = 8;
        byte output = 0;
        int outputShift = 0;
        int outputTileBits = 2; // bits
        int maxOutputShift = 8 - outputTileBits;

        // Loop through the images pixels to reset color.
        for (int frame = 0; frame * frameSize < image.Width; ++frame) {
          for (int tileY = 0; tileY < numTiles; ++tileY) {
            for (int tileX = 0; tileX < numTiles; ++tileX) {

              bool alpha = false;
              bool opaque = false;
              int startX = frame * frameSize + tileX * TILE_SIZE;
              for (int x = startX; x < startX + TILE_SIZE; ++x) {

                int startY = tileY * TILE_SIZE;
                for (int y = startY; y < startY + TILE_SIZE; ++y) {
                  Color pixelColor = image.GetPixel(x, y);
                  switch (pixelColor.A) {
                    case 0xff:
                      opaque = true;
                      break;

                    default:
                      alpha = true;
                      break;
                  }
                }
              }
              OutputBits bits;
              if (alpha && !opaque)
              {
                bits = OutputBits.AllTransparent;
              }
              else if (!alpha && opaque) {
                bits = OutputBits.AllOpaque;
              }
              else {
                bits = OutputBits.SomeTransparent; // alpha && opqaue or !alpha && !opaque
              }
              //Console.Out.WriteLine("frame {0}, tile ({1}, {2}), {3}", frame, tileX, tileY, bits.ToString());
              output |= (byte) ((int)bits << outputShift);
              outputShift += outputTileBits;
              if (outputShift > maxOutputShift) {
                // save byte
                outputList.Add(output);
                //Console.Out.WriteLine("byte 0x{0}", output.ToString("x"));
                outputShift = 0;
                output = 0;
              }
            }
          }
        }
      }

      // Example #3: Write only some strings in an array to a file.
      using (System.IO.StreamWriter file = 
              new System.IO.StreamWriter(inputFileName + ".cpp")) {
        file.WriteLine("const static char imageMask[] =");
        file.WriteLine("{");
        file.Write("\t");
        for (int i = 0; i < outputList.Count; ++i) {
            file.Write("0x" + outputList[i].ToString("x"));
            file.Write(", ");
            if ((i % 4) == 3) {
              file.WriteLine();
              file.Write("\t");
            }
        }
        file.WriteLine("};");
      }

      Console.Out.WriteLine(outputList.ToString());


    }
  }
}
