#ifndef FACTORYTEST_H
#define FACTORYTEST_H

#include <stdint.h>

class FactoryTest
{
public:
    static const unsigned MAX_COMMAND_LEN = 10;
    static const unsigned COMMAND_LEN_IDX = 0;  // first byte is length
    FactoryTest();

    static void init();
    static void onUartIsr();
    static void onTestDataReceivedUSB(uint8_t *buf, unsigned len);

private:
    static void handleTestCommand(uint8_t *cmd, uint8_t len);

    static uint8_t commandBuf[MAX_COMMAND_LEN];
    static uint8_t commandLen;
};

#endif // FACTORYTEST_H
