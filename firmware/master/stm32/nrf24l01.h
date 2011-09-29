/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * Driver for the Nordic nRF24L01 radio
 */

#ifndef _NRF24L01_H
#define _NRF24L01_H

#include "spi.h"
#include "gpio.h"
#include "radio.h"


class NRF24L01 {
 public:
    NRF24L01(GPIOPin _ce,
             GPIOPin _irq,
             SPIMaster _spi)
        : ce(_ce), irq(_irq), spi(_spi),
          txBuffer(NULL, txData), rxBuffer(rxData)
          {}

    void init();
    void ptxMode();

    void isr();

 private:
    enum Command {
        CMD_R_REGISTER          = 0x00,
        CMD_W_REGISTER          = 0x20,
        CMD_R_RX_PAYLOAD        = 0x61,
        CMD_W_TX_PAYLOAD        = 0xA0,
        CMD_FLUSH_TX            = 0xE1,
        CMD_FLUSH_RX            = 0xE2,
        CMD_REUSE_TX_PL         = 0xE3,
        CMD_R_RX_PL_WID         = 0x60,
        CMD_W_ACK_PAYLOAD       = 0xA8,
        CMD_W_TX_PAYLOAD_NO_ACK = 0xB0,
        CMD_NOP                 = 0xFF,
    };

    enum Register {
        REG_CONFIG              = 0x00,
        REG_EN_AA               = 0x01,
        REG_EN_RXADDR           = 0x02,
        REG_SETUP_AW            = 0x03,
        REG_SETUP_RETR          = 0x04,
        REG_RF_CH               = 0x05,
        REG_RF_SETUP            = 0x06,
        REG_STATUS              = 0x07,
        REG_OBSERVE_TX          = 0x08,
        REG_RPD                 = 0x09,
        REG_RX_ADDR_P0          = 0x0A,
        REG_RX_ADDR_P1          = 0x0B,
        REG_RX_ADDR_P2          = 0x0C,
        REG_RX_ADDR_P3          = 0x0D,
        REG_RX_ADDR_P4          = 0x0E,
        REG_RX_ADDR_P5          = 0x0F,
        REG_TX_ADDR             = 0x10,
        REG_RX_PW_P0            = 0x11,
        REG_RX_PW_P1            = 0x12,
        REG_RX_PW_P2            = 0x13,
        REG_RX_PW_P3            = 0x14,
        REG_RX_PW_P4            = 0x15,
        REG_RX_PW_P5            = 0x16,
        REG_FIFO_STATUS         = 0x17,
        REG_DYNPD               = 0x1C,
        REG_FEATURE             = 0x1D,
    };

    enum Status {
        RX_DR                   = 1 << 6,
        TX_DS                   = 1 << 5,
        MAX_RT                  = 1 << 4,
    };

    static const unsigned SOFT_RETRY_MAX = 100;

    GPIOPin ce;
    GPIOPin irq;
    SPIMaster spi;

    PacketTransmission txBuffer;
    PacketBuffer rxBuffer;
    unsigned softRetriesLeft;

    /* XXX: Make these DMA-able */
    uint8_t txData[PacketBuffer::MAX_LEN];
    uint8_t rxData[PacketBuffer::MAX_LEN];   

    void handleTimeout();
    void receivePacket();
    void transmitPacket();
    void pulseCE();
};

#endif
