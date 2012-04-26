/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "xmtrackerplayer.h"
#include "audiomixer.h"

#include "xmtrackertables.h"

#define LGPFX "XmTrackerPlayer: "
XmTrackerPlayer XmTrackerPlayer::instance;

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
        bool recovered = false;

        if ((note.note != XmTrackerPattern::kNoNote ||
             note.instrument != XmTrackerPattern::kNoInstrument) &&
            note.note != XmTrackerPattern::kNoteOff)
        {
            // Note recovery/validation
            if (note.note == XmTrackerPattern::kNoNote) {
                recovered = true;
                // No note, with a valid instrument somewhere, carry over the previous note.
                if (channel.note.instrument < song.nInstruments ||
                    note.instrument < song.nInstruments)
                {
                    note.note = channel.note.note;
                    channel.valid = true;
                }
            }

            // Instrument recovery/validation
            if (note.instrument == XmTrackerPattern::kNoInstrument) {
                recovered = true;
                // No instrument, with either an inactive note or a previously existing instrument.
                if (note.note == XmTrackerPattern::kNoteOff) {
                    channel.valid = true;
                }
                if (channel.note.instrument < song.nInstruments) {
                    note.instrument = channel.note.instrument;
                    channel.valid = true;
                }
            }
        }

        if (!recovered) {
            channel.valid = true;
        }

        if (channel.valid) {
            if (channel.note.instrument != note.instrument && note.instrument < song.nInstruments) {
                // Change the instrument.
                // TODO: Consider mapping this instead of copying, if we can be guaranteed the whole struct.
                if (!SvmMemory::copyROData(channel.instrument, song.instruments + note.instrument * sizeof(_SYSXMInstrument))) {
                    ASSERT(false);
                }
            }
            channel.note = note;
            channel.restart = channel.instrument.sample.pData != 0;
            channel.period = getPeriod(channel.realNote(), channel.instrument.finetune);

            // TODO
            channel.envelope.done = channel.instrument.nVolumeEnvelopePoints == 0;

            // Volume
            memset(&channel.envelope, 0, sizeof(channel.envelope));
            channel.fadeout = UINT16_MAX;
            channel.volume = channel.instrument.sample.volume;
            if (note.volumeColumnByte >= 0x10 && note.volumeColumnByte <= 0x50)
                    channel.volume = note.volumeColumnByte - 0x10;

            channel.active = true;
        }
    }
    pattern.releaseRef();

    // Advance song position
    row++;
    if (row >= pattern.nRows()) {
        // Reached end of pattern
        row = 0;
        phrase++;

        // Reached end of song
        if (phrase >= song.patternOrderTableSize) {
            if (song.restartPosition < song.nPatterns) {
                phrase = song.restartPosition;
            } else {
                stop();
                return;
            }
        }

        pattern.loadPattern(patternOrderTable(phrase));
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
void XmTrackerPlayer::processVolume(XmTrackerChannel &channel)
{
    uint8_t command = channel.note.volumeColumnByte >> 4;
    uint8_t param = channel.note.volumeColumnByte & 0xF;

    switch (command) {
        case vxSlideDown:
            LOG(("%s:%d: NOT_TESTED: vxSlideDown vx(0x%02x)\n", __FILE__, __LINE__, channel.note.volumeColumnByte));
            break;
        case vxSlideUp:
            LOG(("%s:%d: NOT_TESTED: vxSlideUp vx(0x%02x)\n", __FILE__, __LINE__, channel.note.volumeColumnByte));
            break;
        case vxFineVolumeDown:
            LOG(("%s:%d: NOT_TESTED: vxFineVolumeDown vx(0x%02x)\n", __FILE__, __LINE__, channel.note.volumeColumnByte));
            break;
        case vxFineVolumeUp:
            LOG(("%s:%d: NOT_TESTED: vxFineVolumeUp vx(0x%02x)\n", __FILE__, __LINE__, channel.note.volumeColumnByte));
            break;
        case vxVibratoSpeed:
            LOG(("%s:%d: NOT_TESTED: vxVibratoSpeed vx(0x%02x)\n", __FILE__, __LINE__, channel.note.volumeColumnByte));
            break;
        case vxVibratoDepth:
            LOG(("%s:%d: NOT_TESTED: vxVibratoDepth vx(0x%02x)\n", __FILE__, __LINE__, channel.note.volumeColumnByte));
            break;
        case vxTonePortamento:
            LOG(("%s:%d: NOT_TESTED: vxTonePortamento vx(0x%02x)\n", __FILE__, __LINE__, channel.note.volumeColumnByte));
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
void XmTrackerPlayer::processEffects(XmTrackerChannel &channel)
{
    switch (channel.note.effectType) {
        case fxArpeggio:
            LOG(("%s:%d: NOT_TESTED: fxArpeggio fx(0x%02x)\n", __FILE__, __LINE__, channel.note.effectType));
            break;
        case fxPortaUp:
            LOG(("%s:%d: NOT_TESTED: fxPortaUp fx(0x%02x)\n", __FILE__, __LINE__, channel.note.effectType));
            break;
        case fxPortaDown:
            LOG(("%s:%d: NOT_TESTED: fxPortaDown fx(0x%02x)\n", __FILE__, __LINE__, channel.note.effectType));
            break;
        case fxTonePorta:
            LOG(("%s:%d: NOT_TESTED: fxTonePorta fx(0x%02x)\n", __FILE__, __LINE__, channel.note.effectType));
            break;
        case fxVibrato:
            LOG(("%s:%d: NOT_TESTED: fxVibrato fx(0x%02x)\n", __FILE__, __LINE__, channel.note.effectType));
            break;
        case fxTonePortaAndVolumeSlide:
            LOG(("%s:%d: NOT_TESTED: fxTonePortaAndVolumeSlide fx(0x%02x)\n", __FILE__, __LINE__, channel.note.effectType));
            break;
        case fxVibratoAndVolumeSlide:
            LOG(("%s:%d: NOT_TESTED: fxVibratoAndVolumeSlide fx(0x%02x)\n", __FILE__, __LINE__, channel.note.effectType));
            break;
        case fxTremolo:
            LOG(("%s:%d: NOT_TESTED: fxTremolo fx(0x%02x)\n", __FILE__, __LINE__, channel.note.effectType));
            break;
        case fxSampleOffset:
            LOG(("%s:%d: NOT_TESTED: fxSampleOffset fx(0x%02x)\n", __FILE__, __LINE__, channel.note.effectType));
            break;
        case fxVolumeSlide:
            LOG(("%s:%d: NOT_TESTED: fxVolumeSlide fx(0x%02x)\n", __FILE__, __LINE__, channel.note.effectType));
            break;
        case fxPositionJump:
            LOG(("%s:%d: NOT_TESTED: fxPositionJump fx(0x%02x)\n", __FILE__, __LINE__, channel.note.effectType));
            break;
        case fxSetVolume:
            channel.volume = MIN(channel.note.effectParam, kMaxVolume);
            break;
        case fxPatternBreak:
            LOG(("%s:%d: NOT_TESTED: fxPatternBreak fx(0x%02x)\n", __FILE__, __LINE__, channel.note.effectType));
            break;
        case fxSetTempoAndBPM:
            // Only useful at the start of a note
            ASSERT(ticks == 0);
            channel.note.effectType = XmTrackerPattern::kNoEffect;
            if (channel.note.effectParam == 0) break;

            if (channel.note.effectParam < 0x1F)
                tempo = channel.note.effectParam;
            else
                bpm = channel.note.effectParam;
            break;
        case fxSetGlobalVolume:
            LOG(("%s:%d: NOT_TESTED: fxSetGlobalVolume fx(0x%02x)\n", __FILE__, __LINE__, channel.note.effectType));
            break;
        case fxGlobalVolumeSlide:
            LOG(("%s:%d: NOT_TESTED: fxGlobalVolumeSlide fx(0x%02x)\n", __FILE__, __LINE__, channel.note.effectType));
            break;
        case fxSetEnvelopePos:
            LOG(("%s:%d: NOT_TESTED: fxSetEnvelopePos fx(0x%02x)\n", __FILE__, __LINE__, channel.note.effectType));
            break;
        case fxPanSlide:
            LOG(("%s:%d: NOT_TESTED: fxPanSlide fx(0x%02x)\n", __FILE__, __LINE__, channel.note.effectType));
            break;
        case fxMultiRetrigNote:
            LOG(("%s:%d: NOT_TESTED: fxMultiRetrigNote fx(0x%02x)\n", __FILE__, __LINE__, channel.note.effectType));
            break;
        case fxTremor:
            LOG(("%s:%d: NOT_TESTED: fxTremor fx(0x%02x)\n", __FILE__, __LINE__, channel.note.effectType));
            break;
        case fxExtraFinePorta:
            LOG(("%s:%d: NOT_TESTED: fxExtraFinePorta fx(0x%02x)\n", __FILE__, __LINE__, channel.note.effectType));
            break;
        case fxOverflow:
            switch (channel.note.effectParam >> 4) {
                case fxFinePortaUp:
                    LOG(("%s:%d: NOT_TESTED: fxFinePortaUp fx(0x%02x, 0x%02x)\n", __FILE__, __LINE__, channel.note.effectType, channel.note.effectParam));
                    break;
                case fxFinePortaDown:
                    LOG(("%s:%d: NOT_TESTED: fxFinePortaDown fx(0x%02x, 0x%02x)\n", __FILE__, __LINE__, channel.note.effectType, channel.note.effectParam));
                    break;
                case fxGlissControl:
                    LOG(("%s:%d: NOT_TESTED: fxGlissControl fx(0x%02x, 0x%02x)\n", __FILE__, __LINE__, channel.note.effectType, channel.note.effectParam));
                    break;
                case fxVibratoControl:
                    LOG(("%s:%d: NOT_TESTED: fxVibratoControl fx(0x%02x, 0x%02x)\n", __FILE__, __LINE__, channel.note.effectType, channel.note.effectParam));
                    break;
                case fxFinetune:
                    LOG(("%s:%d: NOT_TESTED: fxFinetune fx(0x%02x, 0x%02x)\n", __FILE__, __LINE__, channel.note.effectType, channel.note.effectParam));
                    break;
                case fxLoopPattern:
                    LOG(("%s:%d: NOT_TESTED: fxLoopPattern fx(0x%02x, 0x%02x)\n", __FILE__, __LINE__, channel.note.effectType, channel.note.effectParam));
                    break;
                case fxTremoloControl:
                    LOG(("%s:%d: NOT_TESTED: fxTremoloControl fx(0x%02x, 0x%02x)\n", __FILE__, __LINE__, channel.note.effectType, channel.note.effectParam));
                    break;
                case fxRetrigNote:
                    LOG(("%s:%d: NOT_TESTED: fxRetrigNote fx(0x%02x, 0x%02x)\n", __FILE__, __LINE__, channel.note.effectType, channel.note.effectParam));
                    break;
                case fxFineVolumeSlideUp:
                    channel.volume = MIN((uint32_t)channel.volume + (channel.note.effectParam & 0xF), kMaxVolume);
                    break;
                case fxFineVolumeSlideDown:
                    channel.volume = (uint16_t)MAX((int32_t)channel.volume - (channel.note.effectParam & 0xF), 0);
                    break;
                case fxNoteCut:
                    LOG(("%s:%d: NOT_TESTED: fxNoteCut fx(0x%02x, 0x%02x)\n", __FILE__, __LINE__, channel.note.effectType, channel.note.effectParam));
                    break;
                case fxNoteDelay:
                    LOG(("%s:%d: NOT_TESTED: fxNoteDelay fx(0x%02x, 0x%02x)\n", __FILE__, __LINE__, channel.note.effectType, channel.note.effectParam));
                    break;
                case fxPatternDelay:
                    LOG(("%s:%d: NOT_TESTED: fxPatternDelay fx(0x%02x, 0x%02x)\n", __FILE__, __LINE__, channel.note.effectType, channel.note.effectParam));
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

        if (channel.restart) {
            channel.restart = false;
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
        int32_t volume = (channel.volume * 0xFF) >> 6;

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
    if (ticks++ >= song.tempo) {
        ticks = 0;

        // load next notes into the process channels
        loadNextNotes();
    }

    // process effects and envelopes
    process();

    // update mixer
    commit();
}
