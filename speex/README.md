
Speex Support
==============

Tools for preparing audio for Thundercracker - still very much in flux.


Parts
-----

encoder
  simple app to encode 16-bit PCM audio data via speex, such that it can be loaded onto 
  the master cube.

decoder
  For now, just code to test the encoder - the real decode action will happen on the master cube.

Build
-----
Currently, we require a libspeex installation - I've installed via homebrew on OS X, should be reasonably
straight forward on Linux. Have not yet tested on Windows...
