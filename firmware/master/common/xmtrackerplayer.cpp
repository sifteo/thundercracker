#include "xmtrackerplayer.h"
#include "audiomixer.h"
#include "flash.h"

#define LGPFX "XmTrackerPlayer: "
XmTrackerPlayer XmTrackerPlayer::instance;

// To be replaced someday by a channel allocator and array
#define CHANNEL_FOR(x) (_SYS_AUDIO_MAX_CHANNELS - ((x) + 1))

bool XmTrackerPlayer::play(const struct _SYSXMSong *pSong) {
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
    tempo = song.tempo;
    ticks = bpm;
    phrase = 0;
    row = 0;
    pattern.init(&song)->loadPattern(patternOrderTable(phrase));

    AudioMixer::instance.setTrackerCallbackInterval(2500000 / tempo);
    tick();

    return true;
}

void XmTrackerPlayer::stop() {
    song.nPatterns = 0;
    AudioMixer::instance.setTrackerCallbackInterval(0);
}

inline void XmTrackerPlayer::loadNextNotes()
{
    // Get and process the next row of notes
    struct XmTrackerNote note;
    for (unsigned i = 0; i < song.nChannels; i++) {
        pattern.getNote(row, i, note);

        // …
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

uint8_t XmTrackerPlayer::patternOrderTable(uint16_t order) {
    ASSERT(order < song.patternOrderTableSize);

    uint8_t buf;
    SvmMemory::VirtAddr va = song.patternOrderTable + order;
    if(!SvmMemory::copyROData(buf, va)) {
        LOG((LGPFX"Could not copy %p (length %lu)!\n",
             (void *)va, sizeof(buf)));
        ASSERT(false);

        // At worst, try to do no harm?
        return song.nPatterns - 1;
    }
    return buf;
}

void XmTrackerPlayer::tick()
{
    if (ticks++ >= bpm) {
        ticks = 0;

        // load next notes into the process channels
        loadNextNotes();
    }

    // process effects and envelopes

    // update mixer

}