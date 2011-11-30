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
 
      //Add some key-value pairs to the trie
      trie.Put("James", "112");
      trie.Put("Jake", "222");
      trie.Put("Fred", "326");
      trie.WriteToCppFile();

      //Search the trie
      trie.Matcher.NextMatch('J'); //Prefix thus far: "J"
      trie.Matcher.GetPrefixMatches(); //[112, 222]
      trie.Matcher.IsExactMatch(); //false
      trie.Matcher.NextMatch('a');
      trie.Matcher.NextMatch('m'); //Prefix thus far: "Jam"
      trie.Matcher.GetPrefixMatches(); //[112]
      trie.Matcher.NextMatch('e');
      trie.Matcher.NextMatch('s'); //Prefix thus far: "James"
      trie.Matcher.IsExactMatch(); //true
      trie.Matcher.GetExactMatch(); //[112]
 
      //Remove a string-value pair
      trie.Remove("James");
    }
  }
}
