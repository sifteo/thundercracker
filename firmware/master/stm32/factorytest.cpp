#include "factorytest.h"
#include "usart.h"
#include "board.h"
#include "macros.h"

#include "radio.h"
#include "flash.h"

uint8_t FactoryTest::commandBuf[MAX_COMMAND_LEN];
uint8_t FactoryTest::commandLen;

/*
 * Table of test handlers.
 * Order must match the Command enum.
 */
FactoryTest::TestHandler const FactoryTest::handlers[] = {
    nrfCommsHandler,
    flashCommsHandler,
    flashReadWriteHandler
};

void FactoryTest::init()
{
    commandLen = 0;
}

/*
 * A new byte has arrived over the UART.
 * We're assembling a test command that looks like [len, opcode, params...]
 * If this byte overruns our buffer, reset our count of bytes received.
 * Otherwise, dispatch to the appropriate handler when a complete message
 * has been received.
 */
void FactoryTest::onUartIsr()
{
    uint8_t rxbyte;
    uint16_t status = Usart::Dbg.isr(&rxbyte);
    if (status & Usart::STATUS_RXED) {

        // avoid overflow - reset
        if (commandLen >= MAX_COMMAND_LEN)
            commandLen = 0;

        commandBuf[commandLen++] = rxbyte;

        if (commandBuf[LEN_INDEX] == commandLen) {
            // dispatch to the appropriate handler
            uint8_t cmd = commandBuf[CMD_INDEX];
            if (cmd < arraysize(handlers)) {
                TestHandler handler = handlers[cmd];
                // arg[0] is always the command byte
                handler(commandLen - 1, commandBuf + 1);
            }

            commandLen = 0;
        }
    }
}

/**************************************************************************
 * T E S T  H A N D L E R S
 *
 * Each handler is passed a list of arguments, the first of which is always
 * the command opcode itself.
 *
 **************************************************************************/

void FactoryTest::nrfCommsHandler(uint8_t argc, uint8_t *args)
{
    Radio::TxPower pwr = static_cast<Radio::TxPower>(args[1]);
    Radio::setTxPower(pwr);

    const uint8_t response[] = { 3, args[0], Radio::txPower() };
    Usart::Dbg.write(response, sizeof response);
}

void FactoryTest::flashCommsHandler(uint8_t argc, uint8_t *args)
{
    Flash::JedecID id;
    Flash::readId(&id);

    uint8_t result = (id.manufacturerID == Flash::MACRONIX_MFGR_ID) ? 1 : 0;

    const uint8_t response[] = { 3, args[0], result };
    Usart::Dbg.write(response, sizeof response);
}

void FactoryTest::flashReadWriteHandler(uint8_t argc, uint8_t *args)
{

}

IRQ_HANDLER ISR_USART3()
{
    FactoryTest::onUartIsr();
}
