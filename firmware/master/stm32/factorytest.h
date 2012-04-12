#ifndef FACTORYTEST_H
#define FACTORYTEST_H

#include <stdint.h>

class FactoryTest
{
public:
    static const unsigned MAX_COMMAND_LEN = 6;
    static const unsigned LEN_INDEX = 0;  // first byte is length
    static const unsigned CMD_INDEX = 1;  // second byte is opcode

    enum Command {
        NrfComms        = 0,
        FlashComms      = 1,
        FlashReadWrite  = 2
    };

    static void init();
    static void onUartIsr();
    static void onTestDataReceivedUSB(uint8_t *buf, unsigned len);

private:
    static uint8_t commandBuf[MAX_COMMAND_LEN];
    static uint8_t commandLen;

    typedef void(*TestHandler)(uint8_t argc, uint8_t *args);
    static const TestHandler handlers[];

    static void nrfCommsHandler(uint8_t argc, uint8_t *args);
    static void flashCommsHandler(uint8_t argc, uint8_t *args);
    static void flashReadWriteHandler(uint8_t argc, uint8_t *args);
};

#endif // FACTORYTEST_H
