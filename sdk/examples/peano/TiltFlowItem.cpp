#include "TiltFlowItem.h"
namespace TotalsGame
{

    Vec2 TiltFlowItem::GetSourcePosition() {return sourcePosition;}

    TiltFlowItem::TiltFlowItem (/*Color color TODO */)
    {
      // TODO this.color = color;
      name = "";
      description = "";
      images = NULL;
      imageIndex = 0;
      id = 0;
    }

    TiltFlowItem::TiltFlowItem(const PinnedAssetImage *image)
    {
      singleImage = image;
      images = &singleImage;
        numImages = 1;
      name = "";
      description = " ";
      //TODO this.color = Color.Mask;
        imageIndex = 0;
        id = 0;
    }

    void TiltFlowItem::IncrementImageIndex() {imageIndex = (imageIndex+1)%numImages;}

    TiltFlowItem::TiltFlowItem(const PinnedAssetImage **_images, int _numImages)
    {
      images = _images;
      numImages = _numImages;
      name = "";
      description = " ";
      //TODO this.color = Color.Mask;
      imageIndex = 0;
      id = 0;
    }

    int TiltFlowItem::GetOpt() {return opt;}
    void TiltFlowItem::SetOpt(int val) { imageIndex = val % numImages; }

    const PinnedAssetImage *TiltFlowItem::GetImage() {return images[imageIndex];}

    bool TiltFlowItem::IsToggle() { return images != NULL && numImages > 1; }

    bool TiltFlowItem::HasImage() { return images != NULL;  }

}

