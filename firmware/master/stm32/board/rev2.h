/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef _BOARD_REV2_H
#define _BOARD_REV2_H

/*
 * Some boards have been reworked to move the flash's SPI peripheral
 * to one that isn't multiplexed with JTAG, so we can debug more easily.
 *
 * Disabled by default
 */
//#define REV2_GDB_REWORK

// C L O C K
#define RCC_CFGR_PLLXTPRE   1

// U S B
#define USB_DM_GPIO         GPIOPin(&GPIOA, 11)
#define USB_DP_GPIO         GPIOPin(&GPIOA, 12)
#define USB_VBUS_GPIO       GPIOPin(&GPIOA, 9)

// R A D I O
#define RF_SPI              SPI3
#define RF_CE_GPIO          GPIOPin(&GPIOC, 7)
#define RF_IRQ_GPIO         GPIOPin(&GPIOC, 8)
#define RF_SPI_CSN_GPIO     GPIOPin(&GPIOC, 9)
#define RF_SPI_SCK_GPIO     GPIOPin(&GPIOC, 10)
#define RF_SPI_MISO_GPIO    GPIOPin(&GPIOC, 11)
#define RF_SPI_MOSI_GPIO    GPIOPin(&GPIOC, 12)
#define RF_EXTI_VEC         EXTI9_5

#define RF_DMA_CHAN_RX      DMA2_Channel1
#define RF_DMA_CHAN_TX      DMA2_Channel2


// B L U E T O O T H
#ifdef HAVE_NRF8001

#define NRF8001_SPI         SPI2
#define NRF8001_SCK_GPIO    GPIOPin(&GPIOB, 13)
#define NRF8001_MISO_GPIO   GPIOPin(&GPIOB, 14)
#define NRF8001_MOSI_GPIO   GPIOPin(&GPIOB, 15)
#define NRF8001_REQN_GPIO   GPIOPin(&GPIOC, 2)
#define NRF8001_RDYN_GPIO   GPIOPin(&GPIOC, 3)
#define NRF8001_EXTI_VEC    EXTI3

#define NRF8001_DMA_CHAN_RX DMA1_Channel4
#define NRF8001_DMA_CHAN_TX DMA1_Channel5

#endif // HAVE_NRF8001

#define USE_NRF24L01        // Use the nRF42L01 as our radio

// F L A S H
#define USE_MX25L128        // Use the Macronix 128Mb flash part

#ifdef REV2_GDB_REWORK

#define FLASH_SPI           SPI2
#define FLASH_CS_GPIO       GPIOPin(&GPIOC, 3)
#define FLASH_WP_GPIO       GPIOPin(&GPIOB, 6)
#define FLASH_SCK_GPIO      GPIOPin(&GPIOB, 13)
#define FLASH_MISO_GPIO     GPIOPin(&GPIOB, 14)
#define FLASH_MOSI_GPIO     GPIOPin(&GPIOB, 15)
#define FLASH_REG_EN_GPIO   GPIOPin(&GPIOC, 4)

#define FLASH_DMA_CHAN_RX   DMA1_Channel4
#define FLASH_DMA_CHAN_TX   DMA1_Channel5

#else

#define FLASH_SPI           SPI1
#define FLASH_CS_GPIO       GPIOPin(&GPIOA, 15)
#define FLASH_WP_GPIO       GPIOPin(&GPIOB, 6)
#define FLASH_SCK_GPIO      GPIOPin(&GPIOB, 3)
#define FLASH_MISO_GPIO     GPIOPin(&GPIOB, 4)
#define FLASH_MOSI_GPIO     GPIOPin(&GPIOB, 5)
#define FLASH_REG_EN_GPIO   GPIOPin(&GPIOC, 4)

#define FLASH_DMA_CHAN_RX   DMA1_Channel2
#define FLASH_DMA_CHAN_TX   DMA1_Channel3

#endif // ENABLE_GDB

// N E I G H B O R
#define NBR_OUT1_GPIO       GPIOPin(&GPIOB, 8)
#define NBR_OUT2_GPIO       GPIOPin(&GPIOB, 9)
#define NBR_IN1_GPIO        GPIOPin(&GPIOA, 2)
#define NBR_IN2_GPIO        GPIOPin(&GPIOA, 3)
#define NBR_TX_TIM          TIM4                    // NOTE! same as BATT_LVL_TIM
#define NBR_TX_TIM_CH       3                       // CH3=PB8, CH4=PB9

// U A R T
#define UART_DBG            USART3
#define UART_RX_GPIO        GPIOPin(&GPIOB, 11)
#define UART_TX_GPIO        GPIOPin(&GPIOB, 10)

// L E D
#define LED_GREEN_GPIO      GPIOPin(&GPIOB, 0)
#define LED_RED_GPIO        GPIOPin(&GPIOB, 1)
#define LED_PWM_GREEN_CHAN  3
#define LED_PWM_RED_CHAN    4
#define LED_PWM_TIM         TIM3
#define LED_SEQUENCER_TIM   TIM6

// P O W E R
#define VCC20_ENABLE_GPIO   GPIOPin(&GPIOC, 0)
#define VCC33_ENABLE_GPIO   GPIOPin(&GPIOC, 1)

// A U D I O
#define AUDIO_PWMA_PORT     GPIOA
#define AUDIO_PWMA_PIN      7
#define AUDIO_PWMB_PORT     GPIOA
#define AUDIO_PWMB_PIN      8
#define AUDIO_PWM_CHAN      1
#define AUDIO_PWM_TIM       TIM1
#define AUDIO_SAMPLE_TIM    TIM7

// V O L U M E
#define USE_RC_FADER_MEAS

#define VOLUME_TIM          TIM5
#define VOLUME_CHAN         2
#define VOLUME_GPIO         GPIOPin(&GPIOA, 1)

// B A T T E R Y
#define USE_RC_BATT_MEAS
#define BATT_LVL_TIM        TIM4                    // NOTE! same as NBR_TX_TIM
#define BATT_LVL_CHAN       2
#define BATT_MEAS_GPIO      GPIOPin(&GPIOB,7)
#define BATT_MEAS_GND_GPIO  GPIOPin(&GPIOA,10)

// M I S C
#define BTN_HOME_GPIO       GPIOPin(&GPIOD, 2)
#define BTN_HOME_EXTI_VEC   EXTI2

#define PROFILER_TIM        TIM2

#endif // _BOARD_REV2_H
