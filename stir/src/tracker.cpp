/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * STIR -- Sifteo Tracker Instance Reducer
 * Scott Perry <sifteo@numist.net>
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "tracker.h"
#include "audioencoder.h"
#include <stdlib.h>
#include <string.h>
#include "script.h"

namespace Stir {

// For logger output
const char * XmTrackerLoader::encodings[3] = {"", "Uncompressed PCM", "ADPCM"};

std::vector<std::vector<uint8_t> > XmTrackerLoader::globalSampleDatas;

/*
 * Load and process an entire XM file.
 * 
 * The same instance can be reused for multiple files, just call load() again.
 *
 * Returns success or failure.
 */
bool XmTrackerLoader::load(const char *pFilename, Logger &pLog)
{
    filename = pFilename;
    log = &pLog;

    // If this instance already contains module data, clean it up first.
    if (patterns.size()) init();

    f = fopen(filename, "rb");
    if (f == 0) {
        log->error("Could not open %s", filename);
        return false;
    }

    if (!readSong()) return init();

    fseek(f, 0, SEEK_END);
    fileSize = ftell(f);
    fclose(f);
    f = 0;

    return true;
}

/*
 */
void XmTrackerLoader::deduplicate(std::set<Tracker*> trackers, Logger &log)
{
    log.taskBegin("Deduplicating samples");
    unsigned dups = 0, savings = 0;
    log.taskProgress("%u duplicates found", dups);
    for (unsigned i = 0; i < globalSampleDatas.size() - 1; i++) {
        const std::vector<uint8_t> &a = globalSampleDatas[i];

        // We already deduped this sample. Good job.
        if (!a.size()) continue;

        for (unsigned j = i + 1; j < globalSampleDatas.size(); j++) {
            std::vector<uint8_t> &b = globalSampleDatas[j];
            if (a.size() == b.size() && !memcmp(&a[0], &b[0], a.size())) {
                savings += a.size();
                log.taskProgress("%u duplicates found (saved %5.02f kiB)", ++dups, savings / 1024.0f);

                // Find the module->instrument using this sample and redirect it from j to i
                for (std::set<Tracker*>::iterator t = trackers.begin(); t != trackers.end(); t++) {
                    Tracker *tracker = *t;
                    for (unsigned k = 0; k < tracker->loader.instruments.size(); k++) {
                        _SYSXMInstrument &instrument = tracker->loader.instruments[k];
                        if (instrument.sample.pData == j) instrument.sample.pData = i;
                    }
                }

                // Wipe out the sample, but do not remove it from the sample data list.
                b.clear();
            }
        }
    }
    log.taskEnd();
}

/*
 * Read song data from the module.
 *
 * This method makes calls to read patterns and instruments as appropriate.
 */
bool XmTrackerLoader::readSong()
{
    // FILE: Skip ID text (17), mod name (20), escape (1), tracker name (20), version (2).
    seek(17 + 20 + 1 + 20 + 2);
    uint32_t headerSize = get32();

    // FILE: Song metadata
    song.patternOrderTableSize = get16();
    song.restartPosition = get16();
    song.nChannels = get16();
    if (song.nChannels > _SYS_AUDIO_MAX_CHANNELS) {
        log->error("%s: Song contains %u channels, max %u supported",
                   filename, song.nChannels, _SYS_AUDIO_MAX_CHANNELS);
        return false;
    }

    song.nPatterns = get16();
    song.nInstruments = get16();
    song.frequencyTable = get16();
    song.tempo = get16();
    song.bpm = get16();

    // FILE: Pattern order table
    for (unsigned i = 0; i < song.patternOrderTableSize; i++) {
        patternTable.push_back(get8());
    }

    aseek(17 + 20 + 1 + 20 + 2 + headerSize); // This format is kinda stupid!

    // Load patterns
    for (unsigned i = 0; i < song.nPatterns; i++)
        if (!readNextPattern()) return false;

    /* We compress both envelopes and samples in readNextInstrument and its
     * callees, but they only print envelope compression statistics.
     */
    for (unsigned i = 0; i < song.nInstruments; i++) {
        if (!readNextInstrument()) return false;
    }

    // Verification:
    assert(patternDatas.size() == song.nPatterns);
    assert(patterns.size() == song.nPatterns);
    assert(patternDatas.size() == song.nPatterns);
    assert(instruments.size() == song.nInstruments);

    size += sizeof(song);
    return true;
}

/*
 * Read a pattern from the module.
 *
 * Get bytes from the file, put them in the appropriate data structures.
 */
bool XmTrackerLoader::readNextPattern()
{
    uint32_t offset = pos();
    uint32_t headerLength = get32();

    _SYSXMPattern pattern;
    seek(sizeof(uint8_t)); // packing type, unused
    pattern.nRows = get16();
    pattern.dataSize = get16();
    pattern.pData = 0;

    // Get pattern data
    aseek(offset + headerLength);
    std::vector<uint8_t> patternData(pattern.dataSize);
    getbuf(&patternData[0], pattern.dataSize);

    size += sizeof(pattern);
    patterns.push_back(pattern);
    size += patternData.size();
    patternDatas.push_back(patternData);

    return true;
}

/*
 * Read an instrument's data from the module.
 *
 * In the process, this function also computes loop start and end positions in
 * samples, compresses the volume envelope, 
 */
bool XmTrackerLoader::readNextInstrument()
{
    _SYSXMInstrument instrument;
    memset(&instrument, 0, sizeof(instrument));
    _SYSAudioModule &sample = instrument.sample;

    uint32_t offset = pos();
    uint32_t instrumentLength = get32();

    // FILE: Instrument name and type
    seek(22 + 1);

    // FILE: Number of samples
    uint16_t nSamples = get16();
    if (nSamples == 0) {
        log->error("%s, instrument %u: Instruments with no samples have not been tested",
                   filename, instruments.size());

        // Instruments with no samples have no envelopes to read.
        instrument.volumeEnvelopePoints = -1;

        /* There is nothing more to read, padding aside, the next thing in the
         * file is the next instrument.
         */
        size += sizeof(instrument);
        instruments.push_back(instrument);
        aseek(offset + instrumentLength);
        return true;
    }
    if (nSamples > 1) {
        log->error("%s, instrument %u has %u samples, discarding all but sample 0.", filename, instruments.size(), nSamples);
        log->error("Warning: playback of %s may differ significantly from reference!", filename);
    }
    nSamples--;

    // FILE: Sample header size (redundant), keymap assignments (redundant if only one sample)
    seek(4 + 96);

    /* XXX: XM format has envelope size *after* the points themselves.
     * Fast-forward to get the table size so we're not emitting spurious
     * warnings.
     */
    uint32_t vEnvelopePos = pos();
    seek(48 + 48); // volume envelope & panning envelope
    // FILE: Number of points in volume envelope
    instrument.nVolumeEnvelopePoints = get8();
    // And seek back to the volume envelope.
    aseek(vEnvelopePos);

    // FILE: Volume envelope (48 bytes(read from file) — 12(points) * sizeof(uint16_t)(compression) * 2(offset and value))
    uint16_t vEnvelope[12];
    for (int i = 0; i < 12; i++) {
        uint16_t eOffset = get16();
        uint16_t eValue = get16();

        if (i < instrument.nVolumeEnvelopePoints) {
            if (eOffset != (eOffset & 0x01FF)) { // 9 bits
                log->error("%s, instrument %u, envelope point %d: clamping offset: 0x%hu to 0x%hu\n",
                           filename, instruments.size(), i, eOffset, 0x1FF);
                eOffset = 0x1FF;
            }
            if (eValue != (eValue & 0x7F)) { // 7 bits
                log->error("%s, instrument %u, envelope point %d: clamping value: 0x%hu to 0x%hu\n",
                           filename, instruments.size(), i, eValue, 0x7F + 1);
                eValue = 0x7F;
            }
            vEnvelope[i] = eValue << 9 | (eOffset & 0x01FF);
        } else {
            vEnvelope[i] = 0xFFFF;
        }
    }

    // FILE: Panning envelope, number of points in volume envelope, and number of points in panning envelope.
    seek(48 + 1 + 1);

    // Now that we have all the volume envelope data, alloc and save to instrument
    size_t envelopeSize = instrument.nVolumeEnvelopePoints * sizeof(*vEnvelope);
    if (envelopeSize == 0) {
        instrument.volumeEnvelopePoints = -1;
    } else {
        instrument.volumeEnvelopePoints = envelopes.size();
        std::vector<uint8_t> envelope(envelopeSize * sizeof(*vEnvelope));
        memcpy(&envelope[0], vEnvelope, envelopeSize);
        size += envelopeSize;
        envelopes.push_back(envelope);
    }

    // FILE: Instrument data
    instrument.volumeSustainPoint = get8();
    instrument.volumeLoopStartPoint = get8();
    instrument.volumeLoopEndPoint = get8();
    // FILE: Panning sustain, loop start, and loop end
    seek(1 + 1 + 1);
    instrument.volumeType = get8();
    // FILE: Panning type
    seek(1);
    instrument.vibratoType = get8();
    instrument.vibratoSweep = get8();
    instrument.vibratoDepth = get8();
    instrument.vibratoRate = get8();
    instrument.volumeFadeout = get16();

    // FILE: Seek to sample header
    aseek(offset + instrumentLength);
    // FILE: Sample data
    sample.dataSize = get32();
    sample.loopStart = get32();
    sample.loopEnd = get32();

    sample.volume = get8();
    instrument.finetune = (int8_t)get8();  // TODO: test with a tracker that uses negative finetune

    // FILE: Sample format and loopType share a byte
    sample.loopType = get8();

    // FILE: Default panning
    seek(1);
    instrument.relativeNoteNumber = get8();

    // FILE: Reserved, but the ad hoc spec claims it does something
    uint8_t reserved = get8();
    if (reserved) {
        // The ad hoc stripped spec claims this can be either 0x00 (pcm) and 0xAD (adpcm), so let's run with that.
        if (reserved == 0xAD) {
            log->error("%s, instrument %u: Assuming adpcm encoding", filename, instruments.size());
            sample.loopType = (sample.loopType & 0x3) | 1 << 3;
        }
        // Other tracker implementations claim that this byte is reserved, so ignore if invalid.
    }

    // FILE: Sample name
    seek(22);

    // Parse loop boundaries into sensible units
    uint8_t format = (sample.loopType >> 3) & 0x3;
    if (sample.loopEnd == 0) {
        // If loop length is 0, no looping
        sample.loopStart = 0;
        // Preserve format information, it's used by readSample().
        sample.loopType &= 0xF8;
    } else {
        // Compute offsets in samples
        if (format == kSampleFormatPCM16) {
            sample.loopStart /= sizeof(int16_t);
            sample.loopEnd /= sizeof(int16_t);
        } else if (format == kSampleFormatADPCM) {
            sample.loopStart *= 2;
            sample.loopEnd *= 2;
        }

        // convert loopLength to loopEnd (zero-indexed)
        sample.loopEnd += sample.loopStart;
    }

    // Fast-forward through the extra sample headers, if any.
    uint32_t skipData = 0;
    while (nSamples--) {
        skipData += get32();
        seek(4 + 4 + 1 + 1 + 1 + 1 + 1 + 1 + 22);
    }

    // Read and process sample
    if (!readSample(instrument)) return false;

    // Fast-forward through the superfluous sample data, if any.
    if (skipData) seek(skipData);

    /*
     * That's it for instrument/sample data.
     * Sanity check parameters and convert to expected units/ranges.
     */

    if (sample.volume > 64) {
        log->error("%s, instrument %u: Sample volume is %u, clamped to %u",
                   filename, instruments.size(), sample.volume, 64);
        sample.volume = 64;
    }

    // Save instrument
    size += sizeof(instrument);
    instruments.push_back(instrument);

    return true;
}

/*
 * Read sample data from the file. Sample data is handled differently based on
 * the format:
 *
 *      adpcm: stored as found
 *      pcm8: converted to pcm16 and then processed as pcm16 below
 *      pcm16: converted to adpcm
 *      default: blow up
 */
bool XmTrackerLoader::readSample(_SYSXMInstrument &instrument)
{
    _SYSAudioModule &sample = instrument.sample;
    sample.pData = -1;
    sample.sampleRate = 8363;
    uint8_t format = (sample.loopType >> 3) & 0x3;

    // Loop type should store only the loop type
    sample.loopType &= 0x3;

    if (sample.dataSize == 0) return true;
    
    bool pcm8 = false;
    instrument.compression = 4;

    switch (format) {
        case kSampleFormatADPCM: {
            /*
             * If ADPCM, read directly into memory and done.
             *
             * This isn't quite right, though! Our own variant of ADPCM doesn't
             * quite match XM's variant. For now, we'll just copy it directly
             * anyway (This may be less lossy than decoding and reencoding?) and
             * issue a warning.
             */

            log->error("%s, instrument %u: ADPCM samples cannot be accurately re-encoded. "
                "For better quality, use uncompressed samples in your XM file and let stir "
                "compress them.", filename, instruments.size());

            // Prefix with three zero bytes (default codec initial conditions)
            std::vector<uint8_t> sampleData(sample.dataSize + 3);
            memset(&sampleData[0], 0, 3);
            getbuf(&sampleData[3], sample.dataSize);

            sample.pData = globalSampleDatas.size();
            size += sample.dataSize;
            globalSampleDatas.push_back(sampleData);
            sample.type = _SYS_ADPCM;
            instrument.compression = 1;
            break;
        }
        case kSampleFormatPCM8:
            pcm8 = true;
            instrument.compression = 2;
            // Intentional fall-through
        case kSampleFormatPCM16: {
            uint32_t numSamples = sample.dataSize / (pcm8 ? 1 : 2);
            sample.dataSize = numSamples * sizeof(int16_t);
            int16_t *buf = (int16_t*)malloc(sample.dataSize);
            if (!buf) return false;
            for (unsigned i = 0; i < numSamples; i++) {
                buf[i] = pcm8
                       ? (int16_t)((int8_t)get8() << 8)
                       : (int16_t)get16();
            }

            // XM uses delta modulation; mix into regular pcm.
            int16_t mix = 0;
            for(unsigned i = 0; i < numSamples; i++) {
                // Allowing overflow here is intentional!
                buf[i] = (mix += buf[i]);
            }

            // Ping-pong loops
            if (sample.loopType == 2) {
                /* Convert a ping-pong sample from:
                 * [start][0123456789][end] (loop length: 10)
                 * to:
                 * [start][012345678987654321][end] (loop length: 18)
                 */
                uint32_t loopLength = sample.loopEnd - sample.loopStart + 1;

                // First and last samples of loop are not duplicated
                uint32_t addlLength = loopLength - 2;

                // Extend buffer
                sample.dataSize += addlLength * sizeof(int16_t);
                buf = (int16_t*)realloc(buf, sample.dataSize);
                if (!buf) return false;

                // Move non-looping end of sample
                memcpy(buf + sample.loopStart + loopLength + addlLength,
                       buf + sample.loopStart + loopLength,
                       numSamples - (sample.loopStart + loopLength));

                // Create the pong portion of the loop
                for (unsigned i = 1; i <= addlLength; i++) {
                    buf[sample.loopEnd + i] = buf[sample.loopEnd - i];
                }

                numSamples += addlLength;
                sample.loopEnd += addlLength;
            }

            /// XXX: We could just be using std::vector above too...
            std::vector<uint8_t> rawBytes((const uint8_t*) buf,
                (const uint8_t*) buf + (numSamples * sizeof(int16_t)));
            free(buf);

            // Encode to today's default format
            std::vector<uint8_t> sampleData;
            AudioEncoder *enc = AudioEncoder::create("");
            enc->encode(rawBytes, sampleData);

            sample.pData = globalSampleDatas.size();
            sample.dataSize = sampleData.size();
            size += sample.dataSize;
            globalSampleDatas.push_back(sampleData);
            sample.type = enc->getType();
            break;
        }
        default: {
            log->error("%s, instrument %u: Unknown sample encoding", filename, instruments.size());
            return false;
        }
    }
    return true;
}

/*
 * Clear all allocations and collections.
 * Called either by the destructor, on a loader error, or when loading another module.
 */
bool XmTrackerLoader::init()
{
    size = 0;
    instruments.clear();

    patterns.clear();
    patternDatas.clear();
    patternTable.clear();

    memset(&song, 0, sizeof(song));
    return false;
}
}
