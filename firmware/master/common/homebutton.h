/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef BUTTON_H_
#define BUTTON_H_

namespace Button
{
    void init();
    void onChange();
    bool isPressed();

    void task(void *p);

} // namespace Button

#endif /* BUTTON_H_ */
