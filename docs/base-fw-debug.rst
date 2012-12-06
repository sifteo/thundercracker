Debug Base Firmware
====================

Logging
--------

Logging is most easily done via uart. USART3 is the default debug UART, and is exposed on the flex connector along with the JTAG lines.

Sifteo programmer breakout boards bring the UART lines to a 6-pin header that's compatible with the FTDI serial cables: http://www.ftdichip.com/Products/Cables/USBTTLSerial.htm. Once you install the FTDI VCP (virtual COM port) drivers, (http://www.ftdichip.com/Drivers/VCP.htm), you can access the device as a virtual serial port, through pyserial (http://pyserial.sourceforge.net/pyserial_api.html), for example.

The ``UART()`` and ``UART_HEX()`` macros in macros.h make it convenient to insert logging statements in the code, and will only be included for firmware (ie, not simulator) builds.

See firmware/master/tools/serial-capture.py for a sample script to log and display serial data.

In Circuit Debugging
---------------------

Unfortunately, the base's design multiplexes SPI flash and JTAG GPIO lines, such that we must disable JTAG in order to access flash (ie, always).

Thus, modified hardware is required in order to debug using GDB - this chapter will be for another day.
