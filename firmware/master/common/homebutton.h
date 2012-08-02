/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef HOMEBUTTON_H_
#define HOMEBUTTON_H_

#include <stdint.h>
#include "systime.h"


namespace HomeButton
{
    // Hardware-specific
    void init();
    bool isPressed();

    // Hardware-independent
    void task();
}


/**
 * This is a simple utility class to detect a press event, in cases where
 * the button may already be down. We wait to see both a button release
 * and a press.
 */

class HomeButtonPressDetector {
public:
	enum State {
		S_UNKNOWN,
		S_IDLE,
		S_PRESSED,
		S_RELEASED,
	};

	HomeButtonPressDetector() : pressTimestamp(0), state(S_UNKNOWN) {}
	void update();

	/// Has button been pressed, after having been up?
	ALWAYS_INLINE bool isPressed() const {
		return state == S_PRESSED;
	}

	/// Has the button been released, after being pressed?
	ALWAYS_INLINE bool isReleased() const {
		return state == S_RELEASED;
	}

	/// How long has the button been pressed for? (Does not enforce prior released-ness)
	SysTime::Ticks pressDuration() const;

private:
	SysTime::Ticks pressTimestamp;
	State state;
};


#endif // HOMEBUTTON_H_
