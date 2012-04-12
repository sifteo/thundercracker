#include "factorytest.h"
#include "usart.h"
#include "board.h"

uint8_t FactoryTest::commandBuf[MAX_COMMAND_LEN];
uint8_t FactoryTest::commandLen;

void FactoryTest::init()
{
    commandLen = 0;
}

void FactoryTest::onUartIsr()
{
    uint16_t status = Usart::Dbg.isr(commandBuf + commandLen);
    if (status & Usart::STATUS_RXED) {
        commandLen++;

        // msg too long? error
        if (commandLen >= MAX_COMMAND_LEN) {
            commandLen = 0;
            return;
        }

        // msg complete? handle it
        if (commandBuf[COMMAND_LEN_IDX] == commandLen) {
            handleTestCommand(commandBuf, commandLen);
            commandLen = 0;
        }
    }
}

void FactoryTest::handleTestCommand(uint8_t *cmd, uint8_t len)
{

}

IRQ_HANDLER ISR_USART3()
{
    FactoryTest::onUartIsr();
}
