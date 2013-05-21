Changelog      {#changelog}
===========================

@brief A summary of the relevant changes in each SDK release.

# vNext (Date TBD)

### New
* `swiss savedata delete` can be used to remove just the save data for a particular game. See @ref device_mgmt for details.
* Sifteo::Metadata::isDemoOf() added to indicate that an app is a demo version of another full app.

### Changes
* Sifteo::TileBuffer::tileAddr() was made const, and Sifteo::TileBuffer::tile() and Sifteo::RelocatableTileBuffer::tile() were changed to accept a UInt2 rather than an Int2 pos parameter.

# v1.0.0 (March 27, 2013)

### New
* LOG() now works on hardware as well as the siftulator. Use `swiss listen` to access the log output - see @ref device_mgmt for details.
* `swiss savedata` can help you extract, restore and analyze Sifteo::StoredObject data that has been collected on a base - see @ref device_mgmt for details.
* added Sifteo::AudioTracker::setTempoModifier() - ability to scale the tempo of tracker modules.
* added Sifteo::AudioTracker::setPosition() - ability to update the position of the currently playing tracker module.
* added Sifteo::System::osVersion() and Sifteo::System::hardwareVersion() to check the version of the OS and hardware build of the unit your app is running on.
* added Sifteo::Metadata::minimumOSVersion() - advertise that your app relies on a minimum OS version.
* added attribute getters for Sifteo::AssetTracker - bpm(), numChannels(), numIntruments(), numPatterns(), and tempo()
* `swiss delete` can now delete a game by its package string. Deletion by volume code is still supported - see @ref device_mgmt for details.

### Fixes
* neighboring a cube to the base works again, see Sifteo::NeighborID::isBase() can return true.
* improved the siftulator's ability to maintain lower audio output latencies when possible.
* Sifteo::AssetAudio::speed() is now const

# v0.9.8 (October 3, 2012)

### New
* Initial release!
