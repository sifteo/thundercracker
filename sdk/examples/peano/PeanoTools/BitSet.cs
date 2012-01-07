using System;
using System.Collections.Generic;

namespace PeanoTools {

  public struct BitSet {

    public int mask;

    public BitSet(int mask) { this.mask = mask; }

    public static implicit operator int(BitSet bs) { return bs.mask; }
    public static implicit operator BitSet(int b) { return new BitSet(b); }
    public static BitSet Empty { get { return 0; } }

    public static BitSet operator&(BitSet b0, BitSet b1) { return b0.mask & b1.mask; }
    public static BitSet operator|(BitSet b0, BitSet b1) { return b0.mask | b1.mask; }
    public static BitSet operator^(BitSet b0, BitSet b1) { return b0.mask ^ b1.mask; }

    public bool Contains(int n) { return (mask & (1<<n)) != 0; }

    public IEnumerable<BitSet> AllCombinations { get { for(int i=0; i<32; ++i) { yield return new BitSet(i); } } }

  }

}

