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

    public int Count {
      get {
        int result = 0;
        for(int i=0; i<5; ++i) {
          if ((mask & (1<<i)) != 0) {
            result++;
          }
        }
        return result;
      }
    }
    public int HigestBit {
      get {
        for(int i=4; i>=0; --i) {
          if ((mask & (1<<i)) != 0) {
            return i;
          }
        }
        return 0;
      }
    }

    public int LowestBit {
      get {
        for(int i=0; i<5; ++i) {
          if ((mask & (1<<i)) != 0) {
            return i;
          }
        }
        return -1;
      }
    }
  
    public BitSet WithoutLowestBit {
      get {
        int lowestBit = LowestBit;
        if (lowestBit == -1) {
          return BitSet.Empty;
        } else {
          return mask & ~(1<<lowestBit);
        }
      }
    }
  }

}

