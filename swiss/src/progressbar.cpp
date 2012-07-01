#include "progressbar.h"
#include <stdio.h>
#include <stdint.h>


void ProgressBar::begin()
{
    redraw();
}

void ProgressBar::end()
{
    fprintf(stderr, "\n");
}

void ProgressBar::update(unsigned amount)
{
    unsigned next = (uint64_t(amount) * width) / totalAmount;
    if (next != quantized) {
        quantized = next;
        redraw();
    }
}

void ProgressBar::redraw()
{
    fprintf(stderr, "\r[");

    for (unsigned i = 0; i < width; ++i)
        fprintf(stderr, "%c", " #"[i < quantized]);

    fprintf(stderr, "]");
}
