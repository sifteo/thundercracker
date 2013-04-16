#ifndef OPTIONS_H
#define OPTIONS_H

#include <string>

class Options
{
public:
    Options();  // not implemented

    static unsigned processGlobalArgs(int argc, char **argv);

    static bool useNetDevice() {
        return useNetDev;
    }

    static const char* netDevicePort() {
        return port.c_str();
    }

private:
    static bool useNetDev;
    static std::string port;
};

#endif // OPTIONS_H
