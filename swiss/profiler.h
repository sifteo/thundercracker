#ifndef PROFILER_H
#define PROFILER_H

class Profiler
{
public:
    Profiler();

    // entry point for the 'profile' command
    static int run(int argc, char **argv);
};

#endif // PROFILER_H
