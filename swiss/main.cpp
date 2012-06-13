
#include <stdio.h>
#include <string.h>

static void usage()
{
    fprintf(stderr, "swiss <cmd> <opts>\n");
}

static void version()
{
    fprintf(stderr, "swiss version\n");
}

int main(int argc, char **argv)
{
    for (unsigned i = 0; i < argc; ++i) {

        if (!strcmp(argv[i], "-h")) {
            usage();
            return 0;
        }

        if (!strcmp(argv[i], "-v")) {
            version();
            return 0;
        }
    }

    return 0;
}

