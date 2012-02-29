#pragma once

#include "sifteo.h"
#include <stddef.h>

namespace TotalsGame
{

  class TiltFlowItem
  {
      const char *name;
      Vec2 sourcePosition;
      int imageIndex;
      const PinnedAssetImage **images;
      const PinnedAssetImage *singleImage;
      int numImages;
      int opt;

  public:
    static const int Passive = -2357;   //arbitrary value

    int id;
//TODO    public readonly Color color;
    const char *GetName();
    const char *description;
    const AssetImage *GetImages();
    int GetNumImages();
    Vec2 GetSourcePosition();

    TiltFlowItem (/*Color color TODO */);

    TiltFlowItem(const PinnedAssetImage *image);

    void IncrementImageIndex();

    TiltFlowItem(const PinnedAssetImage **_images, int _numImages);
    int GetOpt();
    void SetOpt(int val);

    const PinnedAssetImage *GetImage();

    bool IsToggle();

    bool HasImage();

    //for placement new
    void* operator new (size_t size, void* ptr) throw() {return ptr;}
    void operator delete(void *ptr) {}
  };
}

