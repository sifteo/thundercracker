/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/*
 * Command line front-end.
 */

#ifdef _WIN32
    #define _WIN32_WINNT 0x0501
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    static bool hasConsole = true;
#else
    #include <unistd.h>
#endif
 
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "frontend.h"
#include "system.h"
#include "ostime.h"
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
            "Sifteo Hardware Emulator (" TOSTRING(SDK_VERSION) ")\n"
            "\n"
            "Options:\n"
            "  -h                    Show this help message, and exit\n"
            "  -n NUM                Set initial number of cubes\n"
            "  -T                    Turbo mode; run faster than real-time if we can\n"
            "  -F FLASH.bin          Persistently keep all flash memory in a file on disk\n"
            "  -P PORT               Run a GDB debug server on the specified TCP port number\n"
            "  -e SCRIPT.lua         Execute a Lua script instead of the default frontend\n"
            "  -l LAUNCHER.elf       Start the supplied binary as the system launcher\n"
            "\n"
            "  --headless            Run without graphics or sound output\n"
            "  --lock-rotation       Lock rotation by default\n"
            "  --mute                Mute the Base's volume control by default\n"
            "  --paint-trace         Trace the state of the repaint controller\n"
            "  --radio-trace         Trace all radio packet contents\n"
            "  --radio-noise FLOAT   Simulated radio noise, arbitrary units.\n"     
            "  --stdout FILENAME     Redirect output to FILENAME\n"
            "  --svm-trace           Trace SVM instruction execution\n"
            "  --svm-stack           Monitor SVM stack usage\n"
            "  --svm-flash-stats     Dump statistics about flash memory usage\n"
            "  --waveout FILE.wav    Log all audio output to LOG.wav\n"
            "  --white-bg            Force the UI to use a plain white background\n"
            "  --window WxH          Initial window size (default 800x600)\n"
            "  --flush-logs          fflush stdout individual game logs to use them like a tail\n"
            "\n"
            "Games:\n"
            "  Any games specified on the command line will be installed to\n"
            "  flash prior to starting the simulation. By default, a built-in\n"
            "  version of the system Launcher is used. This can be overridden\n"
            "  with the -l option.\n"
            "\n"
            APP_COPYRIGHT_ASCII "\n");
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

static void setWorkingDir()
{
    /*
     * Normally we respect the working directory that we were run with.
     * Some GUI environments, like Mac OS X, will leave this set to a pretty
     * useless value, however, like "/". Detect that, and set a platform-specific
     * default instead. Note that this has no effect if the user is already
     * running siftulator on the command line from a more meaningful directory.
     *
     * Right now, this is most important for saving files such as screenshots
     * and trace logs, or any output files left by Lua scripts.
     */

    #ifdef __APPLE__
        char path[1024];
        char *home = getenv("HOME");
        if (home && getcwd(path, sizeof path) && path[1] == '\0') {
            chdir(home);
            chdir("Desktop");
        }
    #endif
}

static int run(System &sys)
{
    /*
     * NB: The Frontend is dynamically allocated to fix a bad ordering dependency
     *     during shutdown via exit(). Static C++ destructors will start getting
     *     called before we're guaranteed to be out of the Frontend main loop,
     *     and these destructors will start tearing down Box2D state too soon.
     */
    Frontend *fe = new Frontend;

    if (!sys.init()) {
        message("Emulator failed to initialize");
        return 1;
    }

    if (!sys.opt_headless && !fe->init(&sys)) {
        message("Graphical frontend failed to initialize");
        return 1;
    }    

    sys.start();

    if (sys.opt_headless) {
        while (sys.isRunning())
            OSTime::sleep(0.1);
    } else {
        while (fe->runFrame());
        fe->exit();
    }

    sys.exit();
    delete fe;

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
    System& sys = System::getInstance();
    const char *scriptFile = NULL;

    // Attach an existing console, if it's already handy
    getConsole();

    // Set the current directory to something more useful
    setWorkingDir();

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

        if (!strcmp(arg, "--mute")) {
            sys.opt_mute = true;
            continue;
        }

        if (!strcmp(arg, "--radio-noise") && argv[c+1]) {
            sys.opt_radioNoise = atof(argv[c+1]);
            c++;
            continue;
        }

        if (!strcmp(arg, "--window") && argv[c+1]) {
            int result = sscanf(argv[c+1], "%dx%d", &(sys.opt_windowWidth), &(sys.opt_windowHeight));
            if (result != 2 || sys.opt_windowWidth <= 0 || sys.opt_windowHeight <=0) {
                message("Error: invalid window size argument \"%s\"", argv[c+1]);
                return 1;
            }
            c++;
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

        if (!strcmp(arg, "--flush-logs")) {
            sys.opt_flushLogs = true;
            continue;
        }

        if (!strcmp(arg, "--waveout") && argv[c+1]) {
            sys.opt_waveoutFilename = argv[c+1];
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

    return scriptFile ? runScript(sys, scriptFile) : run(sys);
}

extern "C" bool glfwSifteoOpenFile(const char *filename)
{
    // Entry point for platform-specific drag and drop in GLFW.
    return SystemMC::installGame(filename);
}
