/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "xmtrackerplayer.h"
#include "audiomixer.h"
#include <stdint.h>
#include <stdlib.h>

// mingw appears to not get this quite right...
#ifndef UINT16_MAX
#define UINT16_MAX 0xffff
#endif
#ifndef UINT32_MAX
#define UINT32_MAX 0xffffffff
#endif

//#define XMTRACKERDEBUG
#define LGPFX "XmTrackerPlayer: "
XmTrackerPlayer XmTrackerPlayer::instance;
const uint8_t XmTrackerPlayer::kLinearFrequencies;
const uint8_t XmTrackerPlayer::kAmigaFrequencies;
const uint8_t XmTrackerPlayer::kMaxVolume;
const uint8_t XmTrackerPlayer::kEnvelopeSustain;
const uint8_t XmTrackerPlayer::kEnvelopeLoop;

#include "xmtrackertables.h"

// To be replaced someday by a channel allocator and array
#define CHANNEL_FOR(x) (_SYS_AUDIO_MAX_CHANNELS - ((x) + 1))

bool XmTrackerPlayer::play(const struct _SYSXMSong *pSong)
{
    // Does the world make any sense? 
    ASSERT(pSong->nPatterns > 0);
    if (pSong->nChannels > _SYS_AUDIO_MAX_CHANNELS) {
        LOG((LGPFX"Song has %u channels, %u supported\n",
             pSong->nChannels, _SYS_AUDIO_MAX_CHANNELS));
        return false;
    }

    /* Make sure no one is currently using the channels I need.
     * Unfortunately, there's no way to make sure no one else clobbers me
     * during playback, but it won't take long for the perp to notice.
     */
    for (unsigned i = 0; i < pSong->nChannels; i++)
        if (AudioMixer::instance.isPlaying(CHANNEL_FOR(i)))
            return false;

    // Ok, things look (probably) good.
    song = *pSong;
    
    bpm = song.bpm;
    ticks = tempo = song.tempo;
    delay = 0;
    phrase = 0;
    row = 0;
    pattern.init(&song)->loadPattern(patternOrderTable(phrase));

    memset(channels, 0, sizeof(channels));
    for (unsigned i = 0; i < arraysize(channels); i++) {
        XmTrackerPattern::resetNote(channels[i].note);
    }

    AudioMixer::instance.setTrackerCallbackInterval(2500000 / bpm);
    tick();

    return true;
}

void XmTrackerPlayer::stop()
{
    song.nPatterns = 0;
    AudioMixer::instance.setTrackerCallbackInterval(0);
}

