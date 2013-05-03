
#include <stdio.h>
#include <string.h>

#include "deployer.h"
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

    // collect HW rev/FW bin pairs
    Deployer::Container container;
    container.outPath = argv[1];

    if (!collectDetails(argc, argv, container)) {
        usage();
        return 1;
    }

    Deployer deployer;
    bool success = deployer.deploy(container);

    printf("fwdeploy: %d\n", success);

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
