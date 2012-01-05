#ifndef USB_H
#define USB_H

#include <stdint.h>

class Usb
{
public:
    enum Endpoints {
        Ep0Out  = 0x00,
        Ep0In   = 0x80,
        Ep1Out  = 0x01,
        Ep1In   = 0x81,
        Ep2Out  = 0x02,
        Ep2In   = 0x82,
        Ep3Out  = 0x03,
        Ep3In   = 0x83
    };

    static void init();

    static void handleINData(void *p);
    static void handleOUTData(void *p);

    static int read(uint8_t *buf, unsigned len);
    static int write(const uint8_t *buf, unsigned len);

private:

};

#endif // USB_H
