#ifndef FACTORYTEST_H
#define FACTORYTEST_H

#include <stdint.h>

class FactoryTest
{
public:
    static const unsigned UART_MAX_COMMAND_LEN = 6;
    static const unsigned UART_LEN_INDEX = 0;   // first byte is length
    static const unsigned UART_CMD_INDEX = 1;   // second byte is opcode

    enum Command {
        NrfComms        = 0,
        FlashComms      = 1,
        FlashReadWrite  = 2,
        Led             = 3,
        UniqueId        = 4
    };

    static void init();
    static void onUartIsr();
    static void onTestDataReceivedUSB(uint8_t *buf, unsigned len);

private:
    static uint8_t commandBuf[UART_MAX_COMMAND_LEN];
    static uint8_t commandLen;

    typedef void(*TestHandler)(uint8_t argc, const uint8_t *args);
    static const TestHandler handlers[];

    static void nrfCommsHandler(uint8_t argc, const uint8_t *args);
    static void flashCommsHandler(uint8_t argc, const uint8_t *args);
    static void flashReadWriteHandler(uint8_t argc, const uint8_t *args);
    static void ledHandler(uint8_t argc, const uint8_t *args);
    static void uniqueIdHandler(uint8_t argc, const uint8_t *args);
    static void volumeCalibrationHandler(uint8_t argc, const uint8_t *args);
    static void batteryCalibrationHandler(uint8_t argc, const uint8_t *args);
    static void homeButtonHandler(uint8_t argc, const uint8_t *args);
};

#endif // FACTORYTEST_H
