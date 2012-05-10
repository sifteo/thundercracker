/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef XMTRACKERPLAYER_H_
#define XMTRACKERPLAYER_H_

#include <stddef.h>
#include "sifteo/abi.h"
#include "xmtrackerpattern.h"
#include "svmmemory.h"
#include "macros.h"

struct XmTrackerEnvelopeMemory {
    int16_t tick;
    uint8_t point;
    uint8_t value;
    bool done;
};

struct XmTrackerChannel {
    struct _SYSXMInstrument instrument;
    struct XmTrackerNote note;
    uint16_t volume;
    uint16_t fadeout;
    uint32_t period;
    uint32_t frequency;
    bool start;
    bool active;
    bool valid;

    struct XmTrackerEnvelopeMemory envelope;

    // Effect parameters
    struct {
        uint8_t portaUp;
        uint8_t portaDown;
        uint8_t tonePorta;
        uint8_t slide;
        uint8_t slideDown;
        uint8_t slideUp;
        uint8_t fineSlide;
        struct {
            uint32_t period;
        } porta;
        struct {
            uint8_t phase;
            uint8_t speed;
            uint8_t depth;
            uint8_t type;
        } vibrato;
    };

    inline uint8_t realNote(uint8_t pNote = XmTrackerPattern::kNoNote) const {
        if (pNote == XmTrackerPattern::kNoNote)
            pNote = note.note;

        if (!instrument.sample.pData ||
            pNote == XmTrackerPattern::kNoteOff)
        {
            return 0;
        }

        return instrument.relativeNoteNumber + pNote;
    }
};

class XmTrackerPlayer {
public:
    static XmTrackerPlayer instance;

    static void mixerCallback() {
        if (instance.song.nPatterns == 0) {
            /* This should never happen.
             * stop() sets nPatterns to 0 and kills the callback.
             */
            XmTrackerPlayer::instance.stop();
            return;
        }

        instance.tick();
    }

    bool play(const struct _SYSXMSong *pSong);
    bool isPlaying() const { return song.nPatterns > 0; }
    void stop();

    // TODO: future:
    // void muteChannel(uint8_t), unmuteChannel(uint8_t).

private:
    // Frequency and period computation
    static const uint8_t kLinearFrequencies = 0x01;
    static const uint8_t kAmigaFrequencies = 0x00;
    static const uint16_t AmigaPeriodTab[];
    static const uint32_t LinearFrequencyTab[];
    uint32_t getPeriod(uint16_t note, int8_t finetune) const;
    uint32_t getFrequency(uint32_t period) const;

    // Volume/Envelopes
    static const uint8_t kMaxVolume = 64;
    static const uint8_t kEnvelopeSustain = 1 << 1;
    static const uint8_t kEnvelopeLoop = 1 << 2;
    inline static const uint16_t envelopeOffset(uint16_t enc) {
        return enc & 0x01FF;
    }
    inline static const uint16_t envelopeValue(uint16_t enc) {
        return enc >> 9;
    }


    // channels
    struct XmTrackerChannel channels[_SYS_AUDIO_MAX_CHANNELS];
    // voice ptrs
    _SYSXMSong song;

    uint8_t ticks;

    /* The naming of bpm and tempo follows the (unofficial) XM module file
     * spec. It can be both dissatisfying and confusing, but is retained here
     * for consistency.
     */
    uint8_t bpm;    // Speed of piece (yes, tempo)
    uint8_t tempo;  // Ticks per note
    uint8_t delay;  // Delay added to end of pattern

    // Playback positions
    uint16_t phrase;          // The current index into the pattern order table
    XmTrackerPattern pattern; // The current pattern
    uint16_t row;     // Current row within pattern, above
    struct {
        bool force;
        uint16_t phrase;
        uint16_t row;
    } next;

    void loadNextNotes();

    void processVolumeSlideUp(XmTrackerChannel &channel, uint16_t &volume, uint8_t inc);
    void processVolumeSlideDown(XmTrackerChannel &channel, uint16_t &volume, uint8_t inc);
    void processVibrato(XmTrackerChannel &channel);
    void processPorta(XmTrackerChannel &channel);
    void processVolume(XmTrackerChannel &channel);

    void processArpeggio(XmTrackerChannel &channel);
    void processVolumeSlide(XmTrackerChannel &channel);
    void processPatternBreak(uint16_t nextPhrase, uint16_t row);
    void processEffects(XmTrackerChannel &channel);

    void processEnvelope(XmTrackerChannel &channel);

    void process();
    void commit();
    uint8_t patternOrderTable(uint16_t order);
    void tick();
};

#endif // XMTRACKERPLAYER_H_
