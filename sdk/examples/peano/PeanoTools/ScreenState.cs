using System;

namespace PeanoTools {
  public struct ScreenState {
    public BitSet top;
    public BitSet left;
    public BitSet bottom;
    public BitSet right;

    public BitSet Union { get { return top | left | bottom | right; } }
  }
}