inline void XmTrackerPlayer::loadNextNotes()
{
    ASSERT(!ticks);

    // Advance song position on row overflow or effect
    if (next.row >= pattern.nRows() || next.force) {
        if (!next.force)
            processPatternBreak(song.patternOrderTableSize, 0);

        if (phrase != next.phrase && next.phrase < song.patternOrderTableSize) {
            pattern.loadPattern(patternOrderTable(next.phrase));
            phrase = next.phrase;
        } else if (next.phrase >= song.patternOrderTableSize) {
            stop();
        }

        if (next.row >= pattern.nRows())
            stop();
        
        next.force = false;
    }

    // Get and process the next row of notes
    struct XmTrackerNote note;
    for (unsigned i = 0; i < song.nChannels; i++) {
        struct XmTrackerChannel &channel = channels[i];
        pattern.getNote(next.row, i, note);

#ifdef XMTRACKERDEBUG
        if (i) LOG((" | "));
        LOG(("%3d %3d x%02x x%02x x%02x",
             note.note, note.instrument,
             note.volumeColumnByte,
             note.effectType, note.effectParam));
        if ((int)i == song.nChannels - 1) LOG(("\n"));
#endif

        channel.valid = false;
        bool recNote = false,
             recInst = false;
        if (note.instrument == XmTrackerPattern::kNoInstrument) {
            recInst = true;
            note.instrument = channel.note.instrument;
        }
        if (note.note == XmTrackerPattern::kNoNote) {
            recNote = true;
            note.note = channel.note.note;
        }

        if (note.effectType != XmTrackerPattern::kNoEffect &&
            note.effectParam == XmTrackerPattern::kNoParam)
        {
            note.effectParam = 0;
        }

        if (channel.note.instrument != note.instrument && note.instrument < song.nInstruments) {
            // Change the instrument.
            // TODO: Consider mapping this instead of copying, if we can be guaranteed the whole struct.
            if (!SvmMemory::copyROData(channel.instrument, song.instruments + note.instrument * sizeof(_SYSXMInstrument))) {
                ASSERT(false);
            }
            // TODO: auto-vibrato.
            channel.vibrato.phase = 0;
        } else if (note.instrument >= song.nInstruments) {
            channel.instrument.sample.pData = 0;
        }
        if (!recInst && channel.instrument.sample.pData) {
            channel.volume = channel.instrument.sample.volume;
        }
        
        channel.start = !recNote || !recInst;
        // Don't play with an invalid instrument
        if (channel.start && note.instrument >= song.nInstruments) {
            channel.start = false;
        }
        // Stop playing/don't play when no sample data or note
        if (channel.realNote(note.note) >= XmTrackerPattern::kNoteOff ||
            !channel.instrument.sample.pData)
        {
            channel.start = false;
            channel.active = false;
        }

        if (!channel.active && !channel.start) {
            channel.note = note;
            continue;
        }

        // Remember old/current period for portamento slide.
        channel.porta.period = channel.period;
        if (!recNote) {
            channel.period = getPeriod(channel.realNote(note.note), channel.instrument.finetune);
            if (note.instrument != channel.note.instrument || !channel.porta.period) {
                channel.porta.period = channel.period;
            }
        }
        if (channel.period) {
            channel.frequency = getFrequency(channel.period);
        }
        channel.note = note;

        if (channel.start) {
            channel.envelope.done = !channel.instrument.nVolumeEnvelopePoints;
            memset(&channel.envelope, 0, sizeof(channel.envelope));
        }

        // Volume
        channel.fadeout = UINT16_MAX;
        if (note.volumeColumnByte >= 0x10 && note.volumeColumnByte <= 0x50)
                channel.volume = note.volumeColumnByte - 0x10;

        channel.active = true;
    }
    pattern.releaseRef();

    // Advance song position
    next.row++;
}

// Volume commands
enum {
    vxSlideDown              = 6,
    vxSlideUp,              // 7
    vxFineVolumeDown,       // 8
    vxFineVolumeUp,         // 9
    vxVibratoSpeed,         // A
    vxVibratoDepth,         // B
    vxTonePortamento        // F. C, D, and E are panning commands
};

void XmTrackerPlayer::incrementVolume(uint16_t &volume, uint8_t inc)
{
    volume = MIN(volume + inc, kMaxVolume);
}

void XmTrackerPlayer::decrementVolume(uint16_t &volume, uint8_t dec)
{
    volume = (uint16_t)MAX((int32_t)volume - dec, 0);
}

void XmTrackerPlayer::processVolumeSlideUp(XmTrackerChannel &channel, uint16_t &volume, uint8_t inc)
{
    if (inc) channel.slideUp = inc;
    incrementVolume(volume, inc);
}

void XmTrackerPlayer::processVolumeSlideDown(XmTrackerChannel &channel, uint16_t &volume, uint8_t dec)
{
    if (dec) channel.slideDown = dec;
    decrementVolume(volume, dec);
}

