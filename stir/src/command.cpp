/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * STIR -- Sifteo Tiled Image Reducer
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * Command line front-end.
 */

#include <stdio.h>
#include <string.h>

#include "tile.h"
#include "script.h"


static void usage(const char *argv0)
{
    fprintf(stderr,
	    "\n"
	    "usage: %s [OPTIONS] FILE.lua\n"
	    "\n"
	    "Sifteo Tiled Image Reducer, a compile-time game asset preparation\n"
	    "tool. Reads a configuration file which describes your game's assets,\n"
	    "compresses and reformats them, and produces code that is incorporated\n"
	    "into your game's build.\n"
	    "\n"
	    "Options:\n"
	    "  -h            Show this help message, and exit\n"
	    "  -v            Verbose mode, show progress as we work\n"
	    "  -o FILE.cpp   Generate a C++ source file with your asset data\n"
	    "  -o FILE.h     Generate a C++ header with metadata for your assets\n"
	    "  -o FILE.html  Generate a proofing sheet for your assets, in HTML format\n"
	    "  VAR=VALUE     Define a script variable, prior to parsing the script\n"
	    "\n"
	    "Copyright <c> 2011 Sifteo, Inc. All rights reserved.\n"
	    "\n",
	    argv0);
}

int main(int argc, char **argv)
{
    Stir::ConsoleLogger log;
    Stir::Script script(log);
    const char *scriptFile = NULL;

    /*
     * Parse command line options
    */

    for (int c = 1; c < argc; c++) {
	char *arg = argv[c];

	if (!strcmp(arg, "-h")) {
	    usage(argv[0]);
	    return 0;
	}

	if (!strcmp(arg, "-v")) {
	    log.setVerbose();
	    continue;
	}
	 
	if (!strcmp(arg, "-o") && argv[c+1]) {
	    if (script.addOutput(argv[c+1])) {
		c++;
		continue;
	    } else {
		log.error("Unrecognized or duplicate output file: '%s'", argv[c+1]);
		return 1;
	    }
	}

	if (arg[0] == '-') {
	    log.error("Unrecognized option: '%s'", arg);
	    return 1;
	}

	if (strchr(arg, '=')) {
	    char *value = strchr(arg, '=') + 1;
	    value[-1] = '\0';
	    script.setVariable(arg, value);
	    continue;
	}

	if (scriptFile) {
	    log.error("Too many command line arguments");
	    return 1;
	}

	scriptFile = arg;
    }

    if (!scriptFile) {
	usage(argv[0]);
	return 1;
    }

    /*
     * Run the script!
     */

    Stir::CIELab::initialize();

    if (script.run(scriptFile))
	return 1;

    return 0;
}


/* Junk...
    std::vector<uint8_t> loadstream;
    pool.encode(loadstream, &log);
    LodePNG::saveFile(loadstream, "loadstream.bin");

    log.infoBegin("Asset statistics");
    log.infoLine("%30s: %d", "Total tiles", pool.size());
    log.infoLine("%30s: %.02f kB", "Installed size in NOR", pool.size() * 128 / 1024.0, pool.size());
    log.infoLine("%30s: %.02f kB", "Loadstream size", loadstream.size() / 1024.0);
    log.infoLine("%30s: %.02f kB", "Compressed loadstream", zLoadstream.size() / 1024.0);
    log.infoLine("%30s: %.02f s", "Load time estimate", loadstream.size() / 40000.0);
    log.infoLine("%30s: %.02f kB", "Uncompressed map", map.size() / 1024.0);
    log.infoLine("%30s: %.02f kB", "Compressed map", zMap.size() / 1024.0);
    log.infoLine("%30s: %.02f kB", "Compressed total", (zLoadstream.size() + zMap.size()) / 1024.0);
    log.infoEnd();

*/
