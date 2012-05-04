/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_ABI_AUDIO_H
#define _SIFTEO_ABI_AUDIO_H

#include <sifteo/abi/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Audio handles
 */

typedef uint8_t _SYSAudioChannelID;     /// Audio channel slot index
#define _SYS_AUDIO_INVALID_CHANNEL_ID   ((_SYSAudioChannelID)-1)

// NOTE - _SYS_AUDIO_BUF_SIZE must be power of 2 for our current FIFO implementation,
// but must also accommodate a full frame's worth of speex data. If we go narrowband,
// that's 160 shorts so we can get away with 512 bytes. Wideband is 320 shorts
// so we need to kick up to 1024 bytes. kind of a lot :/
#define _SYS_AUDIO_BUF_SIZE             (512 * sizeof(int16_t))
#define _SYS_AUDIO_MAX_CHANNELS         8

#define _SYS_AUDIO_MAX_VOLUME       256   // Guaranteed to be a power of two
#define _SYS_AUDIO_DEFAULT_VOLUME   128

/*
 * Types of audio supported by the system
 */
enum _SYSAudioType {
    _SYS_PCM = 1,
    _SYS_ADPCM = 2
};

enum _SYSAudioLoopType {
    _SYS_LOOP_UNDEF     = -1,
    _SYS_LOOP_ONCE      = 0,
    _SYS_LOOP_REPEAT    = 1,
};

struct _SYSAudioModule {
    uint32_t sampleRate;    /// Native sampling rate of data
    uint32_t loopStart;     /// Loop starting point, in samples
    uint32_t loopEnd;       /// Loop ending point, in samples
    uint8_t loopType;       /// Loop type, 0 (no looping) or 1 (forward loop)
    uint8_t type;           /// _SYSAudioType code
    uint16_t volume;        /// Sample default volume (overridden by explicit channel volume)
    uint32_t dataSize;      /// Size of compressed data, in bytes
    uint32_t pData;            /// Flash address for compressed data
};

struct _SYSXMPattern {
    uint16_t nRows;         /// Number of rows in the pattern
    uint16_t dataSize;      /// Size of compressed data, in bytes
    uint32_t pData;         /// Flash address for compressed data
};

struct _SYSXMInstrument {
    struct _SYSAudioModule sample; /// Instrument's sample data
    int8_t finetune;                /// Minor frequency adjustment to note, as x/128 halftone (-1..127/128).
    int8_t relativeNoteNumber;      /// Relative note number (for increasing/decreasing native sample bitrate) (-96..95, 0 = C-4)
    
    uint32_t volumeEnvelopePoints;  /// Flash address for array of envelope points, each is 9 bits of offset (?,0..130) and 7 bits of value (0..64)
    uint8_t nVolumeEnvelopePoints;  /// Size of volumeEnvelopePoints in shorts (0..12)
    uint8_t volumeSustainPoint;     /// TODO
    uint8_t volumeLoopStartPoint;   /// TODO
    uint8_t volumeLoopEndPoint;     /// TODO
    uint8_t volumeType;             /// (0: On, 1: Sustain, 2: Loop)
    uint8_t vibratoType;            /// TODO
    uint8_t vibratoSweep;           /// TODO
    uint8_t vibratoDepth;           /// TODO
    uint8_t vibratoRate;            /// TODO
    uint16_t volumeFadeout;         /// TODO
};

struct _SYSXMSong {
    uint32_t patternOrderTable;     /// Flash address for the song's list of patterns to play
    uint16_t patternOrderTableSize; /// Size of patternOrderTable in bytes (1..256)
    uint8_t restartPosition;        /// Zero-indexed position in patternOrderTable to loop to (0..255)
    uint8_t nChannels;              /// Number of channels used by the tracker (1.._SYS_AUDIO_MAX_CHANNELS)
    
    uint16_t nPatterns;             /// Number of patterns in patterns (1..256)
    uint32_t patterns;              /// Flash address for the patterns of the song
    
    uint8_t nInstruments;           /// Number of instruments in instruments (0..128)
    uint32_t instruments;           /// Flash address for the instruments of the song
    
    uint8_t frequencyTable;         /// Which frequency table the track uses (0: Amiga, 1: Linear)
    uint16_t tempo;                 /// Default playback speed (ticks)
    uint16_t bpm;                   /// Default beats per minute (notes)
};

struct _SYSAudioBuffer {
    uint16_t head;          /// Index of the next sample to read
    uint16_t tail;          /// Index of the next empty slot to write into
    uint8_t buf[_SYS_AUDIO_BUF_SIZE];
};


#ifdef __cplusplus
}  // extern "C"
#endif

#endif
