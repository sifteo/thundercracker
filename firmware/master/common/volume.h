#ifndef VOLUME_H
#define VOLUME_H

namespace Volume
{
    void init();
    int systemVolume();  // current system volume
    void beginSample();   // begin taking a new sample of the volume
}

#endif // VOLUME_H
