/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * STIR -- Sifteo Tiled Image Reducer
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "imagestack.h"

namespace Stir {


ImageStack::ImageStack()
    : mConsistent(true), mWidth(0), mHeight(0), mFrames(0)
    {}

ImageStack::~ImageStack()
{
    for (std::vector<source*>::iterator i = sources.begin(); i != sources.end(); i++)
	delete *i;
}

void ImageStack::setWidth(int width)
{
    if (width > 0 && (mWidth == 0 || mWidth == width))
	mWidth = width;
    else
	mConsistent = false;
}

void ImageStack::setHeight(int height)
{
    if (height > 0 && (mHeight == 0 || mHeight == height))
	mHeight = height;
    else
	mConsistent = false;
}

void ImageStack::setFrames(int frames)
{
    if (frames > 0 && (mFrames == 0 || mFrames == frames))
	mFrames = frames;
    else
	mConsistent = false;
}

bool ImageStack::load(const char *filename)
{
   std::vector<uint8_t> png;
   LodePNG::loadFile(png, filename);
   if (png.empty())
       return false;

   source *s = new source();

   s->decoder.decode(s->rgba, png);
   if (s->rgba.empty() || s->decoder.getWidth() == 0 || s->decoder.getHeight() == 0) {
       delete s;
       return false;
   }

   sources.push_back(s);
   return true;
}

void ImageStack::finishLoading()
{
    /*
     * After all images that will be loaded have been loaded, this
     * function finalizes the geometry for this ImageStack. If there
     * was a problem, isConsistent() will return false.
     *
     * If a width/height have been explicitly specified, we'll assume
     * that each input image is a grid of frames. The sources can be
     * any size, as long as they're an exact multiple of the specified
     * width/height.
     *
     * If not, we'll set the width and height ourselves based on the
     * first source. If subsequent sources are different sizes, they
     * still follow the same rules as above. I.e, the second source
     * may be a multiple of the first source's size.
     *
     * Normally we will automatically determine the frame count. If a
     * frame count has been manually specified, it will just act as a
     * consistency check against our calculated frame count.
     */

    int frames = 0;

    if (sources.size() < 1)
	return;

    if (!mWidth)
	setWidth(sources[0]->decoder.getWidth());
    if (!mHeight)
	setHeight(sources[0]->decoder.getHeight());

    for (std::vector<source*>::iterator i = sources.begin(); i != sources.end(); i++) {
	unsigned width = (*i)->decoder.getWidth();
	unsigned height = (*i)->decoder.getHeight();

	if ((width % mWidth) || (height % mHeight)) {
	    mConsistent = false;
	    continue;
	}

	frames += (width / mWidth) + (height / mHeight);
    }
    
    setFrames(frames);
}


};  // namespace Stir
