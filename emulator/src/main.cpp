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

#include <stdio.h>
#include <string.h>

#include "frontend.h"
#include "system.h"


static void usage()
{
    /*
     * There are additionally several undocumented options that are
     * only useful for internal development:
     *
     *  -f FIRMWARE.hex   Specify firmware image for cubes
     *  -F FLASH.bin      Load/save flash memory (first cube only) to a binary file
     *  -t TRACE.txt      Trace firmware execution (first cube only) to a text file
     *  -p PROFILE.txt    Profile firmware execution (first cube only) to a text file
     *  -d                Launch firmware debugger (first cube only)
     */
     
    fprintf(stderr,
            "\n"
            "usage: tc-siftulator [OPTIONS]\n"
            "\n"
            "Sifteo Thundercracker simulator\n"
            "\n"
            "Options:\n"
            "  -h                        Show this help message, and exit\n"
            "  -n NUM                    Set number of cubes\n"
            "  -T                        No throttle; run faster than real-time if we can\n"
            "\n"
            "Interactions:\n"
            "  Drag center               Pull and align a cube\n"
            "  Drag edge                 Pull and rotate a cube\n"
            "  Right click / Spacebar    While pulling, hover / spin a cube\n"
            "  Click center              Touch a cube\n"
            "  Shift-drag / Right-drag   Tilt a cube\n"
            "  Mouse wheel               Change size of play surface\n"
            "  F key                     Toggle full-screen mode\n"
            "  Z key                     Toggle zoom mode\n"
            "  Q / Escape key            Quit\n"
            "\n"
            "Copyright <c> 2011 Sifteo, Inc. All rights reserved.\n"
            "\n");
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
            usage();
            return 0;
        }

        if (!strcmp(arg, "-d")) {
            sys.opt_cube0Debug = true;
            continue;
        }

        if (!strcmp(arg, "-T")) {
            sys.opt_noThrottle = true;
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
            usage();
            return 1;
        }

        /*
         * No positional command line options yet. In the future this
         * may be a game binary to run on the master block.
         */
        fprintf(stderr, "Unrecognized argument: '%s'\n", arg);
        usage();
        return 1;
    }

    /*
     * We want to disable our debugging features when using the
     * built-in binary translated firmware. The debugger won't really
     * work properly in SBT mode anyway, but we additionally want to
     * disable it in order to make it harder to reverse engineer our
     * firmware. Of course, any dedicated reverse engineer could just
     * disable this test easily :)
     */

    if (!sys.opt_cubeFirmware && (sys.opt_cube0Profile ||
                                  sys.opt_cube0Trace ||
                                  sys.opt_cube0Debug)) {
        fprintf(stderr, "Debug features only available if a firmware image is provided.\n");
        return 1;
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
