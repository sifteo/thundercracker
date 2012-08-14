#ifndef IDLETIMEOUT_H
#define IDLETIMEOUT_H

#include "macros.h"

class IdleTimeout
{
public:
    static ALWAYS_INLINE void reset() {
        countdown = IDLE_TIMEOUT_SYSTICKS;
    }

    static void heartbeat();

private:
    /*
     * Heartbeat is at 10Hz, our idle timeout is 10 minutes.
     */
    static const unsigned IDLE_TIMEOUT_SYSTICKS = 10 * 60 * 10;
    static unsigned countdown;
};

#endif // IDLETIMEOUT_H
