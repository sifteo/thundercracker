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
    const char *GetName();
    const char *description;
    const PinnedAssetImage *GetImages();
    int GetNumImages();
    Vec2 GetSourcePosition();

    TiltFlowItem ();

    TiltFlowItem(const PinnedAssetImage *image);

    void IncrementImageIndex();

    TiltFlowItem(const PinnedAssetImage **_images, int _numImages);
    int GetOpt();
    void SetOpt(int val);

    void SetImage(const PinnedAssetImage *image);
    void SetImages(const PinnedAssetImage **_images, int _numImages);
    const PinnedAssetImage *GetImage();

    bool IsToggle();

    bool HasImage();
  };
}

