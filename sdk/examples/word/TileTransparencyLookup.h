#ifndef TILETRANSPARENCYLOOKUP_H
#define TILETRANSPARENCYLOOKUP_H

enum TransparencyType {
    TransparencyType_All = 0,
    TransparencyType_None = 1,
    TransparencyType_Some = 2,
    NumTransparencyTypes
};

enum ImageIndex {
    ImageIndex_Teeth,
    ImageIndex_Neighbored,
    ImageIndex_Connected,
    ImageIndex_ConnectedWord,

    NumImageIndexes

};

TransparencyType getTransparencyType(ImageIndex imageIndex,
                                     unsigned frame,
                                     unsigned tileX,
                                     unsigned tileY);

#endif // TILETRANSPARENCYLOOKUP_H
