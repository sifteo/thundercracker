
# Bootloader Overview

## Main Responsibilities

* on boot, verify via CRC that the contents of MCU flash constitute a valid image
* when requested, or in the event that MCU flash is corrupt, load a new firmware image into the master's MCU flash. this involves:
** receive encrypted data incrementally over USB
** decrypt it
** write it to flash
** loop to verification and continue
* branch to application firmware

## Design Requirements

* must be safe in the event of power failure
* never stores a copy of the firmware image anywhere other than its final resting place
* provide a mechanism to persistently and safely indicate whether the system should execute the loader at boot (ie, an update has been requested)
* maintain readout protection of the data programmed to MCU flash - bootloader must be in the first couple pages of flash to retain ability to program MCU flash. We rely on the cortex-M3's ability to specify an offset for the vector table to avoid additional indirection for ISRs within the application code.

## Concessions

Our encryption is not robust. However, doing something more complete would be more expensive in terms of resources and effort than we're interested in at the moment. Our main motivation is to deter casual copycats.
