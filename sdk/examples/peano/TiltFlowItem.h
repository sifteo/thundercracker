#pragma once

#include "sifteo.h"
#include <stddef.h>

using namespace Sifteo;

namespace TotalsGame
{

  class TiltFlowItem
  {
      const char *name;
      Int2 sourcePosition;
      int imageIndex;
      const Sifteo::AssetImage **images;
      const Sifteo::AssetImage *singleImage;
      int numImages;
      int opt;

  public:
    static const int Passive = -2357;   //arbitrary value

    int id;
    const char *GetName();
    const char *description;
    const Sifteo::AssetImage *GetImages();
    int GetNumImages();
    Int2 GetSourcePosition();

    TiltFlowItem ();

    TiltFlowItem(const Sifteo::AssetImage *image);

    void IncrementImageIndex();

    TiltFlowItem(const Sifteo::AssetImage **_images, int _numImages);
    int GetOpt();
    void SetOpt(int val);

    void SetImage(const Sifteo::AssetImage *image);
    void SetImages(const Sifteo::AssetImage **_images, int _numImages);
    const Sifteo::AssetImage *GetImage();

    bool IsToggle();

    bool HasImage();
  };
}

