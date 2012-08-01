/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "shutdown.h"
#include "tasks.h"
#include "led.h"
#include "homebutton.h"
#include "radio.h"
#include "svmloader.h"
#include "cubeslots.h"
#include "cubeconnector.h"

#ifndef SIFTEO_SIMULATOR
#   include "powermanager.h"
#endif


ShutdownManager::ShutdownManager(uint32_t excludedTasks)
	: excludedTasks(excludedTasks)
{}

void ShutdownManager::shutdown()
{
	// Make sure the home button is released before we go on
	while (HomeButton::isPressed())
		Tasks::idle(excludedTasks);

	// First round of shut down. We'll appear to be off.
	LED::set(NULL);
	CubeConnector::disableReconnect();
	CubeSlots::sendShutdown = ~0;

	// We have plenty of time now. Clean up.
	housekeeping();

	// Wait until we've finished shutting down all cubes
	while (CubeSlots::sendShutdown & CubeSlots::sysConnected)
		Tasks::idle(excludedTasks);

	// If we're on battery power, now's a good time to sign off.
	hardwareShutdown();

	/*
	 * Otherwise, keep serving USB requests, but without any radio traffic.
	 * Wait until the home button has been pressed again.
	 */

	RadioManager::disableRadio();
	CubeSlots::disconnectCubes(CubeSlots::sysConnected);

	while (!HomeButton::isPressed())
		Tasks::idle(excludedTasks);

	/*
	 * Back to life! From the user's perspective, this looks like a
	 * fresh boot. However we don't actually want to reboot the system,
	 * since that would unnecessarily cause us to drop off the USB.
	 */

	Tasks::cancel(Tasks::HomeButton);
	CubeConnector::enableReconnect();
	RadioManager::enableRadio();
	SvmLoader::execLauncher();
}

void ShutdownManager::housekeeping()
{
	// XXX: Filesystem garbage collection
	// XXX: Pre-erase flash blocks
}

void ShutdownManager::hardwareShutdown()
{
	// On hardware only, and on battery power only, turn the system off.
	// Has no effect in simulation, or while we're running on USB power.

	#ifndef SIFTEO_SIMULATOR
	    if (PowerManager::state() == PowerManager::BatteryPwr)
	        return PowerManager::batteryPowerOff();
	#endif
}
