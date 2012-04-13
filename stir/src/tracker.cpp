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
bool XmTrackerLoader::load(const char *filename, Logger &pLog)
{
    log = &pLog;

    // If this instance already contains module data, clean it up first.
    if (patterns.size()) init();

    f = fopen(filename, "rb");
    if (f == 0) return false;

    if (!readSong()) return init();

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
    bool compressed = false;
    for (unsigned i = 0; i < song.nPatterns; i++) {

        if (!readNextPattern()) return false;

        uint32_t uncompressedSize = patternDatas[i].size();
        unsigned ops = compressPattern(i);
        if (patternDatas[i].size() == 0) return false;
        
        /* Only output compression statistics if we saved any space.
         * In practice, trackers seem to be pretty good at optimising patterns themselves.
         */
        if (ops) {
            if (!compressed) {
                log->infoBegin("Pattern compression");
                compressed = true;
            }
            float ratio = 100.0 - patternDatas[i].size() * 100.0 / uncompressedSize;
            log->infoLine("%4u: % 7u ops, % 4u rows, % 5u bytes, % 5.01f%% compression",
                          i, ops, patterns[i].nRows, patternDatas[i].size(), ratio);
        }
    }
    if (compressed) log->infoEnd();

    /* We compress both envelopes and samples in readNextInstrument and its
     * callees, but they only print envelope compression statistics.
     */
    log->infoBegin("Envelope compression");
    for (unsigned i = 0; i < song.nInstruments; i++) {
        if (!readNextInstrument()) return false;
    }
    log->infoEnd();

    /* Print sample compression statistics separately from envelope
     * compression statistics
     */
    log->infoBegin("Sample compression");
    uint8_t sampleNum = 0;
    for (unsigned i = 0; i < instruments.size(); i++) {
        if (instruments[i].sample.dataSize) {
            log->infoLine("Sample %2u: %5u bytes, %s %s",
                          sampleNum++, instruments[i].sample.dataSize,
                          encodings[instruments[i].sample.type],
                          sampleNames.front().c_str());
            sampleNames.pop();
        }
    }
    assert(sampleNames.size() == 0);
    log->infoEnd();

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
    std::vector<uint8_t> patternData;
    for (int i = 0; i < pattern.dataSize; i++) {
        patternData.push_back(get8());
    }

    patterns.push_back(pattern);
    patternDatas.push_back(patternData);

    return true;
}

/*
 * (Re)compress a pattern.
 *
 * The XM pattern format supports two different methods of storing notes. The
 * first is as five bytes: the note, instrument, volume, effect type, and
 * effect parameter. Notes are within the range of (0..97), so the first byte
 * of this note format always has it's MSB = 0.
 *
 * The second format begins with a code byte, with it's MSB set, the bottom
 * 5 bits of which stand for, in order:
 *      0: Note byte follows
 *      1: Instrument byte follows
 *      2: Volume byte follows
 *      3: Effect type byte follows
 *      4: Effect parameter byte follows
 * Thus a new note using the channel's existing instrument with a new volume
 * and no effects could be described with uint8_t[3] = {0x83, <note>, <volume>}
 */
