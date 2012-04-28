
# Routines common to multiple test modules

def rangeHelper(value, min1, max1, min2, max2):
    range1 = (max1 - min1)
    range2 = (max2 - min2)
    return (((value - min1) * range2) / range1) + min2
