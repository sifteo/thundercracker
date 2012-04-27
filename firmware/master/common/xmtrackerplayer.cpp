/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "xmtrackerplayer.h"
#include "audiomixer.h"

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
    // Get and process the next row of notes
    struct XmTrackerNote note;
    for (unsigned i = 0; i < song.nChannels; i++) {
        struct XmTrackerChannel &channel = channels[i];
        pattern.getNote(row, i, note);
        channel.valid = false;
        bool recovered = false,
             recNote = false,
             recInst = false;

        if (note.note == XmTrackerPattern::kNoNote) {
            // Note recovery/validation
            recovered = true;
            // No note, with a valid instrument somewhere, carry over the previous note.
            if (note.instrument < song.nInstruments) {
                channel.valid = true;
            } else if (channel.note.instrument < song.nInstruments) {
                recNote = true;
                recInst = true;
                note.note = channel.note.note;
                note.instrument = channel.note.instrument;
                channel.valid = true;
            }
        } else if (note.instrument == XmTrackerPattern::kNoInstrument) {
            // Instrument recovery/validation
            recovered = true;
            // No instrument, with either an inactive note or a previously existing instrument.
            if (note.note == XmTrackerPattern::kNoteOff) {
                channel.valid = true;
            }
            if (channel.note.instrument < song.nInstruments) {
                recInst = true;
                note.instrument = channel.note.instrument;
                channel.valid = true;
            }
        }

        // Invalid notes should still let effects through.
        if (recovered && !channel.valid) {
            note.note = XmTrackerPattern::kNoteOff;
            note.instrument = XmTrackerPattern::kNoInstrument;
            channel.valid = true;
        } else if (!recovered) {
            channel.valid = true;
        }

        if (channel.valid) {
            if (channel.note.instrument != note.instrument && note.instrument < song.nInstruments) {
                // Change the instrument.
                // TODO: Consider mapping this instead of copying, if we can be guaranteed the whole struct.
                if (!SvmMemory::copyROData(channel.instrument, song.instruments + note.instrument * sizeof(_SYSXMInstrument))) {
                    ASSERT(false);
                }
            } else if (note.instrument >= song.nInstruments) {
                channel.instrument.sample.pData = 0;
            }
            channel.start = !recovered || !recNote || !recInst;
            if (channel.instrument.sample.pData == 0)
                channel.start = false;
            if (note.note == XmTrackerPattern::kNoteOff || note.note == XmTrackerPattern::kNoNote) {
                channel.start = false;
                channel.active = false;
            }
            channel.note = note;
            if (!channel.start) continue;

            channel.period = getPeriod(channel.realNote(), channel.instrument.finetune);

            // TODO: verify envelopes
            channel.envelope.done = channel.instrument.nVolumeEnvelopePoints == 0;

            // Volume
            memset(&channel.envelope, 0, sizeof(channel.envelope));
            channel.fadeout = UINT16_MAX;
            if (note.volumeColumnByte >= 0x10 && note.volumeColumnByte <= 0x50)
                    channel.volume = note.volumeColumnByte - 0x10;

            channel.active = true;
        }
    }
    pattern.releaseRef();

    // Advance song position
    row++;
    if (row >= pattern.nRows() || next.set) {
        if (!next.set) {
            processPatternBreak(song.patternOrderTableSize, 0);
            if (!isPlaying()) return;
        }
LOG(("was: %u, %u; next: %u, %u\n", phrase, row, next.phrase, next.row));

        if (phrase != next.phrase && next.phrase < song.patternOrderTableSize) {
            pattern.loadPattern(patternOrderTable(next.phrase));
            // TODO: clear active notes/effects here?
        } else if (next.phrase >= song.patternOrderTableSize) {
            stop();
        }
        phrase = next.phrase;

        if (row < pattern.nRows()) {
            row = next.row;
        } else {
            stop();
        }
        
        next.set = false;
    }
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

void XmTrackerPlayer::processVolumeSlideUp(uint16_t &volume, uint8_t inc)
{
    volume = MIN(volume + inc, kMaxVolume);
}

void XmTrackerPlayer::processVolumeSlideDown(uint16_t &volume, uint8_t dec)
{
    volume = (uint16_t)MAX((int32_t)volume - dec, 0);
}

