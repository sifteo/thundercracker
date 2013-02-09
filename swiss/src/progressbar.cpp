#include "progressbar.h"
#include <stdio.h>
#include <stdint.h>


void ProgressBar::begin()
{
    redraw();
}

void ProgressBar::end()
{
    fprintf(stdout, "\n");
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
    fprintf(stdout, "\r[");

    for (unsigned i = 0; i < width; ++i)
        fprintf(stdout, "%c", " #"[i < quantized]);

    fprintf(stdout, "]");
}
