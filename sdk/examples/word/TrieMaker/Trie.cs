using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Typocalypse.Trie
{
    /// <summary>
    /// Trie data structure which maps strings to generic values.
    /// </summary>
    /// <typeparam name="V">Type of values to map to. Must be referencable</typeparam>
    public class Trie<V> where V:class
    {

        private TrieNode<V> root;

        /// <summary>
        /// Matcher object for matching prefixes of strings to the strings stored in this trie.
        /// </summary>
        public IPrefixMatcher<V> Matcher { get; private set; }
        
        /// <summary>
        /// Create an empty trie with an empty root node.
        /// </summary>
        public Trie()
        {
            this.root = new TrieNode<V>(null, '\0');
            this.Matcher = new PrefixMatcher<V>(this.root);
        }

        /// <summary>
        /// Put a new key value pair, overwriting the existing value if the given key is already in use.
        /// </summary>
        /// <param name="key">Key to search for value by.</param>
        /// <param name="value">Value associated with key.</param>
        public void Put(string key, V value)
        {
            TrieNode<V> node = root;
            foreach (char c in key)
            {
                node = node.AddChild(c);
            }
            node.Value = value;
        }

        /// <summary>
        /// Remove the value that a key leads to and any redundant nodes which result from this action.
        /// Clears the current matching process.
        /// </summary>
        /// <param name="key">Key of the value to remove.</param>
        public void Remove(string key)
        {
            TrieNode<V> node = root;
            foreach (char c in key)
            {
                node = node.GetChild(c);
            }
            node.Value = null;

            //Remove all ancestor nodes which don't lead to a value.
            while (node != root && !node.IsTerminater() && node.NumChildren() == 0)
            {
                char prevKey = node.Key;
                node = node.Parent;
                node.RemoveChild(prevKey);
            }

            Matcher.ResetMatch();
        }

        public void WriteToCppFile() 
        {
          using (System.IO.StreamWriter file = new System.IO.StreamWriter(@"..\..\..\TrieNode.cpp")) 
          {
            file.WriteLine("#include \"TrieNode.h\"\n");
            this.root.WriteToCppFile(file, null);

          }
        }

    }
}