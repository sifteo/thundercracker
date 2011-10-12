/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * Command line front-end.
 */

#include <SDL.h>
#include <stdio.h>
#include <string.h>

#include "system.h"
#include "frontend.h"

// We run in console mode on Windows
#ifdef _WIN32
#undef main
#endif


static void usage(const char *argv0)
{
    fprintf(stderr,
            "\n"
            "usage: %s [OPTIONS]\n"
            "\n"
            "Sifteo Thundercracker simulator\n"
            "\n"
            "Options:\n"
            "  -h                Show this help message, and exit\n"
            "  -f FIRMWARE.hex   Specify firmware image for cubes\n"
            "  -F FLASH.bin      Load/save flash memory (first cube only) to a binary file\n"
            "  -t TRACE.txt      Trace firmware execution (first cube only) to a text file\n"
            "  -p PROFILE.txt    Profile firmware execution (first cube only) to a text file\n"
            "  -d                Launch firmware debugger (first cube only)\n"
            "  -n NUM            Set number of cubes\n"
            "\n"
            "Copyright <c> 2011 Sifteo, Inc. All rights reserved.\n"
            "Confidential, not for redistribution.\n"
            "\n",
            argv0);
}

int main(int argc, char **argv)
{
    static System sys;
    static Frontend fe;

    /*
     * Parse command line options
     */

    for (int c = 1; c < argc; c++) {
        char *arg = argv[c];

        if (!strcmp(arg, "-h")) {
            usage(argv[0]);
            return 0;
        }

        if (!strcmp(arg, "-d")) {
            sys.opt_cube0Debug = true;
            continue;
        }
         
        if (!strcmp(arg, "-f") && argv[c+1]) {
            sys.opt_cubeFirmware = argv[c+1];
            c++;
            continue;
        }

        if (!strcmp(arg, "-F") && argv[c+1]) {
            sys.opt_cube0Flash = argv[c+1];
            c++;
            continue;
        }

        if (!strcmp(arg, "-t") && argv[c+1]) {
            sys.opt_cube0Trace = argv[c+1];
            c++;
            continue;
        }

        if (!strcmp(arg, "-p") && argv[c+1]) {
            sys.opt_cube0Profile = argv[c+1];
            c++;
            continue;
        }

        if (!strcmp(arg, "-n") && argv[c+1]) {
            sys.opt_numCubes = atoi(argv[c+1]);
            if (sys.opt_numCubes > sys.MAX_CUBES) {
                fprintf(stderr, "Error: Unsupported number of cubes (Minimum 0, maximum %d)\n",
                        sys.MAX_CUBES);
                return 1;
            }
            c++;
            continue;
        }

        if (arg[0] == '-') {
            fprintf(stderr, "Unrecognized option: '%s'\n", arg);
            return 1;
        }

        /*
         * No positional command line options yet. In the future this
         * may be a game binary to run on the master block.
         */
        fprintf(stderr, "Unrecognized argument: '%s'\n", arg);
        return 1;
    }

    if (!sys.opt_cubeFirmware) {
        /*
         * XXX: We should have a built in binary-translated firmware to run
         *      when no image was specified.
         */
        fprintf(stderr, "Warning: No cube firmware image! Did you forget '-f path/to/cube.hex'?\n");
    }

    /*
     * Run stuff!
     */

    sys.init();
    fe.init(&sys);

    sys.start();
    fe.run();

    fe.exit();
    sys.exit();

    return 0;
}
