#ifndef _BOOTLOADER_H
#define _BOOTLOADER_H

#include <stdint.h>

class Bootloader
{
public:
    static const uint32_t APPLICATION_ADDRESS = 0x8002000;

    Bootloader();
    void init();
    void exec();
    
private:
    bool load();
    bool updaterIsEnabled();
    bool mcuFlashIsValid();
    void jumpToApplication();
};

#endif // _BOOTLOADER_H
