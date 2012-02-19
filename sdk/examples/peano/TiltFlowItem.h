#pragma once

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

    const void *userData;
//TODO    public readonly Color color;
    const char *GetName();
    const char *description;
    const AssetImage *GetImages();
    int GetNumImages();
    Vec2 GetSourcePosition() {return sourcePosition;}

    TiltFlowItem (/*Color color TODO */)
    {
      // TODO this.color = color;
      name = "";
      description = "";
      images = NULL;
      imageIndex = 0;
      userData = NULL;
    }

    TiltFlowItem(const AssetImage *image)
    {
      singleImage = image;
      images = &singleImage;
        numImages = 1;
      name = "";
      description = " ";
      //TODO this.color = Color.Mask;
        imageIndex = 0;
        userData = NULL;
    }

    void IncrementImageIndex() {imageIndex = (imageIndex+1)%numImages;}

    TiltFlowItem(const AssetImage **_images, int _numImages)
    {
      images = _images;
      numImages = _numImages;
      name = "";
      description = " ";
      //TODO this.color = Color.Mask;
      imageIndex = 0;
      userData = NULL;
    }

    int GetOpt() {return opt;}
    void SetOpt(int val) { imageIndex = val % numImages; }

    const AssetImage *GetImage() {return images[imageIndex];}

    bool IsToggle() { return images != NULL && numImages > 1; }

    bool HasImage() { return images != NULL;  }

    //for placement new
    void* operator new (size_t size, void* ptr) throw() {return ptr;}
    void operator delete(void *ptr) {}
  };
}