unsigned XmTrackerLoader::compressPattern(uint16_t pattern)
{
    unsigned ops = 0;
    uint32_t w = 0;
    uint32_t r = 0;
    std::vector<uint8_t> &data = patternDatas[pattern];

    while(r < data.size()) {
        // decode note
        uint8_t code = data[r++];
        uint8_t note = 0;
        uint8_t instrument = 0;
        uint8_t volume = 0;
        uint8_t effectType = 0;
        uint8_t effectParam = 0;
        bool wasEncoded = !!(code & 0x80);

        if (code & 0x80) {
            if (code & 1 << 0) {
                assert(r < data.size());
                note = data[r++];
            }
            if (code & 1 << 1) {
                assert(r < data.size());
                instrument = data[r++];
            }
            if (code & 1 << 2) {
                assert(r < data.size());
                volume = data[r++];
            }
            if (code & 1 << 3) {
                assert(r < data.size());
                effectType = data[r++];
            }
            if (code & 1 << 4) {
                assert(r < data.size());
                effectParam = data[r++];
            }
        } else {
            assert(r < data.size() - 4);
            note = code;
            instrument = data[r++];
            volume = data[r++];
            effectType = data[r++];
            effectParam = data[r++];
        }

        // value consistency checking here
        code = 0x80;
        code |= note == 0 ? 0 : 1 << 0;
        code |= instrument == 0 ? 0 : 1 << 1;
        code |= volume == 0 ? 0 : 1 << 2;
        code |= effectType == 0 ? 0 : 1 << 3;
        code |= effectParam == 0 ? 0 : 1 << 4;

        // store uncompressed if it saves space, or if it doesn't make a difference and it wasn't encoded before.
        if (__builtin_popcount(code & 0x1F) > 4 || (__builtin_popcount(code & 0x1F) > 3 && !wasEncoded)) {
            if (wasEncoded) ops++;
            data[w++] = note;
            data[w++] = instrument;
            data[w++] = volume;
            data[w++] = effectType;
            data[w++] = effectParam;
        } else {
            if (!wasEncoded) ops++;
            
            data[w++] = code;
            if (code & 1 << 0) {
                data[w++] = note;
            }
            if (code & 1 << 1) {
                data[w++] = instrument;
            }
            if (code & 1 << 2) {
                data[w++] = volume;
            }
            if (code & 1 << 3) {
                data[w++] = effectType;
            }
            if (code & 1 << 4) {
                data[w++] = effectParam;
            }
        }
    }

    while (w < data.size()) data.pop_back();
    return ops;
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

    // FILE: Instrument name
    std::string instrumentName;
    for (int i = 0; i < 22; i++)
        instrumentName += get8();
    // Make it pretty for the logger
    processName(instrumentName);
    if (instrumentName.size())
        instrumentName.insert(0, 1, '(').push_back(')');

    // FILE: Instrument type
    seek(1);

    // FILE: Number of samples
    uint16_t nSamples = get16();
    if (nSamples == 0) {
        log->error("Instruments with no samples have not been tested");
        /* There is nothing more to read, padding aside, the next thing in the
         * file is the next instrument.
         */
        aseek(offset + instrumentLength);
        return true;
    }
    if (nSamples > 1) {
        log->error("Instrument has %u samples, expected 1", nSamples);
        return false;
    }

    // FILE: Sample header size (redundant), keymap assignments (redundant if only one sample)
    seek(4 + 96);

    // FILE: Volume envelope
    uint16_t vEnvelope[12];
    for (int i = 0; i < 12; i++) {
        uint16_t eOffset = get16();
        uint16_t eValue = get16();
        assert(eOffset == eOffset & 0x01FF); // 9 bits
        assert(eValue == eValue & 0x7F); // 7 bits
        vEnvelope[i] = eValue << 9 | (eOffset & 0x01FF);
    }

    // FILE: Panning envelope
    seek(48);

    // FILE: Number of points in volume envelope
    instrument.nVolumeEnvelopePoints = get8();

    // Now that we have all the volume envelope data, alloc and save to instrument
    size_t envelopeSize = instrument.nVolumeEnvelopePoints * sizeof(*vEnvelope);
    if (envelopeSize == 0) {
        instrument.volumeEnvelopePoints = 0;
    } else {
        instrument.volumeEnvelopePoints = (uint16_t *)malloc(envelopeSize);
        if (instrument.volumeEnvelopePoints == 0) return false;
        memcpy(instrument.volumeEnvelopePoints, vEnvelope, envelopeSize);

        float ratio = 100.0 - envelopeSize * 100.0 / 48;
        log->infoLine("Envelope %2u: %2u points, % 5.01f%% compression %s",
            instruments.size(), instrument.nVolumeEnvelopePoints, ratio, instrumentName.c_str());
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
            log->error("Instrument %u: Assuming adpcm sample encoding", instruments.size());
            sample.loopType = (sample.loopType & 0x3) | 1 << 3;
        }
        // Other tracker implementations claim that this byte is reserved, so ignore if invalid.
    }

    // FILE: Sample name, we don't want it if theres no sampledata
    if (sample.dataSize) {
        std::string sampleName;
        for (int i = 0; i < 22; i++) {
            sampleName += get8();
        }
        // Make it pretty for the logger
        processName(sampleName);
        if (sampleName.size()) {
            sampleName.insert(0, 1, '(').push_back(')');
        }
        sampleNames.push(sampleName);        
    } else {
        seek(22);
    }

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
            log->error("Instrument %u: Ping-pong loops are not supported, falling back to normal looping", instruments.size());
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
    sample.pData = 0;
    uint8_t format = (sample.loopType >> 3) & 0x3;

    if (sample.dataSize == 0) return true;
    
    bool pcm8 = false;
    switch (format) {
        case kSampleFormatADPCM: {
            // if adpcm, read directly into memory and done
            uint8_t *buf = (uint8_t*)malloc(sample.dataSize);
            if (!buf) return false;
            getbuf(buf, sample.dataSize);
            sample.pData = buf;
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
            sample.pData = (uint8_t *)realloc(buf, sample.dataSize);
            sample.type = enc->getType();
            break;
        }
        default: {
            log->error("Instrument %u has unknown sample type %u", instruments.size(), format);
            return false;
        }
    }
    return true;
}

/*
 * Remove padding from names.
 *
 * Name fields in XM files are defined to be between 17 and 22 ASCII chars,
 * padded by 0x20 chars. In practice it is also common to find names padded
 * with NULs, which std::string will happily store.
 */
void XmTrackerLoader::processName(std::string &name)
{
    while ((name[name.size() - 1] == ' ' || name[name.size() - 1] == '\0') && name.size()) {
        name.erase(name.size() - 1);
    }
}

/*
 * Clear all allocations and collections.
 * Called either by the destructor, on a loader error, or when loading another module.
 */
bool XmTrackerLoader::init()
{
    for (unsigned i = 0; i < instruments.size(); i++) {
        if (instruments[i].volumeEnvelopePoints) free(instruments[i].volumeEnvelopePoints);
        if (instruments[i].sample.pData) free(instruments[i].sample.pData);
    }

    instruments.clear();
    patterns.clear();
    patternDatas.clear();
    patternTable.clear();

    while (!sampleNames.empty()) {
        sampleNames.pop();
    }

    memset(&song, 0, sizeof(song));
    return false;
}

XmTrackerLoader::~XmTrackerLoader()
{
    init();
}

}