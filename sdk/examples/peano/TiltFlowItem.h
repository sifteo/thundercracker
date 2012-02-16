#pragma once

namespace TotalsGame
{

  class TiltFlowItem
  {
      const char *name;
      Vec2 sourcePosition;
      int imageIndex;
      const AssetImage *images;
      int numImages;
      int opt;

  public:
    //TODO public readonly static object Passive = new object();

    const void *userData;
//TODO    public readonly Color color;
    const char *GetName();
    const char *description;
    const AssetImage *GetImages();
    int GetNumImages();
    Vec2 GetSourcePosition();

    TiltFlowItem (/*Color color TODO */)
    {
      // TODO this.color = color;
      name = "";
      description = "";
      images = NULL;
      imageIndex = 0;
    }

    TiltFlowItem(const AssetImage *image)
    {
      images = image;
        numImages = 1;
      name = "";
      description = " ";
      //TODO this.color = Color.Mask;
        imageIndex = 0;
    }

    TiltFlowItem(const AssetImage *_images, int _numImages)
    {
      images = _images;
      numImages = _numImages;
      name = "";
      description = " ";
      //TODO this.color = Color.Mask;
      imageIndex = 0;
    }

    int GetOpt() {return opt;}
    void SetOpt(int val) { imageIndex = val % numImages; }

    const AssetImage *GetImage() {return images+imageIndex;}

    bool IsToggle() { return images != NULL && numImages > 1; }

    bool HasImage() { return images != NULL;  }

    //for placement new
    void* operator new (size_t size, void* ptr) throw() {return ptr;}
  };
}

