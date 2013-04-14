#ifndef OPTIONS_H
#define OPTIONS_H

class Options
{
public:
    Options();  // not implemented

    static unsigned processGlobalArgs(int argc, char **argv);

    static bool useNetDevice() {
        return useNetDev;
    }

private:
    static bool useNetDev;
};

#endif // OPTIONS_H
