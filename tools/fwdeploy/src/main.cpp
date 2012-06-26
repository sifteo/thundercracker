
#include <stdio.h>
#include <string.h>

#include "deployer.h"

static void usage()
{
    fprintf(stderr, "fwdeploy <fw.bin> <out.sft>\n");
}

static void version()
{
    fprintf(stderr, "fwdeploy %s\n", SDK_VERSION);
}

int main(int argc, char **argv)
{
    const char *inPath = 0;
    const char *outPath = 0;

    for (int i = 1; i < argc; ++i) {

        if (!strcmp("-h", argv[i])) {
            usage();
            return 0;
        }

        if (!strcmp("-v", argv[i])) {
            version();
            return 0;
        }

        if (!inPath)
            inPath = argv[i];
        else
            outPath = argv[i];
    }

    if (!inPath || !outPath) {
        fprintf(stderr, "error: incorrect args\n");
        usage();
        return 1;
    }

    Deployer deployer;
    bool success = deployer.deploy(inPath, outPath);
    return success ? 0 : 1;
}
