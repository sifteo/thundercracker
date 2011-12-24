#ifndef USB_H
#define USB_H

#define EP0_OUT                    0x00
#define EP0_IN                     0x80
#define EP1_OUT                    0x01
#define EP1_IN                     0x81
#define EP2_OUT                    0x02
#define EP2_IN                     0x82
#define EP3_OUT                    0x03
#define EP3_IN                     0x83

class Usb
{
public:
    static void init();
};

#endif // USB_H
