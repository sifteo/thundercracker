#ifndef TILETRANSPARENCYLOOKUP_H
#define TILETRANSPARENCYLOOKUP_H

enum TransparencyType {
    TransparencyType_All = 0,
    TransparencyType_None = 1,
    TransparencyType_Some = 2,
    NumTransparencyTypes
};

enum ImageIndex {
    ImageIndex_Connected,
    ImageIndex_ConnectedWord,
    ImageIndex_ConnectedLeft,
    ImageIndex_ConnectedLeftWord,
    ImageIndex_ConnectedRight,
    ImageIndex_ConnectedRightWord,
    ImageIndex_Neighbored,
    ImageIndex_Teeth,

    NumImageIndexes
};

TransparencyType getTransparencyType(ImageIndex imageIndex,
                                     unsigned frame,
                                     unsigned tileX,
                                     unsigned tileY);

#endif // TILETRANSPARENCYLOOKUP_H
