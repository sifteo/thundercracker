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

#ifdef _WIN32
#define _WIN32_WINNT 0x0501
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static bool hasConsole = true;
#endif
 
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "frontend.h"
#include "system.h"


static void getConsole()
{
    /*
     * We run as a GUI app on Windows, and we don't have stdout/stderr by default.
     * Try to reattach them if the parent process has a console.
     */

#ifdef _WIN32
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        hasConsole = true;
        freopen("CONOUT$", "wb", stdout);
        freopen("CONOUT$", "wb", stderr);
    } else {
        hasConsole = false;
    }
#endif
}


static void message(const char *fmt, ...)
{
    char buf[1024];

    va_list a;
    va_start(a, fmt);
    vsnprintf(buf, sizeof buf, fmt, a);
    va_end(a);
    
#ifdef _WIN32
    if (!hasConsole) {
        // No console.. resort to a messagebox
        MessageBox(NULL, buf, APP_TITLE, MB_OK);
        return;
    }
#endif

    fprintf(stderr, "%s", buf);
}
    

static void usage()
{
    /*
     * There are additionally several undocumented options that are
     * only useful for internal development:
     *
     *  -f FIRMWARE.hex   Specify firmware image for cubes
     *  -t TRACE.txt      Trace firmware execution to a text file (Toggle with 'T' key)
     *  -F FLASH.bin      Load/save flash memory (first cube only) to a binary file
     *  -p PROFILE.txt    Profile firmware execution (first cube only) to a text file
     *  -d                Launch firmware debugger (first cube only)
     */

    message("\n"
            "usage: tc-siftulator [OPTIONS]\n"
            "\n"
            "Sifteo Thundercracker simulator\n"
            "\n"
            "Options:\n"
            "  -h            Show this help message, and exit\n"
            "  -n NUM        Set initial number of cubes\n"
            "  -T            No throttle; run faster than real-time if we can\n"
            "\n"
            "Copyright <c> 2011 Sifteo, Inc. All rights reserved.\n"
            "\n");
}

int main(int argc, char **argv)
{
    static System sys;
    static Frontend fe;

    // Attach an existing console, if it's already handy
    getConsole();
    
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
            sys.opt_cubeTrace = argv[c+1];
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
                message("Error: Unsupported number of cubes (Minimum 0, maximum %d)\n",
                        sys.MAX_CUBES);
                return 1;
            }
            c++;
            continue;
        }

        if (arg[0] == '-') {
            message("Unrecognized option: '%s'\n", arg);
            usage();
            return 1;
        }

        /*
         * No positional command line options yet. In the future this
         * may be a game binary to run on the master block.
         */
        message("Unrecognized argument: '%s'\n", arg);
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
                                  sys.opt_cubeTrace ||
                                  sys.opt_cube0Debug)) {
        message("Debug features only available if a firmware image is provided.\n");
        return 1;
    }

    /*
     * Run stuff!
     */

    sys.init();
    if (!fe.init(&sys)) {
        message("Graphical frontend failed to initialize.\n");
        return 1;
    }    

    sys.start();
    fe.run();

    fe.exit();
    sys.exit();

    return 0;
}
