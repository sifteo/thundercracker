using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using Typocalypse.Trie;

namespace TrieMaker {
  class Program {
    static void Main(string[] args) {

      //Create trie
      Trie < string > trie = new Trie < string > ();

      // Read each line of the file into a string array. Each element
      // of the array is one line of the file.
      string[] lines = System.IO.File.ReadAllLines(@"..\..\dictionary.txt");

      // Display the file contents by using a foreach loop.
      //System.Console.WriteLine("Contents of WriteLines2.txt = ");
      foreach (string line in lines) 
      {
        // Use a tab to indent each line of the file.
        trie.Put(line.Trim(), "1");
        //Console.WriteLine("\t" + line);
      }

      // Read each line of the file into a string array. Each element
      // of the array is one line of the file.
      lines = System.IO.File.ReadAllLines(@"..\..\badwords.txt");

      // Display the file contents by using a foreach loop.
      //System.Console.WriteLine("Contents of WriteLines2.txt = ");
      foreach (string line in lines) 
      {
        // Use a tab to indent each line of the file.
        trie.Remove(line.Trim());
        //Console.WriteLine("\t" + line);
      }

      trie.WriteToCppFile();
    }
  }
}
