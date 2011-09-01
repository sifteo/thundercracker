/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * STIR -- Sifteo Tiled Image Reducer
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <float.h>
#include <algorithm>
#include "color.h"

CIELab CIELab::lut565[0x10000];


RGB565 RGB565::wiggle() const
{
    // Make a slight (1 LSB) modification to the low byte

    uint16_t rg = value & 0xFFE0;
    uint16_t b = value & 0x1F;

    if (b == 0x1F)
	b--;
    else
	b++;

    return (uint16_t)(rg | b);
}

CIELab::CIELab(uint32_t rgb)
{
    double red = (uint8_t)rgb;
    double green = (uint8_t)(rgb >> 8);
    double blue = (uint8_t)(rgb >> 16);

    double xyz[3] = { 0, 0, 0 };
    static const double m[3][3] = {
	{0.79767484649999998, 0.13519170820000001, 0.031353408800000003},
	{0.28804020250000001, 0.71187413249999998, 8.5665099999999994e-05},
	{0, 0, 0.82521143890000004}
    };
    for (int i = 0; i < 3; i++) {
	xyz[i] += m[i][0] * red;
	xyz[i] += m[i][1] * green;
	xyz[i] += m[i][2] * blue;
    }

    static const double d50_white[3] = {0.964220, 1, 0.825211}; 
    for (int i = 0; i < 3; i++) {
	xyz[i] = f_cbrt(xyz[i] / d50_white[i]);
    }

    L = 116.0 * xyz[1] - 16.0;
    a = 500.0 * (xyz[0] - xyz[1]);
    b = 200.0 * (xyz[1] - xyz[2]);
}

uint32_t CIELab::rgb() const
{
    // Convert L*a*b* to XYZ  
    double fy = (L+16.0)/116.0;
    double fx = fy + a/500.0;
    double fz = fy - b/200.0;
  
    static const double ep = 216.0 / 24389.0;
    static const double ka = 24389.0 / 27.0;
    static const double d50_white[3] = {0.964220, 1, 0.825211}; 
    
    double xyz[3];
    xyz[0] = 255.0 * d50_white[0] * (fx*fx*fx>ep?fx*fx*fx:(116.0*fx-16.0)/ka);  
    xyz[1] = 255.0 * d50_white[1] * (L>ka*ep?pow(fy,3.0):L/ka);  
    xyz[2] = 255.0 * d50_white[2] * (fz*fz*fz>ep?fz*fz*fz:(116.0*fz-16.0)/ka);  
  
    // Convert XYZ to RGB  
    double rgb[3] = { 0, 0, 0 };
    static const double m[3][3] = {
	{1.3459434662015495, -0.54459884249024337, -6.9388939039072284e-18},
	{-0.25560754075639425, 1.5081672430343562, 3.9573379295720912e-18},
	{-0.05111177218797338, 0.020535140503506362, 1.2118106376869808}
    };
    for (int i = 0; i < 3; i++) {
	rgb[0] += xyz[i] * m[i][0];  
	rgb[1] += xyz[i] * m[i][1];  
	rgb[2] += xyz[i] * m[i][2];  
    }  

    uint32_t rgb32 = 0;
    for (int i = 0; i < 3; i++) {
	// Round, not truncate
	double v = rgb[i] + 0.5;

	if (v < 0) v = 0;
	if (v > 255) v = 255;

	rgb32 |= (uint8_t)v << (i*8);
    }  

    return rgb32;
}  
    
double CIELab::f_cbrt(double r)
{ 
    static const double ep = 216.0 / 24389.0;
    static const double ka = 24389.0 / 27.0;
    r/=255.0;
    return r > ep ? pow(r,1/3.0) : (ka*r+16)/116.0;  
}  

double CIELab::meanSquaredError(CIELab other)
{
    // http://en.wikipedia.org/wiki/Mean_squared_error
    double err = 0;
    err += (L - other.L) * (L - other.L);
    err += (a - other.a) * (a - other.a);
    err += (b - other.b) * (b - other.b);
    return err;
}

void CIELab::initialize(void)
{
    // Initialize the global lookup table from RGB565 to CIE L*a*b*
    
    for (unsigned v = 0; v < 0x10000; v++)
	lut565[v] = CIELab(RGB565((uint16_t)v).rgb());
}

ColorReducer::ColorReducer()
{
    memset(colorWeights, 0, sizeof colorWeights);
}

