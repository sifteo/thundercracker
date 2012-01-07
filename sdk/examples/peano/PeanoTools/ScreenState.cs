using System;
using System.Collections.Generic;

namespace PeanoTools {
  public struct ScreenState {
    public BitSet top;
    public BitSet left;
    public BitSet bottom;
    public BitSet right;

    public BitSet Union { get { return top | left | bottom | right; } }
    public unsafe BitSet this[int i] { get {
        fixed(BitSet* p = &top) {
          return p[i];
        }
      }
    }

    public bool IsValid {
      get {
        // connection counts are valid
        for(int i=0; i<5; ++i) {
          int count = 0;
          for(int side=0; side<4; ++side) {
            if (this[side].Contains(i)) {
              count++;
            }
          }
          if (count > i+1) {
            return false;
          }
        }

        // connection envelopes are valid
        var union =  Union;
        for(int side=0; side<4; ++side) {
          for(int i=0; i<5; ++i) {
            if (this[i].Contains(i)) {
              for(int j=i+1; j<5; ++j) {
                if(union.Contains(j) && !this[i].Contains(j)) {
                  return false;
                }
              }
            }
          }
        }

        return true;
      }
    }

    public static IEnumerable<BitSet> AllValid {
      get {
        ScreenState ss = new ScreenState();
        for(ss.top.mask=0; ss.top.mask<32; ++ss.top.mask) {
          for(ss.left.mask=0; ss.left.mask<32; ++ss.left.mask) {
            for(ss.left.mask=0; ss.left.mask<32; ++ss.left.mask) {
              for(ss.left.mask=0; ss.left.mask<32; ++ss.left.mask) {

              }
            }
          }
        }
      }
    }
  }
}

