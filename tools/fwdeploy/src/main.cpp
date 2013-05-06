
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <climits>

#include "deployer.h"
#include "inspect.h"
#include "macros.h"

using namespace std;

static void usage()
{
    fprintf(stderr, "fwdeploy <out.sft> --fw-version <version> --fw <hwrev fw.bin> ...\n");
}

static void version()
{
    fprintf(stderr, "fwdeploy " TOSTRING(SDK_VERSION) "\n");
}

bool collectDetails(int argc, char **argv, Deployer::Container &container);

int main(int argc, char **argv)
{
    if (argc < 2) {
        usage();
        return 0;
    }

    if (!strcmp("-h", argv[1])) {
        usage();
        return 0;
    }

    if (!strcmp("-v", argv[1])) {
        version();
        return 0;
    }

    if (!strcmp("--inspect", argv[1])) {
        if (argc < 3) {
            fprintf(stderr, "need more args\n");
            return 1;
        }

        Inspect inspct;
        inspct.dumpSections(argv[2]);
        return 0;
    }

    Deployer deployer;
    bool success = false;

    // collect HW rev/FW bin pairs
    Deployer::Container container;
    if (collectDetails(argc, argv, container)) {
        container.outPath = argv[1];
        success = deployer.deploy(container);

    } else if (argc == 3) {

        // original form was simply 2 fixed position args
        success = deployer.deploySingle(argv[1], argv[2]);

    } else {
        fprintf(stderr, "incorrect args\n");
    }

    return success ? 0 : 1;
}

bool collectDetails(int argc, char **argv, Deployer::Container &container)
{
    for (int i = 2; i < argc; i++) {

        if (!strcmp("--fw-version", argv[i]) && (i + 1 < argc)) {
            container.fwVersion = argv[i + 1];
            i++;
            continue;
        }

        if (!strcmp("--fw", argv[i]) && (i + 2 < argc)) {

            const char *hwRev = argv[i + 1];
            const char *fwPath = argv[i + 2];

            unsigned long hwRevNum = strtoul(hwRev, NULL, 0);
            if (hwRevNum == 0 || hwRevNum == ULONG_MAX) {
                fprintf(stderr, "%s could not be converted to a hardware revision\n", hwRev);
                return 1;
            }

            container.firmwares.push_back(new Deployer::Firmware(hwRevNum, fwPath));

            i += 2;
            continue;
        }
    }

    return container.isValid();
}
