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
    void invalidateCache(unsigned flags = 0);

    /**
     * The nuclear option.
     *
     * Physically erase the entire device, and pre-erase all blocks.
     * If SVM is executing code, this places the virtual CPU in an abort trap.
     */
    void reformatDevice();

} // end namespace FlashStack


#endif
