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
#include <set>

namespace Stir {

// For logger output
const char * XmTrackerLoader::encodings[3] = {"", "Uncompressed PCM", "IMA 4-bit ADPCM"};

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
    if (f == 0) return false;

    if (!readSong()) return init();

    fseek(f, 0, SEEK_END);
    fileSize = ftell(f);
    fclose(f);
    f = 0;

    return true;
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
        /* There is nothing more to read, padding aside, the next thing in the
         * file is the next instrument.
         */
        size += sizeof(instrument);
        instruments.push_back(instrument);
        aseek(offset + instrumentLength);
        return true;
    }
    if (nSamples > 1) {
        log->error("%s, instrument %u: found %u samples, expected 1", filename, instruments.size(), nSamples);
        return false;
    }

    // FILE: Sample header size (redundant), keymap assignments (redundant if only one sample)
    seek(4 + 96);

    // FILE: Volume envelope (48 bytes — 12 * sizeof(uint16_t) * 2)
    uint16_t vEnvelope[12];
    for (int i = 0; i < 12; i++) {
        uint16_t eOffset = get16();
        uint16_t eValue = get16();
        assert(eOffset == (eOffset & 0x01FF));  // 9 bits
        assert(eValue == (eValue & 0x7F));      // 7 bits
        vEnvelope[i] = eValue << 9 | (eOffset & 0x01FF);
    }

    // FILE: Panning envelope
    seek(48);

    // FILE: Number of points in volume envelope
    instrument.nVolumeEnvelopePoints = get8();

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

    // FILE: Number of points in panning envelope
    seek(1);

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

    // Read and process sample
    if (!readSample(instrument)) return false;

    /*
     * That's it for instrument/sample data.
     * Sanity check parameters and convert to expected units/ranges.
     */

    // Sifteo volume range is (0..256), XM is (0..64)
    sample.volume *= 4;

    // Loop type should store only the loop type
    uint8_t format = (sample.loopType >> 3) & 0x3;
    sample.loopType &= 0x3;

    // If loop length is 0, no looping
    if (sample.loopEnd == 0) {
        sample.loopStart = 0;
        sample.loopType = 0;
    } else {
        // Ping-pong loops are not supported
        if (sample.loopType > 1) {
            assert(sample.loopType == 2);
            log->error("%s, instrument %u: Ping-pong loops are not supported, falling back to normal looping",
                       filename, instruments.size());
            sample.loopType = 1;
        }
    
        // Compute offsets in samples
        if (format == kSampleFormatPCM16) {
            sample.loopStart /= sizeof(int16_t);
            sample.loopEnd /= sizeof(int16_t);
        } else if (format == kSampleFormatADPCM) {
            sample.loopStart *= 2;
            sample.loopEnd *= 2;
        }

        // convert loopLength to loopEnd (zero-indexed)
        // TODO: double check loop end!
        sample.loopEnd += sample.loopStart - 1;
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

    if (sample.dataSize == 0) return true;
    
    bool pcm8 = false;
    switch (format) {
        case kSampleFormatADPCM: {
            // if adpcm, read directly into memory and done
            std::vector<uint8_t> sampleData(sample.dataSize);
            getbuf(&sampleData[0], sample.dataSize);
            sample.pData = sampleDatas.size();
            size += sample.dataSize;
            sampleDatas.push_back(sampleData);
            sample.type = _SYS_ADPCM;
            break;
        }
        case kSampleFormatPCM8:
            pcm8 = true;
            // intentional fall-through
        case kSampleFormatPCM16: {
            uint32_t numSamples = sample.dataSize / (pcm8 ? 1 : 2);
            int16_t *buf = (int16_t*)malloc(numSamples * 2);
            if (!buf) return false;
            for (unsigned i = 0; i < numSamples; i++) {
                buf[i] = pcm8
                       ? (int16_t)((int8_t)get8() << 8)
                       : (int16_t)get16();
            }

            // Encode to today's default format
            AudioEncoder *enc = AudioEncoder::create("");
            sample.dataSize = enc->encodeBuffer(buf, numSamples * sizeof(int16_t));

            std::vector<uint8_t> sampleData(sample.dataSize);
            memcpy(&sampleData[0], buf, sample.dataSize);
            free(buf);
            sample.pData = sampleDatas.size();
            size += sample.dataSize;
            sampleDatas.push_back(sampleData);
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
    sampleDatas.clear();

    patterns.clear();
    patternDatas.clear();
    patternTable.clear();

    memset(&song, 0, sizeof(song));
    return false;
}
}
