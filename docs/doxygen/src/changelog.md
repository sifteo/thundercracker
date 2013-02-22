Changelog      {#changelog}
===========================

@brief A summary of the relevant changes in each SDK release.

# v1.0.0 (date TBD)

### New
* LOG() now works on hardware as well as the siftulator. Use `swiss listen` to access the log output - see @ref device_mgmt for details.
* `swiss savedata` can help you extract, restore and analyze Sifteo::StoredObject data that has been collected on a base - see @ref device_mgmt for details.
* added Sifteo::AudioTracker::setTempoModifier() - ability to scale the tempo of tracker modules.
* added Sifteo::AudioTracker::setPosition() - ability to update the position of the currently playing tracker module.
* added Sifteo::System::osVersion() and Sifteo::System::hardwareVersion() to check the version of the OS and hardware build of the unit your app is running on.
* added Sifteo::Metadata::minimumOSVersion() - advertise that your app relies on a minimum OS version.
* added attribute getters for Sifteo::AssetTracker - bpm(), numChannels(), numIntruments(), numPatterns(), and tempo()

### Fixes
* neighboring a cube to the base works again, see Sifteo::NeighborID::isBase() can return true.
* improved the siftulator's ability to maintain lower audio output latencies when possible.
* Sifteo::AssetAudio::speed() is now const

# v0.9.8 (October 3, 2012)

### New
* Initial release!
