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
        bool valid = false;
        bool recovered = false;

        // Note recovery/validation
        if (note.note == XmTrackerPattern::kNoNote) {
            recovered = true;
            // No note, with a valid instrument somewhere, carry over the previous note.
            if (channel.note.instrument < song.nInstruments ||
                note.instrument < song.nInstruments)
            {
                note.note = channel.note.note;
                valid = true;
            }
        }

        // Instrument recovery/validation
        if (note.instrument == XmTrackerPattern::kNoInstrument) {
            recovered = true;
            // No instrument, with either an inactive note or a previously existing instrument.
            if (note.note == XmTrackerPattern::kNoteOff) {
                valid = true;
            }
            if (channel.note.instrument != XmTrackerPattern::kNoInstrument) {
                ASSERT(channel.instrument.sample.pData);
                note.instrument = channel.note.instrument;
                valid = true;
            }
        }

        if (!recovered) {
            valid = true;
        }

        if (valid) {
            if (channel.note.instrument != note.instrument) {
                // Change the instrument.
                // TODO: Consider mapping this instead of copying, if we can be guaranteed the whole struct.
                if (!SvmMemory::copyROData(channel.instrument, song.instruments + note.instrument * sizeof(_SYSXMInstrument))) {
                    ASSERT(false);
                }
            }
            channel.note = note;
            channel.restart = true;
            channel.period = getPeriod(channel.realNote(), channel.instrument.finetune);

            // Volume
            channel.volume = channel.instrument.sample.volume;
            if (note.volumeColumnByte >= 0x10 && note.volumeColumnByte <= 0x50)
                    channel.volume = note.volumeColumnByte - 0x10;
        } else {
            memset(&channel, 0, sizeof(channel));
            XmTrackerPattern::resetNote(channel.note);
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

void XmTrackerPlayer::commit()
{
    AudioMixer &mixer = AudioMixer::instance;

    for (unsigned i = 0; i < song.nChannels; i++) {
        struct XmTrackerChannel &channel = channels[i];
        if (!channel.instrument.sample.pData && mixer.isPlaying(CHANNEL_FOR(i))) {
            mixer.stop(CHANNEL_FOR(i));
            continue;
        }

        if (channel.restart) {
            channel.restart = false;
            if (!mixer.play(&channel.instrument.sample,
                            CHANNEL_FOR(i),
                            (_SYSAudioLoopType)channel.instrument.sample.loopType))
                continue;
        }

        // Volume
        int16_t volume = (channel.volume * 0xFF) >> 6;
        mixer.setVolume(CHANNEL_FOR(i), clamp((int)volume, 0, _SYS_AUDIO_MAX_VOLUME));

        // Sampling rate
        mixer.setSpeed(CHANNEL_FOR(i), getFrequency(channel.period));
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

    // update mixer
    commit();
}
