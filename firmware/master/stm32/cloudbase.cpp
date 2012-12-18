#include "cloudbase.h"
#include "usart.h"
#include "tasks.h"

FlashVolumeWriter CloudBase::writer;
CloudBase::RxBuf CloudBase::rxBuf;

void CloudBase::onUartIsr()
{
    uint8_t c;
    uint16_t status = Usart::Dbg.isr(c);

    if (status & Usart::StatusRxed) {
        processByte(c);
    }

    if (status & Usart::StatusOverrun) {
        // we're out of sync.
        rxBuf.reset();
    }
}


void CloudBase::task()
{
    processPacket();
}


void CloudBase::processByte(uint8_t c)
{
    // first check to see if this new byte was escaped by the previous one
    if (rxBuf.escaped) {
        if (c == ESC_END)
          c = END;
        else if (c == ESC_ESC)
          c = ESC;

        // if it was neither, the spec says it's malformed, but to drop it in the packet anyway
        rxBuf.append(c);
        return;
    }

    // otherwise, we're processing an unescaped byte
    switch (c) {
    case END:
        if (!rxBuf.synced) {
            rxBuf.reset();
            rxBuf.synced = true;
        }
        if (rxBuf.len) {
            Tasks::trigger(Tasks::CloudBase);
        }
        break;

    case ESC:
        rxBuf.escaped = true;
        break;

    default:
        rxBuf.append(c);
        break;
    }
}


void CloudBase::processPacket()
{
    switch (Opcode(rxBuf.opcode())) {

    case WriterHeader:
        if (rxBuf.payloadLen() >= 5) {
            const uint32_t numBytes = *reinterpret_cast<const uint32_t*>(rxBuf.payload());
            const char* packageStr = reinterpret_cast<const char*>(rxBuf.payload() + sizeof(numBytes));
            writer.beginGame(numBytes, packageStr);
        }
        break;

    case Payload:
        writer.appendPayload(rxBuf.payload(), rxBuf.payloadLen());
        break;

    case Commit:
        if (writer.isPayloadComplete()) {
            writer.commit();
            rxBuf.synced = false;
        }
        break;
    }

    // return the first byte of this packet's payload to give at least a little confidence that
    // we're transferring the data we think we are
    Usart::Dbg.put(rxBuf.payload()[0]);

    rxBuf.reset();
}
