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
#include "lua_script.h"


static void message(const char *fmt, ...);

static void usage()
{
    /*
     * There are additionally several undocumented options that are
     * only useful for internal development. They intentionally have short
     * names, so that the long names don't show up in our binary's strings.
     *
     *  -f FIRMWARE.hex   Specify firmware image for cubes
     *  -p PROFILE.txt    Profile firmware execution (first cube only) to a text file
     *  -d                Launch firmware debugger (first cube only)
     *  -c                Continue executing on exception, rather than stopping the debugger.
     *  -R                Cube trace enabled at startup.
     */

    message("\n"
            "usage: siftulator [OPTIONS] [GAME.elf ...]\n"
            "\n"
            "Sifteo Thundercracker simulator\n"
            "\n"
            "Options:\n"
            "  -h                  Show this help message, and exit\n"
            "  -n NUM              Set initial number of cubes\n"
            "  -T                  Turbo mode; run faster than real-time if we can\n"
            "  -F FLASH.bin        Persistently keep all flash memory in a file on disk\n"
            "  -P PORT             Run a GDB debug server on the specified TCP port number\n"
            "  -e SCRIPT.lua       Execute a Lua script instead of the default frontend\n"
            "  -l LAUNCHER.elf     Start the supplied binary as the system launcher\n"
            "\n"
            "  --headless          Run without the graphical frontend\n"
            "  --lock-rotation     Lock rotation by default\n"
            "  --svm-trace         Trace SVM instruction execution\n"
            "  --svm-stack         Monitor SVM stack usage\n"
            "  --svm-flash-stats   Dump statistics about flash memory usage\n"
            "  --radio-trace       Trace all radio packet contents\n"
            "  --paint-trace       Trace the state of the repaint controller\n"
            "  --white-bg          Force the UI to use a plain white background\n"
            "  --stdout FILENAME   Redirect output to FILENAME\n"
            "\n"
            "Games:\n"
            "  Any games specified on the command line will be installed to\n"
            "  flash prior to starting the simulation. By default, a built-in\n"
            "  version of the system Launcher is used. This can be overridden\n"
            "  with the -l option.\n"
            "\n"
            APP_COPYRIGHT "\n");
}

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
    char buf[16*1024];

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

    fprintf(stderr, "%s\n", buf);
}

static int run(System &sys)
{
    static Frontend fe;
        
    if (!sys.init()) {
        message("Emulator failed to initialize");
        return 1;
    }

    if (!sys.opt_headless && !fe.init(&sys)) {
        message("Graphical frontend failed to initialize");
        return 1;
    }    

    sys.start();

    if (sys.opt_headless) {
        while (sys.isRunning())
            glfwSleep(0.1);
    } else {
        while (fe.runFrame());
        fe.exit();
    }

    sys.exit();

    return 0;
}

static int runScript(System &sys, const char *file)
{
    LuaScript lua(sys);
    int result = lua.runFile(file);
    sys.exit();
    return result;
}

int main(int argc, char **argv)
{
    static System sys;
    const char *scriptFile = NULL;

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

        if (!strcmp(arg, "-c")) {
            sys.opt_continueOnException = true;
            continue;
        }

        if (!strcmp(arg, "-T")) {
            sys.opt_turbo = true;
            continue;
        }
        
        if (!strcmp(arg, "-R")) {
            sys.opt_traceEnabledAtStartup = true;
            continue;
        }

        if (!strcmp(arg, "--lock-rotation")) {
            sys.opt_lockRotationByDefault = true;
            continue;
        }

        if (!strcmp(arg, "--svm-trace")) {
            sys.opt_svmTrace = true;
            continue;
        }

        if (!strcmp(arg, "--svm-stack")) {
            sys.opt_svmStackMonitor = true;
            continue;
        }

        if (!strcmp(arg, "--svm-flash-stats")) {
            sys.opt_svmFlashStats = true;
            continue;
        }

        if (!strcmp(arg, "--radio-trace")) {
            sys.opt_radioTrace = true;
            continue;
        }

        if (!strcmp(arg, "--paint-trace")) {
            sys.opt_paintTrace = true;
            continue;
        }
        
        if (!strcmp(arg, "--headless")) {
            sys.opt_headless = true;
            continue;
        }

        if (!strcmp(arg, "--white-bg")) {
            sys.opt_whiteBackground = true;
            continue;
        }
        
        if (!strcmp(arg, "--stdout") && argv[c+1]) {
            if(!freopen(argv[c+1], "w", stdout)) {
                message("Error: opening file %s for write", argv[c+1]);
                return 1;
            }
            c++;
            continue;
        }

        if (!strcmp(arg, "-f") && argv[c+1]) {
            sys.opt_cubeFirmware = argv[c+1];
            c++;
            continue;
        }

        if (!strcmp(arg, "-e") && argv[c+1]) {
            scriptFile = argv[c+1];
            c++;
            continue;
        }

        if (!strcmp(arg, "-F") && argv[c+1]) {
            sys.opt_flashFilename = argv[c+1];
            c++;
            continue;
        }

        if (!strcmp(arg, "-l") && argv[c+1]) {
            sys.opt_launcherFilename = argv[c+1];
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
                message("Error: Unsupported number of cubes (Minimum 0, maximum %d)", sys.MAX_CUBES);
                return 1;
            }
            c++;
            continue;
        }

        if (!strcmp(arg, "-P") && argv[c+1]) {
            sys.opt_gdbServerPort = atoi(argv[c+1]);
            c++;
            continue;
        }
        
        if (!strncmp(arg, "-psn_", 5)) {
            // Used by Mac OS app bundles; ignore it.
            continue;
        }

        if (arg[0] == '-') {
            message("Unrecognized option: '%s'", arg);
            usage();
            return 1;
        }

        // Other arguments are game binaries
        SystemMC::installGame(arg);
    }

    // Necessary even when running windowless, since we use GLFW for time
    glfwInit();

    return scriptFile ? runScript(sys, scriptFile) : run(sys);
}

extern "C" bool glfwSifteoOpenFile(const char *filename)
{
    // Entry point for platform-specific drag and drop in GLFW.
    return SystemMC::installGame(filename);
}