void XmTrackerPlayer::processVibrato(XmTrackerChannel &channel)
{
    int32_t periodDelta = 0;

    switch (channel.vibrato.type) {
        default:
            // Intentional fall-through
        case 3:   // Random (but not really)
            // Intentional fall-through
        case 0: { // Sine
            // Half-wave precomputed sine table.
            static const uint8_t sineTable[] = {0,24,49,74,97,120,141,161,
                                                180,197,212,224,235,244,250,253,
                                                255,253,250,244,235,224,212,197,
                                                180,161,141,120,97,74,49,24};
            STATIC_ASSERT(arraysize(sineTable) == 32);
            periodDelta = sineTable[channel.vibrato.phase % arraysize(sineTable)];
            if (channel.vibrato.phase % 64 >= 32) periodDelta = -periodDelta;
            break;
        }
        case 1:   // Ramp up
            LOG(("%s:%d: NOT_TESTED: ramp vibrato\n", __FILE__, __LINE__));
            periodDelta = (channel.vibrato.phase % 32) * -8;
            if ((channel.vibrato.phase % 64) >= 32) periodDelta += 255;
            break;
        case 2:   // Square
            LOG(("%s:%d: NOT_TESTED: square vibrato\n", __FILE__, __LINE__));
            periodDelta = 255;
            if (channel.vibrato.phase % 64 >= 32) periodDelta = -periodDelta;
            break;
    }
    periodDelta = (periodDelta * channel.vibrato.depth) / 32;

    channel.period += periodDelta;

    if (ticks)
        channel.vibrato.phase += channel.vibrato.speed;
}

void XmTrackerPlayer::processPorta(XmTrackerChannel &channel)
{
    if (!ticks) {
        uint32_t tmp;
        tmp = channel.period;
        channel.period = channel.porta.period;
        channel.porta.period = tmp;
        // Not playing a new note anymore, sustain-shifting to another.
        channel.start = false;
        return;
    }

    ASSERT(channel.porta.period);

    int32_t delta=(int32_t)channel.period-(int32_t)channel.porta.period;

    if (!delta) return;

    if (abs(delta) < channel.tonePorta * 4) {
        channel.period = channel.porta.period;
    } else if (delta < 0) {
        channel.period += channel.tonePorta * 4;
    } else {
        channel.period -= channel.tonePorta * 4;
    }
    channel.frequency = getFrequency(channel.period);
}

void XmTrackerPlayer::processVolume(XmTrackerChannel &channel)
{
    uint8_t command = channel.note.volumeColumnByte >> 4;
    uint8_t param = channel.note.volumeColumnByte & 0xF;

    switch (command) {
        case vxSlideDown:
            if (!param) LOG(("%s:%d: NOT_IMPLEMENTED: empty param to vxSlideDown\n", __FILE__, __LINE__));
            LOG(("%s:%d: NOT_TESTED: vxSlideDown vx(0x%02x)\n", __FILE__, __LINE__, channel.note.volumeColumnByte));
            decrementVolume(channel.volume, param);
            break;
        case vxSlideUp:
            if (!param) LOG(("%s:%d: NOT_IMPLEMENTED: empty param to vxSlideUp\n", __FILE__, __LINE__));
            incrementVolume(channel.volume, param);
            break;
        case vxFineVolumeDown:
            LOG(("%s:%d: NOT_TESTED: vxFineVolumeDown vx(0x%02x)\n", __FILE__, __LINE__, channel.note.volumeColumnByte));
            if (ticks) break;
            processVolumeSlideDown(channel, channel.volume, param);
            break;
        case vxFineVolumeUp:
            LOG(("%s:%d: NOT_TESTED: vxFineVolumeUp vx(0x%02x)\n", __FILE__, __LINE__, channel.note.volumeColumnByte));
            if (ticks) break;
            processVolumeSlideUp(channel, channel.volume, param);
            break;
        case vxVibratoSpeed:
            LOG(("%s:%d: NOT_IMPLEMENTED: vxVibratoSpeed vx(0x%02x)\n", __FILE__, __LINE__, channel.note.volumeColumnByte));
            // TODO: do vibrato as normal, but when complete, hold the final pitch
            break;
        case vxVibratoDepth:
            LOG(("%s:%d: NOT_IMPLEMENTED: vxVibratoDepth vx(0x%02x)\n", __FILE__, __LINE__, channel.note.volumeColumnByte));
            // TODO: do vibrato as normal, but when complete, hold the final pitch
            break;
        case vxTonePortamento:
            LOG(("%s:%d: NOT_IMPLEMENTED: vxTonePortamento vx(0x%02x)\n", __FILE__, __LINE__, channel.note.volumeColumnByte));
            break;
        default:
            break;
    }
}

