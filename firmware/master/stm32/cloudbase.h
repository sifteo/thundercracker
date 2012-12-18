#ifndef CLOUDBASE_H
#define CLOUDBASE_H

#include "flash_volume.h"

#include <stdint.h>

class CloudBase
{
public:
    CloudBase(); // do not implement

    static void onUartIsr();
    static void task();

private:
    static const unsigned BUF_SIZE = 64;

    // SLIP codes
    static const uint8_t END            = 0300;     // indicates end of packet
    static const uint8_t ESC            = 0333;     // indicates byte stuffing
    static const uint8_t ESC_END        = 0334;     // ESC ESC_END means END data byte
    static const uint8_t ESC_ESC        = 0335;     // ESC ESC_ESC means ESC data byte

    enum Opcode {
        WriterHeader,
        Payload,
        Commit
    };

    struct RxBuf {
        uint8_t data[BUF_SIZE];
        uint8_t len;
        bool escaped;
        bool synced;

        void reset() {
            len = 0;
            escaped = false;
        }

        const uint8_t *payload() const {
            return &data[1];
        }

        uint8_t opcode() const {
            return data[0];
        }

        uint8_t payloadLen() const {
            if (len == 0) {
                return 0;
            }
            return len - 1;
        }

        void append(uint8_t c) {
            if (len < sizeof(data)) {
                data[len++] = c;
            }
            escaped = false;
        }
    };

    static RxBuf rxBuf;
    static FlashVolumeWriter writer;

    static void processByte(uint8_t c);
    static void processPacket();
};

#endif // CLOUDBASE_H
