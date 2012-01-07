using System;
using System.IO;

namespace PeanoTools {
	 static partial class Program {

		static void Main (string[] args) {
      // kludge the base directory for relative-paths to the root of the game tree
      while(Path.GetFileName(Directory.GetCurrentDirectory()) != "peano") {
        Directory.SetCurrentDirectory("..");
      }

			//ComputeAssets();
		}

	}

}