void ColorReducer::reduce(double maxMSE)
{
    /*
     * This is a median-cut style color reducer. We start with a
     * single box that encloses all pixels in the image, and we
     * iteratively subdivide boxes until we reach the targetted MSE
     * metric.
     *
     * The method we pick to choose a box to subdivide, in this case,
     * is based on measuring the potential improvement to the total
     * MSE we would get by subdividing that box. We always pick the
     * box that gives us the most bang for our colortable buck.
     */

    fprintf(stderr, "Optimizing palette...\n");

    // Base case: One single color.
    box root = { 0, (unsigned)colors.size() };
    boxes.clear();
    boxes.push_back(root);
    boxQueue.clear();
    boxQueue.push_back(0);

    /*
     * Keep splitting until our error is low enough or we run out of boxes.
     *
     * It's actually much more expensive to calculate the MSE (and the
     * color LUT it requires) than it is to perform palette
     * splits.. so we'll try to do multiple splits at a time, speeding
     * up over time.
     */

    while (!boxQueue.empty()) {
	double mse = meanSquaredError();

	fprintf(stderr, "\r\t%d colors, MSE %g > %g    ",
		(int)boxes.size(), mse, maxMSE);

	if (mse <= maxMSE)
	    break;

	// Heuristic
	int numSplits = 1 + boxQueue.size() / 20;

	for (unsigned i = 0; i < numSplits && !boxQueue.empty(); i++) {
	    unsigned boxIndex = *boxQueue.begin();
	    struct box& b = boxes[boxIndex];
	    boxQueue.pop_front();

	    int major = CIELab::findMajorAxis(&colors[b.begin], b.end - b.begin);

	    std::sort(colors.begin() + b.begin,
		      colors.begin() + b.end,
		      CIELab::sortAxis(major));

	    splitBox(b);
	}

	updateInverseLUT();
    }

    fprintf(stderr, "\n");
}

void ColorReducer::updateInverseLUT()
{
    /*
     * Regenerate the lookup table which converts RGB565 colors to our
     * reduced palette, as used by nearest().
     *
     * XXX: This is really slow.
     */

    unsigned v;

    for (v = 0; v < 0x10000; v++) {
	box *b = NULL;
	double distance = DBL_MAX;
	CIELab reference(RGB565((uint16_t)v));

	for (std::vector<box>::iterator i = boxes.begin(); i != boxes.end(); i++) {
	    double err = reference.meanSquaredError(CIELab(boxMedian(*i)));
	    if (err < distance) {
		distance = err;
		b = &*i;
	    }
	}

	inverseLUT[v] = b - &boxes[0];
    }
}

double ColorReducer::meanSquaredError()
{
    /*
     * Measure the Mean Squared Error error experienced by every
     * original pixel when transformed into the current set of reduced
     * colors.
     */

    double error = 0;
    unsigned total = 0;
    unsigned v;

    for (v = 0; v < 0x10000; v++) {
	RGB565 color((uint16_t)v);
	unsigned weight = colorWeights[v];
	error += CIELab(nearest(color)).meanSquaredError(CIELab(color)) * weight;
	total += weight;
    }

    return error / total;
}

int CIELab::findMajorAxis(RGB565 *colors, size_t count)
{
    /*
     * Of all the colors given, find the component axis which occupies
     * the widest range of values. This is the axis we would get the
     * most benefit from splitting along.
     */

    double labMin[3] = { DBL_MAX, DBL_MAX, DBL_MAX };
    double labMax[3] = { DBL_MIN, DBL_MIN, DBL_MIN };
    for (unsigned i = 0; i < count; i++) {
	CIELab c = CIELab(colors[i]);

	for (int a = 0; a < 3; a++) {
	    labMin[a] = std::min(labMin[a], c.axis[a]);
	    labMax[a] = std::max(labMax[a], c.axis[a]);
	}
    }

    double diffs[3];
    for (int a = 0; a < 3; a++)
	diffs[a] = labMax[a] - labMin[a];

    double major = 0;
    double maxDiff = DBL_MIN;
    for (int a = 0; a < 3; a++) {
	if (diffs[a] > maxDiff) {
	    maxDiff = diffs[a];
	    major = a;
	}
    }

    return major;
}

bool ColorReducer::splitBox(box &b)
{
    /*
     * Split a box roughly in half. This function requires that the
     * split occurs on a boundry between two different RGB565 colors,
     * in order to avoid creating boxes that are smaller than one
     * 16-bit device color. So, we start at the actual median of the box,
     * and search outward for the nearest color boundary.
     *
     * If we never find one (the box is all one color), returns false.
     * On success, returns true and appends a new box to the box vector.
     */

    // Max number of steps: Half the width, rounded up.
    unsigned maxSteps = (b.end - b.begin + 1) >> 1;

    // The boundary we're looking for is between (middle +/- step) and (middle +/- step + 1).
    // Start with the middle index, rounded down.
    unsigned middle = (b.end + b.begin) >> 1;

    for (int step = 0; step < maxSteps; step++) {
	unsigned split = middle + step;
	if (split >= b.begin && split+1 < b.end && colors[split] != colors[split+1]) {
	    splitBox(b, split);
	    return true;
	}
	split = middle - step;
	if (split >= b.begin && split+1 < b.end && colors[split] != colors[split+1]) {
	    splitBox(b, split);
	    return true;
	}
    }

    return false;
}

void ColorReducer::splitBox(box &b, int at)
{
    /*
     * Split a box at the specified position, between index at and
     * at+1. Both new boxes are added to the box queue.
     */

    box newBox = { at+1, b.end };
    b.end = newBox.begin;

    boxQueue.push_back(&b - &boxes[0]);
    boxQueue.push_back(boxes.size());
    boxes.push_back(newBox);
}