void XmTrackerPlayer::processVolume(XmTrackerChannel &channel)
{
    uint8_t command = channel.note.volumeColumnByte & 0xF;
    uint8_t param = channel.note.volumeColumnByte >> 4;

    switch (command) {
        case vxSlideDown:
            LOG(("%s:%d: NOT_TESTED: vxSlideDown vx(0x%02x)\n", __FILE__, __LINE__, channel.note.volumeColumnByte));
            processPatternBreak(0, 0);
            processVolumeSlideDown(channel.volume, param);
            break;
        case vxSlideUp:
            LOG(("%s:%d: NOT_TESTED: vxSlideUp vx(0x%02x)\n", __FILE__, __LINE__, channel.note.volumeColumnByte));
            processPatternBreak(0, 0);
            processVolumeSlideUp(channel.volume, param);
            break;
        case vxFineVolumeDown:
            LOG(("%s:%d: NOT_TESTED: vxFineVolumeDown vx(0x%02x)\n", __FILE__, __LINE__, channel.note.volumeColumnByte));
            processPatternBreak(0, 0);
            if (ticks) break;
            processVolumeSlideDown(channel.volume, param);
            break;
        case vxFineVolumeUp:
            LOG(("%s:%d: NOT_TESTED: vxFineVolumeUp vx(0x%02x)\n", __FILE__, __LINE__, channel.note.volumeColumnByte));
            processPatternBreak(0, 0);
            if (ticks) break;
            processVolumeSlideUp(channel.volume, param);
            break;
        case vxVibratoSpeed:
            LOG(("%s:%d: NOT_TESTED: vxVibratoSpeed vx(0x%02x)\n", __FILE__, __LINE__, channel.note.volumeColumnByte));
            processPatternBreak(0, 0);
            break;
        case vxVibratoDepth:
            LOG(("%s:%d: NOT_TESTED: vxVibratoDepth vx(0x%02x)\n", __FILE__, __LINE__, channel.note.volumeColumnByte));
            processPatternBreak(0, 0);
            break;
        case vxTonePortamento:
            LOG(("%s:%d: NOT_TESTED: vxTonePortamento vx(0x%02x)\n", __FILE__, __LINE__, channel.note.volumeColumnByte));
            processPatternBreak(0, 0);
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
    fxSetEnvelopePos             = 0x15, // L
    fxPanSlide                   = 0x19, // P
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

void XmTrackerPlayer::processVolumeSlide(XmTrackerChannel &channel)
{
    if (ticks == 0 || channel.note.effectParam == 0) return;
    uint8_t up = channel.note.effectParam >> 4;
    uint8_t down = channel.note.effectParam & 0x0F;

    if (up)
        processVolumeSlideUp(channel.volume, up);
    else if (down)
        processVolumeSlideDown(channel.volume, down);
}

void XmTrackerPlayer::processPatternBreak(uint16_t nextPhrase, uint16_t nextRow)
{
    next.set = true;

    // Adjust phrase as appropriate
    if (nextPhrase >= song.patternOrderTableSize) next.phrase = phrase + 1;
    else next.phrase = nextPhrase;

    // This is bounds-checked later, when the pattern is loaded.
    next.row = nextRow;

    // Reached end of song
    if (nextPhrase >= song.patternOrderTableSize) {
        // This is bounds-checked later, when the break is applied.
        phrase = song.restartPosition;
    }
}

void XmTrackerPlayer::processEffects(XmTrackerChannel &channel)
{
    const uint8_t param = channel.note.effectParam;
    switch (channel.note.effectType) {
        case fxArpeggio:
            LOG(("%s:%d: NOT_TESTED: fxArpeggio fx(0x%02x)\n", __FILE__, __LINE__, channel.note.effectType));
            processPatternBreak(0, 0);
            break;
        case fxPortaUp:
            if (param == 0) LOG(("%s:%d: NOT_TESTED: empty param to fxPortaUp\n", __FILE__, __LINE__));
            if (ticks == 0) break;
            channel.period -= param; // TODO: bounds checking?
            break;
        case fxPortaDown:
            if (param == 0) LOG(("%s:%d: NOT_TESTED: empty param to fxPortaDown\n", __FILE__, __LINE__));
            if (ticks == 0) break;
            channel.period += param; // TODO: bounds checking?
            break;
        case fxTonePorta:
            LOG(("%s:%d: NOT_TESTED: fxTonePorta fx(0x%02x)\n", __FILE__, __LINE__, channel.note.effectType));
            processPatternBreak(0, 0);
            break;
        case fxVibrato:
            LOG(("%s:%d: NOT_TESTED: fxVibrato fx(0x%02x)\n", __FILE__, __LINE__, channel.note.effectType));
            processPatternBreak(0, 0);
            break;
        case fxTonePortaAndVolumeSlide:
            LOG(("%s:%d: NOT_TESTED: fxTonePortaAndVolumeSlide fx(0x%02x)\n", __FILE__, __LINE__, channel.note.effectType));
            processPatternBreak(0, 0);
            break;
        case fxVibratoAndVolumeSlide:
            LOG(("%s:%d: NOT_TESTED: fxVibratoAndVolumeSlide fx(0x%02x)\n", __FILE__, __LINE__, channel.note.effectType));
            processPatternBreak(0, 0);
            break;
        case fxTremolo:
            LOG(("%s:%d: NOT_TESTED: fxTremolo fx(0x%02x)\n", __FILE__, __LINE__, channel.note.effectType));
            processPatternBreak(0, 0);
            break;
        case fxSampleOffset:
            LOG(("%s:%d: NOT_TESTED: fxSampleOffset fx(0x%02x)\n", __FILE__, __LINE__, channel.note.effectType));
            processPatternBreak(0, 0);
            break;
        case fxVolumeSlide:
            if (param == 0) LOG(("%s:%d: NOT_TESTED: empty param to fxVolumeSlide\n", __FILE__, __LINE__));
            processVolumeSlide(channel);
            break;
        case fxPositionJump:
            LOG(("%s:%d: NOT_TESTED: fxPositionJump fx(0x%02x)\n", __FILE__, __LINE__, channel.note.effectType));
            processPatternBreak(0, 0);
            break;
        case fxSetVolume:
            channel.volume = MIN(param, kMaxVolume);
            break;
        case fxPatternBreak:
            // Seriously, the spec says the higher order nibble is * 10, not * 16.
            processPatternBreak(-1, param & 0xF + (param >> 4) * 10);
            break;
        case fxSetTempoAndBPM:
            ASSERT(ticks == 0);
            // Only useful at the start of a note
            channel.note.effectType = XmTrackerPattern::kNoEffect;
            if (param == 0) break;

            if (param < 0x1F)
                tempo = param;
            else
                bpm = param;
            break;
        case fxSetGlobalVolume:
            LOG(("%s:%d: NOT_TESTED: fxSetGlobalVolume fx(0x%02x)\n", __FILE__, __LINE__, channel.note.effectType));
            processPatternBreak(0, 0);
            break;
        case fxGlobalVolumeSlide:
            LOG(("%s:%d: NOT_TESTED: fxGlobalVolumeSlide fx(0x%02x)\n", __FILE__, __LINE__, channel.note.effectType));
            processPatternBreak(0, 0);
            break;
        case fxSetEnvelopePos:
            LOG(("%s:%d: NOT_TESTED: fxSetEnvelopePos fx(0x%02x)\n", __FILE__, __LINE__, channel.note.effectType));
            processPatternBreak(0, 0);
            break;
        case fxPanSlide:
            LOG(("%s:%d: NOT_TESTED: fxPanSlide fx(0x%02x)\n", __FILE__, __LINE__, channel.note.effectType));
            processPatternBreak(0, 0);
            break;
        case fxMultiRetrigNote:
            LOG(("%s:%d: NOT_TESTED: fxMultiRetrigNote fx(0x%02x)\n", __FILE__, __LINE__, channel.note.effectType));
            processPatternBreak(0, 0);
            break;
        case fxTremor:
            LOG(("%s:%d: NOT_TESTED: fxTremor fx(0x%02x)\n", __FILE__, __LINE__, channel.note.effectType));
            processPatternBreak(0, 0);
            break;
        case fxExtraFinePorta:
            LOG(("%s:%d: NOT_TESTED: fxExtraFinePorta fx(0x%02x)\n", __FILE__, __LINE__, channel.note.effectType));
            processPatternBreak(0, 0);
            break;
        case fxOverflow:
            switch (param >> 4) {
                case fxFinePortaUp:
                    LOG(("%s:%d: NOT_TESTED: fxFinePortaUp fx(0x%02x, 0x%02x)\n", __FILE__, __LINE__, channel.note.effectType, param));
                    processPatternBreak(0, 0);
                    break;
                case fxFinePortaDown:
                    LOG(("%s:%d: NOT_TESTED: fxFinePortaDown fx(0x%02x, 0x%02x)\n", __FILE__, __LINE__, channel.note.effectType, param));
                    processPatternBreak(0, 0);
                    break;
                case fxGlissControl:
                    LOG(("%s:%d: NOT_TESTED: fxGlissControl fx(0x%02x, 0x%02x)\n", __FILE__, __LINE__, channel.note.effectType, param));
                    processPatternBreak(0, 0);
                    break;
                case fxVibratoControl:
                    LOG(("%s:%d: NOT_TESTED: fxVibratoControl fx(0x%02x, 0x%02x)\n", __FILE__, __LINE__, channel.note.effectType, param));
                    processPatternBreak(0, 0);
                    break;
                case fxFinetune:
                    LOG(("%s:%d: NOT_TESTED: fxFinetune fx(0x%02x, 0x%02x)\n", __FILE__, __LINE__, channel.note.effectType, param));
                    processPatternBreak(0, 0);
                    break;
                case fxLoopPattern:
                    LOG(("%s:%d: NOT_TESTED: fxLoopPattern fx(0x%02x, 0x%02x)\n", __FILE__, __LINE__, channel.note.effectType, param));
                    processPatternBreak(0, 0);
                    break;
                case fxTremoloControl:
                    LOG(("%s:%d: NOT_TESTED: fxTremoloControl fx(0x%02x, 0x%02x)\n", __FILE__, __LINE__, channel.note.effectType, param));
                    processPatternBreak(0, 0);
                    break;
                case fxRetrigNote:
                    LOG(("%s:%d: NOT_TESTED: fxRetrigNote fx(0x%02x, 0x%02x)\n", __FILE__, __LINE__, channel.note.effectType, param));
                    processPatternBreak(0, 0);
                    break;
                case fxFineVolumeSlideUp:
                    if (param & 0x0F == 0) LOG(("%s:%d: NOT_TESTED: empty param to fxFineVolumeSlideUp\n", __FILE__, __LINE__));
                    // Apply once, at beginning of note
                    if (ticks) break;
                    processVolumeSlideUp(channel.volume, (param & 0xF));
                    break;
                case fxFineVolumeSlideDown:
                    LOG(("%s:%d: NOT_TESTED: fxFineVolumeSlideDown fx(0x%02x, 0x%02x)\n", __FILE__, __LINE__, channel.note.effectType, param));
                    if (param & 0x0F == 0) LOG(("%s:%d: NOT_TESTED: empty param to fxFineVolumeSlideDown\n", __FILE__, __LINE__));
                    // Apply once, at beginning of note
                    if (ticks) break;
                    processVolumeSlideDown(channel.volume, (param & 0xF));
                    break;
                case fxNoteCut:
                    LOG(("%s:%d: NOT_TESTED: fxNoteCut fx(0x%02x, 0x%02x)\n", __FILE__, __LINE__, channel.note.effectType, param));
                    processPatternBreak(0, 0);
                    break;
                case fxNoteDelay:
                    LOG(("%s:%d: NOT_TESTED: fxNoteDelay fx(0x%02x, 0x%02x)\n", __FILE__, __LINE__, channel.note.effectType, param));
                    processPatternBreak(0, 0);
                    break;
                case fxPatternDelay:
                    delay = param & 0x0f;
                    break;
            }
            break;
        default:
            LOG(("%s:%d: NOT_IMPLEMENTED: fx(0x%02x, 0x%02x)\n", __FILE__, __LINE__, channel.note.effectType, channel.note.effectParam));
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

    if (instrument.volumeType & kEnvelopeSustain && channel.note.note != XmTrackerPattern::kNoteOff) {
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

    if (envelope.point >= instrument.nVolumeEnvelopePoints - 1 ||
        envelope.tick == 0)
    {
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

        if (channel.instrument.volumeType) {
            volume = (volume * channel.envelope.value) >> 6;

            if (channel.note.note == XmTrackerPattern::kNoteOff) {
                // Update fade
                if (channel.instrument.volumeFadeout > 0xFFF ||
                    channel.instrument.volumeFadeout > channel.fadeout ||
                    channel.instrument.volumeFadeout == 0)
                {
                    channel.fadeout = 0;
                } else {
                    channel.fadeout -= channel.instrument.volumeFadeout;
                }

                // Completely faded -> stop playing sample.
                if (channel.fadeout == 0) {
                    channel.active = false;
                }

                LOG(("%s:%d: NOT_TESTED: fadeout\n", __FILE__, __LINE__));

                // Apply fadeout
                volume = volume * channel.fadeout / UINT16_MAX;
            }
        }
        volume *= 4;
        mixer.setVolume(CHANNEL_FOR(i), clamp(volume, 0, _SYS_AUDIO_MAX_VOLUME));

        // Sampling rate
        if (channel.period > 0) {
            mixer.setSpeed(CHANNEL_FOR(i), getFrequency(channel.period));
        } else {
            mixer.stop(CHANNEL_FOR(i));
        }
    }

    // Future effects are capable of adjusting the tempo/bpm
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
    if (ticks++ >= tempo + delay) {
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