// Note effects
enum {
    fxArpeggio                   = 0x00, // "Appregio" in the file spec *frown*
    fxPortaUp,                  // 0x01
    fxPortaDown,                // 0x02
    fxTonePorta,                // 0x03
    fxVibrato,                  // 0x04
    fxTonePortaAndVolumeSlide,  // 0x05
    fxVibratoAndVolumeSlide,    // 0x06
    fxTremolo,                  // 0x07
    // fx8 is Set Panning, not implemented.
    fxSampleOffset               = 0x09,
    fxVolumeSlide,              // 0x0A
    fxPositionJump,             // 0x0B
    fxSetVolume,                // 0x0C
    fxPatternBreak,             // 0x0D
    fxOverflow,                 // 0x0E, hack inherited from the MOD format
    fxSetTempoAndBPM,           // 0x0F
    fxSetGlobalVolume,          // 0x10 / G
    fxGlobalVolumeSlide,        // 0x11 / H
    // TODO: fxKeyOff (K)
    fxSetEnvelopePos             = 0x15, // L
    fxMultiRetrigNote            = 0x1B, // R
    fxTremor                     = 0x1D, // T
    fxExtraFinePorta             = 0x21, // X
};

enum {
    fxFinePortaUp                = 0x01, // E1
    fxFinePortaDown,            // 0x02
    fxGlissControl,             // 0x03
    fxVibratoControl,           // 0x04
    fxFinetune,                 // 0x05
    fxLoopPattern,              // 0x06, called "Set loop begin/loop" in XM spec
    fxTremoloControl,           // 0x07
    fxRetrigNote                 = 0x09, // 8 is not used
    fxFineVolumeSlideUp,        // 0x0A
    fxFineVolumeSlideDown,      // 0x0B
    fxNoteCut,                  // 0x0C
    fxNoteDelay,                // 0x0D
    fxPatternDelay              // 0x0E
};

void XmTrackerPlayer::processArpeggio(XmTrackerChannel &channel)
{
    uint8_t phase = ticks % 3;
    uint8_t note;

    // ProTracker 2/3 sequence. FastTracker II reverses cases 1 and 2.
    switch (phase) {
        case 0:
            return;
        case 1:
            note = channel.note.effectParam >> 4;
            break;
        case 2:
            note = channel.note.effectParam & 0x0F;
            break;
    }
    note += channel.realNote();

    if (note >= XmTrackerPattern::kNoteOff) {
        LOG((LGPFX"Clipped arpeggio (base note: %d, arpeggio: %02x)\n",
             channel.realNote(), channel.note.effectParam));
        note = XmTrackerPattern::kNoteOff - 1;
    }

    // Apply relative period shift, to avoid disrupting other active effects
    channel.frequency = getFrequency(getPeriod(note, channel.instrument.finetune));
}

void XmTrackerPlayer::processVolumeSlide(XmTrackerChannel &channel)
{
    if (!ticks || !channel.slide) return;
    uint8_t up = channel.slide >> 4;
    uint8_t down = channel.slide & 0x0F;

    if (up) {
        incrementVolume(channel.volume, up);
    } else if (down) {
        decrementVolume(channel.volume, down);
    }
}

void XmTrackerPlayer::processPatternBreak(uint16_t nextPhrase, uint16_t nextRow)
{
    next.force = true;

    // Adjust phrase as appropriate
    if (nextPhrase >= song.patternOrderTableSize) next.phrase = phrase + 1;
    else next.phrase = nextPhrase;

    // This is bounds-checked later, when the pattern is loaded.
    next.row = nextRow;

    // Reached end of song
    if (next.phrase >= song.patternOrderTableSize) {
        // This is bounds-checked later, when the break is applied.
        next.phrase = song.restartPosition;
    }
}

