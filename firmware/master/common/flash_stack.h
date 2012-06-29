/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Definitions which affect our entire flash storage stack.
 */

#ifndef FLASH_STACK_H_
#define FLASH_STACK_H_


namespace FlashStack {

    void init();
    void invalidateCache();

} // end namespace FlashStack


#endif
