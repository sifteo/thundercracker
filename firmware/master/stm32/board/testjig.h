/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _BOARD_TESTJIG_H
#define _BOARD_TESTJIG_H

// C L O C K
#define RCC_CFGR_PLLXTPRE   0

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

// F L A S H
#define FLASH_SPI           SPI1
#define FLASH_CS_GPIO       GPIOPin(&GPIOA, 15)
#define FLASH_WP_GPIO       GPIOPin(&GPIOB, 2)
#define FLASH_SCK_GPIO      GPIOPin(&GPIOB, 3)
#define FLASH_MISO_GPIO     GPIOPin(&GPIOB, 4)
#define FLASH_MOSI_GPIO     GPIOPin(&GPIOB, 5)
#define FLASH_REG_EN_GPIO   GPIOPin(&GPIOC, 4)

// N E I G H B O R
#define NBR_OUT3_GPIO       GPIOPin(&GPIOB, 8)
#define NBR_OUT4_GPIO       GPIOPin(&GPIOB, 9)
#define NBR_OUT1_GPIO       GPIOPin(&GPIOB, 1)  //this kills the green LED, but good for testing
#define NBR_OUT2_GPIO       GPIOPin(&GPIOB, 1)

// U A R T
#define UART_DBG            USART3
#define UART_RX_GPIO        GPIOPin(&GPIOB, 11)
#define UART_TX_GPIO        GPIOPin(&GPIOB, 10)

// L E D
#define LED_GREEN_GPIO      GPIOPin(&GPIOB, 0)
#define LED_RED_GPIO        GPIOPin(&GPIOB, 1)

// P O W E R
#define VCC20_ENABLE_GPIO   GPIOPin(&GPIOC, 13)
#define VCC33_ENABLE_GPIO   GPIOPin(&GPIOC, 14)

// A U D I O
#define AUDIO_DAC_GPIO      GPIOPin(&GPIOA, 4)
#define AUDIO_DAC_CHAN      1

#define AUDIO_PWMA_GPIO     GPIOPin(&GPIOA, 7)
#define AUDIO_PWMB_GPIO     GPIOPin(&GPIOA, 8)
#define AUDIO_PWM_CHAN      1

// M I S C
#define BTN_HOME_GPIO       GPIOPin(&GPIOC, 0)
#define BTN_HOME_EXTI_VEC   EXTI2


// T E S T  J I G

#define LED_GREEN1_GPIO     GPIOPin(&GPIOC,2)
#define LED_GREEN2_GPIO     GPIOPin(&GPIOC,3)
#define LED_RED1_GPIO       GPIOPin(&GPIOC,0)
#define LED_RED2_GPIO       GPIOPin(&GPIOC,1)

#define JIG_I2C             I2C1
#define JIG_SCL_GPIO        GPIOPin(&GPIOB, 6)
#define JIG_SDA_GPIO        GPIOPin(&GPIOB, 7)

#define JIG_NBR_IN1_GPIO    GPIOPin(&GPIOA, 0)
#define JIG_NBR_IN2_GPIO    GPIOPin(&GPIOA, 1)
#define JIG_NBR_IN3_GPIO    GPIOPin(&GPIOA, 2)
#define JIG_NBR_IN4_GPIO    GPIOPin(&GPIOA, 3)

#define JIG_NBR_OUT1_GPIO   GPIOPin(&GPIOA, 6)
#define JIG_NBR_OUT2_GPIO   GPIOPin(&GPIOA, 7)
#define JIG_NBR_OUT3_GPIO   GPIOPin(&GPIOB, 0)
#define JIG_NBR_OUT4_GPIO   GPIOPin(&GPIOB, 1)

#define USB_PWR_GPIO        GPIOPin(&GPIOB, 15)

#endif // _BOARD_TESTJIG_H