void XmTrackerPlayer::processRetrigger(XmTrackerChannel &channel, uint8_t interval, uint8_t slide)
{
    if (!channel.retrigger.interval) return;
    if (!ticks) channel.retrigger.phase = 0;

    if (channel.retrigger.phase >= channel.retrigger.interval) {
        channel.retrigger.phase = 0;

        if (slide >= 1 && slide <= 5) {
            decrementVolume(channel.volume, 1 << (slide - 1));
        } else if (slide >= 9 && slide <= 0xD) {
            incrementVolume(channel.volume, 1 << (slide - 9));
        } else if (slide == 6) {
            channel.volume = channel.volume * 2 / 3;
        } else if (slide == 7) {
            channel.volume /= 2;
        } else if (slide == 0xE) {
            incrementVolume(channel.volume, channel.volume / 2);
        } else if (slide == 0xF) {
            incrementVolume(channel.volume, channel.volume);
        }
    }

    channel.retrigger.phase++;
}

void XmTrackerPlayer::processEffects(XmTrackerChannel &channel)
{
    const uint8_t param = channel.note.effectParam;
    switch (channel.note.effectType) {
        case fxArpeggio: {
            processArpeggio(channel);
            break;
        }
        case fxPortaUp: {
            // ProTracker 2/3 does not support memory.
            if (!ticks && param) channel.portaUp = param;
            channel.period -= channel.portaUp;
            break;
        }
        case fxPortaDown: {
            // ProTracker 2/3 does not support memory.
            if (!ticks && param) channel.portaDown = param;
            channel.period += channel.portaDown;
            break;
        }
        case fxTonePorta: {
            if (!ticks && param) channel.tonePorta = param;
            processPorta(channel);
            break;
        }
        case fxVibrato: {
            if (!ticks) {
                if (channel.start) channel.vibrato.phase = 0;
                if (param & 0xF0) channel.vibrato.speed = (param & 0xF0) >> 4;
                if (param & 0x0F) channel.vibrato.depth = param & 0x0F;
            }
            processVibrato(channel);
            break;
        }
        case fxTonePortaAndVolumeSlide: {
            /* Strictly speaking, if both nibbles in the parameter are nonzero,
             * this command is illegal.
             */
            if (!ticks && param) channel.slide = param;
            processVolumeSlide(channel);
            processPorta(channel);
            break;
        }
        case fxVibratoAndVolumeSlide: {
            /* Strictly speaking, if both nibbles in the parameter are nonzero,
             * this command is illegal.
             */
            if (param) channel.slide = param;
            processVolumeSlide(channel);
            processVibrato(channel);
            break;
        }
        case fxTremolo: {
            LOG(("%s:%d: NOT_IMPLEMENTED: fxTremolo fx(0x%02x)\n", __FILE__, __LINE__, channel.note.effectType));
            break;
        }
        case fxSampleOffset: {
            if (ticks) break;
            channel.offset = param << 8;
            // Compensate for sample compression (param is in shorts, offset is in samples)
            channel.offset *= channel.instrument.compression;
            break;
        }
        case fxVolumeSlide: {
            /* Unlike the volume slide combination effects, both nibbles in the
             * parameter are allowed to be set.
             */
            if (param) channel.slide = param;
            processVolumeSlide(channel);
            break;
        }
        case fxPositionJump: {
            // Only useful at the start of a note
            ASSERT(!ticks);
            channel.note.effectType = XmTrackerPattern::kNoEffect;

            processPatternBreak(param, 0);
            break;
        }
        case fxSetVolume: {
            channel.volume = MIN(param, kMaxVolume);
            break;
        }
        case fxPatternBreak: {
            // Only useful at the start of a note
            ASSERT(!ticks);
            channel.note.effectType = XmTrackerPattern::kNoEffect;

            // Seriously, the spec says the higher order nibble is * 10, not * 16.
            processPatternBreak(-1, (param & 0xF) + (param >> 4) * 10);
            break;
        }
        case fxSetTempoAndBPM: {
            // Only useful at the start of a note
            ASSERT(!ticks);
            channel.note.effectType = XmTrackerPattern::kNoEffect;

            if (!param)
                stop();
            else if (param <= 32)
                tempo = param;
            else
                bpm = param;
            break;
        }
        case fxSetGlobalVolume: {
            LOG(("%s:%d: NOT_IMPLEMENTED: fxSetGlobalVolume fx(0x%02x)\n", __FILE__, __LINE__, channel.note.effectType));
            break;
        }
        case fxGlobalVolumeSlide: {
            LOG(("%s:%d: NOT_IMPLEMENTED: fxGlobalVolumeSlide fx(0x%02x)\n", __FILE__, __LINE__, channel.note.effectType));
            break;
        }
        case fxSetEnvelopePos: {
            LOG(("%s:%d: NOT_IMPLEMENTED: fxSetEnvelopePos fx(0x%02x)\n", __FILE__, __LINE__, channel.note.effectType));
            break;
        }
        case fxMultiRetrigNote: {
            if (param & 0xF0) channel.retrigger.speed = (param & 0xF0) >> 4;
            if (param & 0x0F) channel.retrigger.interval = param & 0x0F;
            processRetrigger(channel, channel.retrigger.interval, channel.retrigger.speed);
            break;
        }
        case fxTremor: {
            LOG(("%s:%d: NOT_IMPLEMENTED: fxTremor fx(0x%02x)\n", __FILE__, __LINE__, channel.note.effectType));
            break;
        }
        case fxExtraFinePorta: {
            if (param >> 4 == 1) {
                if (!ticks) channel.period -= (param & 0x0F);
            } else if (param >> 4 == 2) {
                if (!ticks) channel.period += (param & 0x0F);
            }
            break;
        }
        case fxOverflow: {
            switch (param >> 4) {
                case fxFinePortaUp: {
                    if (!ticks) channel.period -= (param & 0x0F) * 4;
                    break;
                }
                case fxFinePortaDown: {
                    if (!ticks) channel.period += (param & 0x0F) * 4;
                    break;
                }
                case fxGlissControl: {
                    LOG(("%s:%d: NOT_IMPLEMENTED: fxGlissControl fx(0x%02x, 0x%02x)\n", __FILE__, __LINE__, channel.note.effectType, param));
                    break;
                }
                case fxVibratoControl: {
                    if (ticks) break;
                    channel.vibrato.type = (param & 0x0F);
                    break;
                }
                case fxFinetune: {
                    LOG(("%s:%d: NOT_IMPLEMENTED: fxFinetune fx(0x%02x, 0x%02x)\n", __FILE__, __LINE__, channel.note.effectType, param));
                    break;
                }
                case fxLoopPattern: {
                    LOG(("%s:%d: NOT_IMPLEMENTED: fxLoopPattern fx(0x%02x, 0x%02x)\n", __FILE__, __LINE__, channel.note.effectType, param));
                    break;
                }
                case fxTremoloControl: {
                    LOG(("%s:%d: NOT_IMPLEMENTED: fxTremoloControl fx(0x%02x, 0x%02x)\n", __FILE__, __LINE__, channel.note.effectType, param));
                    break;
                }
                case fxRetrigNote: {
                    LOG(("%s:%d: NOT_IMPLEMENTED: fxRetrigNote fx(0x%02x, 0x%02x)\n", __FILE__, __LINE__, channel.note.effectType, param));
                    break;
                }
                case fxFineVolumeSlideUp: {
                    // Only useful at the start of a note
                    ASSERT(!ticks);
                    channel.note.effectType = XmTrackerPattern::kNoEffect;

                    // Save parameter for later use, shared with fxFineVolumeSlideDown.
                    if (param & 0x0F) channel.fineSlide = (channel.fineSlide & 0x0F) | ((param & 0x0F) << 4);
                    incrementVolume(channel.volume, channel.fineSlide >> 4);
                    break;
                }
                case fxFineVolumeSlideDown: {
                    // Only useful at the start of a note
                    ASSERT(!ticks);
                    channel.note.effectType = XmTrackerPattern::kNoEffect;

                    // Save parameter for later use, shared with fxFineVolumeSlideUp.
                    if (param & 0x0F) channel.fineSlide = (channel.fineSlide & 0xF0) | (param & 0x0F);
                    decrementVolume(channel.volume, channel.fineSlide & 0xF);
                    break;
                }
                case fxNoteCut: {
                    LOG(("%s:%d: NOT_IMPLEMENTED: fxNoteCut fx(0x%02x, 0x%02x)\n", __FILE__, __LINE__, channel.note.effectType, param));
                    break;
                }
                case fxNoteDelay: {
                    channel.start = ticks == (param & 0x0F) + 1;
                    break;
                }
                case fxPatternDelay: {
                    delay = param & 0x0f;
                    break;
                }
            }
            break;
        }
        default:
            LOG(("%s:%d: NOT_REACHED: fx(0x%02x, 0x%02x)\n", __FILE__, __LINE__, channel.note.effectType, channel.note.effectParam));
            break;
        case XmTrackerPattern::kNoEffect:
            break;
    }
}

