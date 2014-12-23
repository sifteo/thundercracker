/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * STIR -- Sifteo Tiled Image Reducer
 * Micah Elizabeth Scott <micah@misc.name>
 *
 * Copyright <c> 2011 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef _IMAGESTACK_H
#define _IMAGESTACK_H

#include <stdint.h>
#include <vector>

#include "tile.h"
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

    void storeFrame(unsigned frame, TileGrid &tg, const TileOptions &opt);

    unsigned getWidth() const {
        return mWidth;
    }

    unsigned getHeight() const {
        return mHeight;
    }

    unsigned getFrames() const {
        return mFrames;
    }

    bool isConsistent() const {
        return mConsistent;
    }

    bool divisibleBy(unsigned size) const {
        return !(getWidth() % size) && !(getHeight() % size);
    }

 private:
    struct source {
        LodePNG::Decoder decoder;
        std::vector<uint8_t> rgba;
        unsigned firstFrame;
        unsigned numFrames;
    };

    bool mConsistent;
    unsigned mWidth;
    unsigned mHeight;
    unsigned mFrames;

    std::vector<source*> sources;

    source *getSourceForFrame(unsigned frame);
};


};  // namespace Stir

#endif
