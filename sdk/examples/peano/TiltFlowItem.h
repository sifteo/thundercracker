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
      const AssetImage **images;
      const AssetImage *singleImage;
      int numImages;
      int opt;

  public:
    static const int Passive = -2357;   //arbitrary value

    int id;
    const char *GetName();
    const char *description;
    const AssetImage *GetImages();
    int GetNumImages();
    Vec2 GetSourcePosition();

    TiltFlowItem ();

    TiltFlowItem(const AssetImage *image);

    void IncrementImageIndex();

    TiltFlowItem(const AssetImage **_images, int _numImages);
    int GetOpt();
    void SetOpt(int val);

    const AssetImage *GetImage();

    bool IsToggle();

    bool HasImage();

    //for placement new
    void* operator new (size_t size, void* ptr) throw() {return ptr;}
    void operator delete(void *ptr) {}
  };
}