void XmTrackerPlayer::processEnvelope(XmTrackerChannel &channel)
{
    if (!channel.instrument.volumeType || channel.envelope.done) {
        return;
    }

    // Save some space in my editor.
    _SYSXMInstrument &instrument = channel.instrument;
    struct XmTrackerEnvelopeMemory &envelope = channel.envelope;

    ASSERT(instrument.nVolumeEnvelopePoints > 0);

    if ((instrument.volumeType & kEnvelopeSustain) && channel.note.note != XmTrackerPattern::kNoteOff) {
        // volumeSustainPoint is a maxima, don't exceed it.
        if (envelope.point >= instrument.volumeSustainPoint) {
            envelope.point = instrument.volumeSustainPoint;
            envelope.tick = 0;
        }
    } else if (instrument.volumeType & kEnvelopeLoop) {
        // Loop the loop
        if (envelope.point >= instrument.volumeLoopEndPoint) {
            envelope.point = instrument.volumeLoopStartPoint;
            envelope.tick = 0;
        }
    }

    int16_t pointLength = 0;
    uint16_t envPt0 , envPt1 = 0;
    if (envelope.point == instrument.nVolumeEnvelopePoints - 1) {
        // End of envelope
        envelope.done = true;

        // Load the last envelope point from flash.
        SvmMemory::VirtAddr va = instrument.volumeEnvelopePoints + envelope.point * sizeof(uint16_t);
        if (!SvmMemory::copyROData(envPt0, va)) {
            ASSERT(0); stop(); return;
        }
    } else {
        ASSERT(envelope.point < instrument.nVolumeEnvelopePoints - 1);

        // Load current and next envelope points from flash.
        FlashBlockRef ref;
        SvmMemory::VirtAddr va = instrument.volumeEnvelopePoints + envelope.point * sizeof(uint16_t);
        if (!SvmMemory::copyROData(ref, envPt0, va)) {
            ASSERT(0); stop(); return;
        }
        if (!SvmMemory::copyROData(ref, envPt1, va + sizeof(uint16_t))) {
            ASSERT(0); stop(); return;
        }

        pointLength = envelopeOffset(envPt1) - envelopeOffset(envPt0);
    }

    if (envelope.point >= instrument.nVolumeEnvelopePoints - 1 || !envelope.tick) {
        // Beginning of node/end of envelope, no interpolation.
        envelope.value = envelopeValue(envPt0);
    } else {
        /* Interpolates unnecessarily with tick == pointLength, but specifically
         * catching that case isn't really worth it.
         */
        int16_t v1 = envelopeValue(envPt0);
        int16_t v2 = envelopeValue(envPt1);
        envelope.value = v1 + envelope.tick * (v2 - v1) / pointLength;
    }

    // Progress!
    if (envelope.point < instrument.nVolumeEnvelopePoints - 1) {
        envelope.tick++;
        if (envelope.tick >= pointLength) {
            envelope.tick = 0;
            envelope.point++;
        }
    }
}

