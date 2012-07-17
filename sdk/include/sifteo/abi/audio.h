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


typedef uint8_t _SYSAudioChannelID;     /// Audio channel slot index

#define _SYS_AUDIO_INVALID_CHANNEL_ID   ((_SYSAudioChannelID)-1)
#define _SYS_AUDIO_MAX_CHANNELS         8

#define _SYS_AUDIO_MAX_VOLUME_LOG2      8
#define _SYS_AUDIO_MAX_VOLUME           (1 << _SYS_AUDIO_MAX_VOLUME_LOG2)
#define _SYS_AUDIO_DEFAULT_VOLUME       (_SYS_AUDIO_MAX_VOLUME / 2)


/*
 * Types of audio supported by the system
 */
enum _SYSAudioType {
    _SYS_PCM = 1,
    _SYS_ADPCM = 2
};

enum _SYSAudioLoopType {
    _SYS_LOOP_UNDEF              = -1,
    _SYS_LOOP_ONCE               = 0,
    _SYS_LOOP_REPEAT             = 1,
    _SYS_LOOP_EMULATED_PING_PONG = 2,
};

struct _SYSAudioModule {
    uint32_t sampleRate;    /// Native sampling rate of data, in Hz
    uint32_t loopStart;     /// Index of first sample in loop
    uint32_t loopEnd;       /// Index of first sample past the end of the loop/sample
    uint8_t loopType;       /// Loop type, 0 (stop at loopEnd) or 1 (forward loop)
    uint8_t type;           /// _SYSAudioType code
    uint16_t volume;        /// Sample default volume (overridden by explicit channel volume)
    uint32_t dataSize;      /// Size of compressed data, in bytes
    uint32_t pData;         /// Flash address for compressed data
};

struct _SYSXMPattern {
    uint16_t nRows;         /// Number of rows in the pattern
    uint16_t dataSize;      /// Size of compressed data, in bytes
    uint32_t pData;         /// Flash address for compressed data
};

struct _SYSXMInstrument {
    struct _SYSAudioModule sample;  /// Instrument's sample data
    int8_t finetune;                /// Minor frequency adjustment to note, as x/128 halftone (-1..127/128).
    int8_t relativeNoteNumber;      /// Relative note number (for increasing/decreasing native sample bitrate) (-96..95, 0 = C-4)
    uint8_t compression;            /// Original number of nibbles per sample
    
    uint32_t volumeEnvelopePoints;  /// Flash address for array of envelope points, each is 9 bits of offset (?,0..130) and 7 bits of value (0..64)
    uint8_t nVolumeEnvelopePoints;  /// Size of volumeEnvelopePoints in shorts (0..12)
    uint8_t volumeSustainPoint;     /// Volume envelope sustain point
    uint8_t volumeLoopStartPoint;   /// Volume envelope loop starting point
    uint8_t volumeLoopEndPoint;     /// Volume envelope loop ending point
    uint8_t volumeType;             /// (0: Enabled, 1: Sustain, 2: Loop)
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

#ifdef __cplusplus
}  // extern "C"
#endif

#endif
