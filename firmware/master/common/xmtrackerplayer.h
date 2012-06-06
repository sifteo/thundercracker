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
    uint16_t userVolume;
    uint16_t volume;
    uint16_t fadeout;
    uint32_t period;
    uint32_t frequency;
    uint32_t offset;
    uint8_t state;
    uint8_t applyStateOnTick;

    struct XmTrackerEnvelopeMemory envelope;

    // Effect parameters
    struct {
        uint8_t portaUp;
        uint8_t portaDown;
        uint8_t tonePorta;
        uint8_t slide;
        uint8_t slideDown;
        uint8_t slideUp;
        uint8_t fineSlideDown:4,
                fineSlideUp:4;
        struct {
            uint32_t period;
        } porta;
        struct {
            uint8_t phase;
            uint8_t speed:4,
                    depth:4;
            uint8_t type;
        } vibrato;
        struct {
            uint8_t speed:4,
                    interval:4;
            uint16_t phase;
        } retrigger;
        struct {
            uint8_t phase;
            uint8_t speed:4,
                    depth:4;
            uint16_t volume;
        } tremolo;
        struct {
            uint8_t on:4,
                    off:4;
            uint8_t phase;
        } tremor;
    };

    inline uint8_t realNote(uint8_t pNote = 0) const {

        // Implied argument
        if (pNote == 0)
            pNote = note.note;

        if (pNote >= XmTrackerPattern::kNoteOff)
            return 0;

        pNote += instrument.relativeNoteNumber;

        if (!instrument.sample.pData ||
            pNote > XmTrackerPattern::kMaxNote)
        {
            return 0;
        }

        return pNote;
    }
};

class XmTrackerPlayer {
public:
    static XmTrackerPlayer instance;

    static void mixerCallback() {
        if (instance.isStopped()) {
            /* This should never happen. Call stop() (again?) to ensure a consistent playback state.
             * stop() sets nPatterns to 0 and kills the callback.
             */
            instance.stop();
            return;
        }

        instance.tick();
    }

    XmTrackerPlayer() : hasSong(0), paused(0) { memset(&song, 0, sizeof song); }
    void init();
    bool play(const struct _SYSXMSong *pSong);
    bool isPaused() { return paused; }
    bool isStopped() { return !hasSong; }
    void stop();
    void setVolume(int volume, uint8_t ch);
    void pause();

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
    uint16_t volume;
    uint16_t userVolume;
    uint8_t ticks;
    bool hasSong;
    bool paused;

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
    struct {
        bool force;
        uint16_t phrase;
        uint16_t row;
    } next;
    struct {
        uint16_t start;
        uint16_t end;
        uint8_t limit:4,
                i:4;
    } loop;

    void loadNextNotes();

    void incrementVolume(uint16_t &volume, uint8_t inc);
    void decrementVolume(uint16_t &volume, uint8_t dec);
    void processVolumeSlideUp(XmTrackerChannel &channel, uint16_t &volume, uint8_t inc);
    void processVolumeSlideDown(XmTrackerChannel &channel, uint16_t &volume, uint8_t inc);
    void processVibrato(XmTrackerChannel &channel);
    void processPorta(XmTrackerChannel &channel);
    void processVolume(XmTrackerChannel &channel);

    void processArpeggio(XmTrackerChannel &channel);
    void processVolumeSlide(uint16_t &volume, uint8_t param);
    void processTremolo(XmTrackerChannel &channel);
    void processPatternBreak(uint16_t nextPhrase, uint16_t row);
    void processRetrigger(XmTrackerChannel &channel, uint8_t interval, uint8_t slide = 8);
    bool processTremor(XmTrackerChannel &channel);
    void processEffects(XmTrackerChannel &channel);

    void processEnvelope(XmTrackerChannel &channel);

    void process();
    void commit();
    uint8_t patternOrderTable(uint16_t order);
    void tick();
};

#endif // XMTRACKERPLAYER_H_
