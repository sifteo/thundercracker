#!/usr/bin/env python
#
# Utility to generate a lookup table for the audio mixer's soft limiter.
# The input to this table is a peak signal amplitude, and the output
# is an attenuator gain in 16-bit fixed point. The mixer will linearly
# interpolate between the samples in this lookup table.
#
# For a hard limiter, we would use a gain of 1 for values below the
# threshold, and a gain of (threshold / amplitude) for values above.
# This is the basis for the limiting function we use, but to give the
# limiter a soft edge we smooth this function with a gaussian kernel.
#
# Input parameters:
#
#   Steps - Size of the lookup table. Larger values use more ROM space
#           at the expense of a smoother curve.
#
#   Max Peak - The highest peak amplitude represented by the table.
#              If the audio signal is higher than this, the mixer will
#              extrapolate rather than interpolating.
#
#   Threshold - The center of the 'knee' in the limiter's gain curve.
#               Note that, due to the smoothing, the limiter will start
#               affecting the gain before we actually reach this threshold,
#               but this is where the gain curve will have an inflection
#               point.
#
#               This can generally be as high as possible without causing
#               the softened limit * the peak to exceed 1.0. There's an
#               assertion to check this condition, so decrease the threshold
#               if you hit this assert.
#
#   Softness - This controls the width of the gaussian kernel. Higher
#              values will give the limiter a smoother edge, at the expense
#              of less linear dynamic range. Smaller values will make the
#              limiter "transparent" at lower volumes.
#
# This will generate two files, a CSV file for visualizing the results
# as well as a C++ header for the actual lookup table.
#

STEPS = 256         # Power of two, integer
MAX_PEAK = 8        # Power of two, integer
THRESHOLD = 0.95
SOFTNESS = 0.1

#############################################################################

import math

def peakForStep(i):
    return i * MAX_PEAK / float(STEPS)

def hardLimitForPeak(p):
    if p < THRESHOLD:
        return 1.0
    else:
        return float(THRESHOLD) / p

def sampledGaussian(x, sigma):
    # Discrete gaussian would be more correct, but this produces
    # nearly identical results and it's much easier to calculate.
    return math.exp(-x*x / (2 * sigma * sigma)) / (sigma * math.sqrt(2 * math.pi))

def soften(p, f, resolution=16):
    # Convolve f() with sampledGaussian().
    # Uses 'resolution' integration steps per table step.
    # 
    # Integrates over 200% of our table's range, to keep the
    # beginning and end of the table smooth.

    s = 0
    for i in range(resolution * 2 * STEPS):
        step = i / float(resolution) - (STEPS / 2)
        peak = peakForStep(step)
        g = sampledGaussian(peak - p, SOFTNESS)
        s += g * f(peak)
    s *= 8.0 / (resolution * STEPS)
    return s

# Check the gain on our smoothing function
assert abs(soften(peakForStep(0), lambda x: 1.0) - 1.0) < 1e-6
assert abs(soften(peakForStep(STEPS/2), lambda x: 1.0) - 1.0) < 1e-6
assert abs(soften(peakForStep(STEPS-1), lambda x: 1.0) - 1.0) < 1e-6

header = open("../common/audiolimit-table.def", "w")
csv = open("../common/audiolimit-table.csv", "w")

header.write("static const unsigned AudioLimitSteps = %d;\n" % STEPS);
header.write("static const int AudioLimitMaxPeak = %d;\n" % MAX_PEAK);

header.write("static const uint16_t AudioLimitTable[] = {\n")
csv.write("Step, Peak, Hard Limit, Soft Limit, Table Value\n")

# Output a smoothed gain table
for s in range(STEPS):
    # Progress
    print "%d / %d" % (s+1, STEPS)

    peak = peakForStep(s)
    hardLimit = hardLimitForPeak(peak)
    softLimit = soften(peakForStep(s), hardLimitForPeak)

    # Make sure we aren't clipping
    assert softLimit * peak < 1.0
    assert softLimit < 1.0

    # Encoded as a 16-bit fixed point value - 1.
    encoded = int(softLimit * 0x10000 + 0.5) - 1
    assert encoded >= 0x0000
    assert encoded <= 0xFFFF

    header.write("    /* Step 0x%04x, peak = %6.4f, limit = %6.4f */ %d,\n" %
        (s, peak, softLimit, encoded))

    csv.write("%d, %f, %f, %f, %d\n" % (s, peak, hardLimit, softLimit, encoded))

header.write("};\n");
header.close()
csv.close()
