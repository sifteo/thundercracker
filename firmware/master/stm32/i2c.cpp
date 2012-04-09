/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "i2c.h"

uint8_t Tx_Idx1=0, Rx_Idx1=0;
uint8_t Tx_Idx2=0, Rx_Idx2=0;
uint32_t NumbOfBytes1;
uint32_t NumbOfBytes2;
uint8_t Buffer_Rx1[1];
uint8_t Buffer_Tx1[1];
uint8_t Buffer_Rx2[1];
uint8_t Buffer_Tx2[1];

#if 0
void I2C_Init(volatile I2C_t *I2Cx, I2C_InitTypeDef* I2C_InitStruct)
{
    uint16_t tmpreg = 0;
    uint16_t freqrange = 0;

    tmpreg = I2Cx->CR2;
    tmpreg &= CR2_FREQ_Reset;
    freqrange = 8;
    tmpreg |= freqrange;
    I2Cx->CR2 = tmpreg;
    I2Cx->CR1 &= CR1_PE_Reset;
    tmpreg = 0;

    I2Cx->TRISE&=0xffc0;
    I2Cx->TRISE|=0x0b;


    I2Cx->CCR = tmpreg;

    I2Cx->CR1 |= CR1_PE_Set;

    tmpreg = I2Cx->CR1;
    tmpreg &= CR1_CLEAR_Mask;
    tmpreg |= (uint16_t)((uint32_t)I2C_InitStruct->I2C_Mode | I2C_InitStruct->I2C_Ack);
    I2Cx->CR1 = tmpreg;

    I2Cx->OAR1 = (I2C_InitStruct->I2C_AcknowledgedAddress | I2C_InitStruct->I2C_OwnAddress1);
}
#endif

/**
  * @brief  Fills each I2C_InitStruct member with its default value.
  * @param  I2C_InitStruct: pointer to an I2C_InitTypeDef structure which will be initialized.
  * @retval None
  */


//#define I2C_AcknowledgedAddress_7bit    ((uint16_t)0x4000)
//#define  I2C_Direction_Transmitter      ((uint8_t)0x00)
//#define  I2C_Direction_Receiver         ((uint8_t)0x01)
//#define I2C_Ack_Enable                  ((uint16_t)0x0400)
//#define I2C_Ack_Disable                 ((uint16_t)0x0000)
//#define I2C_DutyCycle_2                 ((uint16_t)0xBFFF)
//#define I2C_Mode_I2C                    ((uint16_t)0x0000)

void I2CSlave::init(GPIOPin scl, GPIOPin sda)
{
    // enable i2c clock & ISRs
    if (hw == &I2C1) {
        RCC.APB2ENR |= (1 << 21);

        NVIC.irqEnable(IVT.I2C1_EV);
        NVIC.irqPrioritize(IVT.I2C1_EV, 0x0);    //  Set to highest priority

        NVIC.irqEnable(IVT.I2C1_ER);
        NVIC.irqPrioritize(IVT.I2C1_ER, 0x1);    //  Just a little lower
    }
    else if (hw == &I2C2) {
        RCC.APB2ENR |= (1 << 22);
    }

//    I2C_InitTypeDef  I2C_InitStructure;

    scl.setControl(GPIOPin::OUT_ALT_OPEN_50MHZ);
    sda.setControl(GPIOPin::OUT_ALT_OPEN_50MHZ);

    // TODO: zap ST lib remnants
    /* I2C1 and I2C2 configuration */
//    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
//    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
//    I2C_InitStructure.I2C_OwnAddress1 = 10;
//    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
//    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
//    I2C_InitStructure.I2C_ClockSpeed = ClockSpeed;
//    I2C_Init(&I2C1, &I2C_InitStructure);

    // reset the peripheral
    hw->CR1 = (1 << 15);
    hw->CR1 = 0;

    // select ISRs we're interested in
    hw->CR2 |= (1 << 9) |   // ITEVTEN: event ISR
               (1 << 8) |   // ITERREN: error ISR
               (1 << 10);   // ITBUFEN: buffer ISR

    hw->CR1 &= ~((1 << 3) | (1 << 2));  // set i2c mode: clear smbus bits

    hw->CR1 |= (1 << 0);    // enable peripheral
}

/*
 * Error interrupt handler.
 * Not handling errors at the moment other than for debug.
 */
void I2CSlave::isr_ER()
{
    uint32_t status = hw->SR1;
    hw->SR1 = 0;

    if (status & (1 << 10)) {   // AF: acknowledge failure

    }

    if (status & (1 << 9)) {    // ARLO: arbitrarion lost

    }

    if (status & (1 << 8)) {    // BERR: bus error

    }

    if (status & (1 << 11)) {   // OVR: Overrun/underrun

    }
}

void I2CSlave::isr_EV()
{
    uint32_t status1 = hw->SR1;
    uint32_t status2 = hw->SR2;

    // ensure we're not operating as an i2c master
    // TODO: ditch this check if not necessary
    if (status2 & 0x1)
        return;

    // ADDR: Address match for incoming transaction.
    if (status1 & (1 << 1)) {
        /* Initialize the transmit/receive counters for next transmission/reception
        using Interrupt  */
        Tx_Idx1 = 0;
        Rx_Idx1 = 0;
    }

    // TXE: data register empty, time to transmit next byte
    if (status1 & (1 << 7)) {
        hw->DR = nextTest;
    }

    // RXNE: data register not empty, byte has arrived
    if (status1 & (1 << 6)) {
        Buffer_Rx1[Rx_Idx1++] = hw->DR;
    }

    // STOPF: Slave has detected a STOP condition on the bus
    if (status1 & (1 << 4)) {
        hw->CR1 |= (1 << 0);  // why is this enabling the peripheral?
    }
}

void I2CSlave::setNextTest(uint8_t test)
{
    nextTest = test;
}
