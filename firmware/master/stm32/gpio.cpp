/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
 *
 * Copyright <c> 2012 Sifteo, Inc.
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

#include "gpio.h"

void GPIOPin::setControl(Control c) const {
    setControl(port(), pin(), c);
}

void GPIOPin::irqInit() const
{
    /*
     * Use the External IRQ controller to make this pin an IRQ input.
     *
     * All pins with the same number share an EXTI controller. If you've
     * already used an external input on PA5, for example, PB5-PG5 are
     * not simultaneously usable for IRQ inputs.
     */

    unsigned portID = portIndex();
    unsigned pinID = pin();
    
    /*
     * Set EXTICRx. We have one nybble per EXTI controller, four nybbles
     * per word, and four words. This nybble is a multiplexor input
     * which selects a port ID for that EXTI controller to listen on.
     */

    volatile uint32_t *cr = &AFIO.EXTICR[pinID >> 2];
    unsigned crShift = (pinID & 3) << 2;
    *cr = (*cr & ~(0xF << crShift)) | (portID << crShift);
}
