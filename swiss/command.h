#ifndef COMMAND_H
#define COMMAND_H

typedef int (*CommandRun)(int argc, char **argv);

struct Command {
    const char *name;
    const char *description;
    const char *usage;
    CommandRun run;
};

#endif // COMMAND_H