void XmTrackerPlayer::process()
{
    for (unsigned i = 0; i < song.nChannels; i++) {
        struct XmTrackerChannel &channel = channels[i];

        // TODO: Reset
        // ch->tickrel_period=0;
        // ch->tickrel_volume=0;

        processVolume(channel);
        processEffects(channel);
        processEnvelope(channel);
    }
}

void XmTrackerPlayer::commit()
{
    AudioMixer &mixer = AudioMixer::instance;

    for (unsigned i = 0; i < song.nChannels; i++) {
        struct XmTrackerChannel &channel = channels[i];

        if (!channel.active && mixer.isPlaying(CHANNEL_FOR(i))) {
            mixer.stop(CHANNEL_FOR(i));
            continue;
        }

        /* This code relies on the single-threaded nature of the audio
         * stack--the sample is loaded first and volume and sampling rate
         * adjusted shortly after.
         */
        if (channel.start) {
            channel.start = false;
            if (mixer.isPlaying(CHANNEL_FOR(i))) mixer.stop(CHANNEL_FOR(i));
            if (!mixer.play(&channel.instrument.sample,
                            CHANNEL_FOR(i),
                            (_SYSAudioLoopType)channel.instrument.sample.loopType)) {
                channel.active = false;
                continue;
            }
        }

        channel.active = mixer.isPlaying(CHANNEL_FOR(i));

        // Volume
        int32_t volume = channel.volume;

        if (channel.active && channel.instrument.volumeType) {
            volume = (volume * channel.envelope.value) >> 6;

            if (channel.active && channel.note.note == XmTrackerPattern::kNoteOff) {
                // Update fade
                if (channel.instrument.volumeFadeout > 0xFFF ||
                    channel.instrument.volumeFadeout > channel.fadeout ||
                    !channel.instrument.volumeFadeout)
                {
                    channel.fadeout = 0;
                } else {
                    channel.fadeout -= channel.instrument.volumeFadeout;
                }

                // Completely faded -> stop playing sample.
                if (!channel.fadeout) {
                    channel.active = false;
                }

                // Apply fadeout
                volume = volume * channel.fadeout / UINT16_MAX;
            }
        }
        mixer.setVolume(CHANNEL_FOR(i), clamp(volume * 4, (int32_t)0, (int32_t)_SYS_AUDIO_MAX_VOLUME));

        // Sampling rate
        if (channel.frequency > 0) {
            mixer.setSpeed(CHANNEL_FOR(i), channel.frequency);
        } else if (mixer.isPlaying(CHANNEL_FOR(i))) {
            mixer.stop(CHANNEL_FOR(i));
        }

        // Offset
        if (channel.offset < UINT32_MAX) {
            mixer.setPos(CHANNEL_FOR(i), channel.offset);
            channel.offset = UINT32_MAX;
        }
    }

    // Future effects are capable of adjusting the tempo/bpm
    // (24 / 60 * bpm)Hz
    mixer.setTrackerCallbackInterval(2500000 / bpm);
}

uint8_t XmTrackerPlayer::patternOrderTable(uint16_t order)
{
    ASSERT(order < song.patternOrderTableSize);

    uint8_t buf;
    SvmMemory::VirtAddr va = song.patternOrderTable + order;
    if(!SvmMemory::copyROData(buf, va)) {
        LOG((LGPFX"Could not copy %p (length %lu)!\n",
             (void *)va, (long unsigned)sizeof(buf)));
        ASSERT(false);

        // At worst, try to do no harm?
        return song.nPatterns - 1;
    }
    return buf;
}

void XmTrackerPlayer::tick()
{
    if (++ticks >= tempo * (delay + 1)) {
        ticks = delay = 0;
        // load next notes into the process channels
        loadNextNotes();
        if (!isPlaying()) return;
    }

    // process effects and envelopes
    process();

    // update mixer
    commit();
}
