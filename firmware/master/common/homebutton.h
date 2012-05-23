/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef HOMEBUTTON_H_
#define HOMEBUTTON_H_

namespace HomeButton
{
    void init();
    void onChange();
    bool isPressed();

    void task(void *p);

} // namespace Button

#endif // HOMEBUTTON_H_
