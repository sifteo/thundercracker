/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * STIR -- Sifteo Tiled Image Reducer
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _IMAGESTACK_H
#define _IMAGESTACK_H

#include <stdint.h>
#include <vector>

#include "lodepng.h"

namespace Stir {


/*
 * ImageStack --
 *
 *    A group of 2D images, interpreted as a sequence of frames. There
 *    need not be a one-to-one relationship between input images and
 *    frames: One image can be used as a strip or grid of frames, for
 *    example.
 */

class ImageStack {
 public:
    ImageStack();
    ~ImageStack();

    bool load(const char *filename);
    void finishLoading();

    void setWidth(int width);
    void setHeight(int height);
    void setFrames(int frames);

    int getWidth() const {
	return mWidth;
    }

    int getHeight() const {
	return mHeight;
    }

    int getFrames() const {
	return mFrames;
    }

    bool isConsistent() const {
	return mConsistent;
    }

    bool divisibleBy(int size) const {
	return !(getWidth() % size) && !(getHeight() % size);
    }

 private:
    struct source {
	LodePNG::Decoder decoder;
	std::vector<uint8_t> rgba;
    };

    bool mConsistent;
    int mWidth;
    int mHeight;
    int mFrames;

    std::vector<source*> sources;
};


};  // namespace Stir

#endif
