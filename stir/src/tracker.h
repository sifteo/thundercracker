/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * STIR -- Sifteo Tracker Instance Reducer
 * Scott Perry <sifteo@numist.net>
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _TRACKER_H
#define _TRACKER_H

#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <vector>
#include <queue>
#include "logger.h"
#include "sifteo/abi.h"

namespace Stir {

class XmTrackerLoader {
public:
	XmTrackerLoader() : log(0) {}
	~XmTrackerLoader();
	bool load(const char *filename, Logger &pLog);

	const _SYSXMSong &getSong() const {
		assert(song.nPatterns);
		return song;
	}
	const _SYSXMPattern &getPattern(uint8_t i) const {
		assert(i < song.nPatterns);
		return patterns[i];
	}
	const std::vector<uint8_t> &getPatternData(uint8_t i) const {
		assert(i < song.nPatterns);
		return patternDatas[i];
	}
	const _SYSXMInstrument &getInstrument(uint8_t i) const {
		assert(i < song.nInstruments);
		return instruments[i];
	}
	const uint8_t *getSample(uint8_t i) const {
		assert(i < sampleDatas.size());
		return sampleDatas[i];
	}

private:
	bool openTracker(const char *filename);
	bool readSong();

	bool readNextInstrument();
	bool readSample(_SYSXMInstrument &instrument);
	// bool saveInstruments();

	bool readNextPattern();
	unsigned compressPattern(uint16_t pattern);
	bool savePatterns();

	bool init();

	// Portable file functions, modules are little-endian binary files!
	void seek(uint32_t offset) { fseek(f, offset, SEEK_CUR); }
	void aseek(uint32_t offset) { fseek(f, offset, SEEK_SET); }
	uint32_t pos() { return ftell(f); }
	uint8_t get8() { return getc(f); }
	uint16_t get16() { return get8() | (uint16_t)get8() << 8; }
	uint32_t get32() { return get16() | (uint32_t)get16() << 16; }
	void getbuf( void *buf, uint32_t bufsiz ) {
		uint8_t *wbuf = (uint8_t*)buf;
		for (uint32_t i = 0; i < bufsiz; i++) {
			wbuf[i] = get8();
		}
	}

	void processName(std::string &name);

	// constants
	enum {
		kSampleFormatPCM8 = 0,
		kSampleFormatADPCM,
		kSampleFormatPCM16,
		kSampleFormatUnknown
	};
	static const char *encodings[3];

	FILE *f;
	Logger *log;
	_SYSXMSong song;
	
	std::vector<std::vector<uint8_t> > patternDatas;
	std::vector<_SYSXMPattern> patterns;
	std::vector<uint8_t> patternTable;

	std::vector<_SYSXMInstrument> instruments;
	std::vector<uint8_t *> sampleDatas;
	std::queue<std::string> sampleNames;
};

}

#endif // _TRACKER_H