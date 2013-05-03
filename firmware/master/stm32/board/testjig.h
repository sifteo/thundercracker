/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _BOARD_TESTJIG_H
#define _BOARD_TESTJIG_H

// C L O C K
#if TEST_JIG_REV == 2
#define RCC_CFGR_PLLXTPRE   1
#elif TEST_JIG_REV == 1
#define RCC_CFGR_PLLXTPRE   0
#else
#error TEST_JIG_REV not configured
#endif

// U S B
#define USB_DM_GPIO         GPIOPin(&GPIOA, 11)
#define USB_DP_GPIO         GPIOPin(&GPIOA, 12)
#define USB_VBUS_GPIO       GPIOPin(&GPIOA, 9)

// N E I G H B O R
#define NBR_OUT1_GPIO       GPIOPin(&GPIOA, 0)
#define NBR_OUT2_GPIO       GPIOPin(&GPIOA, 2)
#define NBR_OUT3_GPIO       GPIOPin(&GPIOA, 1)
#define NBR_OUT4_GPIO       GPIOPin(&GPIOA, 3)

#define NBR_IN1_GPIO        GPIOPin(&GPIOA, 6)
#define NBR_IN2_GPIO        GPIOPin(&GPIOA, 7)
#define NBR_IN3_GPIO        GPIOPin(&GPIOB, 0)
#define NBR_IN4_GPIO        GPIOPin(&GPIOB, 1)

#define NBR_RX_TIM          TIM3
#define NBR_TX_TIM          TIM5
#define NBR_TX_TIM_CH       1

// XXX: these are not currently broken out separately per channel
#define NBR_BUF_GPIO        GPIOPin(&GPIOC, 8)

// U A R T
#define UART_DBG            USART3
#define UART_RX_GPIO        GPIOPin(&GPIOB, 11)
#define UART_TX_GPIO        GPIOPin(&GPIOB, 10)

// L E D
#define LED_GREEN1_GPIO     GPIOPin(&GPIOC,2)
#define LED_GREEN2_GPIO     GPIOPin(&GPIOC,3)
#define LED_RED1_GPIO       GPIOPin(&GPIOC,0)
#define LED_RED2_GPIO       GPIOPin(&GPIOC,1)

// P O W E R
#define USB_PWR_GPIO        GPIOPin(&GPIOB, 15)

// B A T T E R Y
#define BATTERY_SIM_GPIO    GPIOPin(&GPIOA, 5)
#define BATTERY_SIM_DAC_CH  2

// T E S T  J I G
#define DIP_SWITCH4_GPIO    GPIOPin(&GPIOC,12)
#define DIP_SWITCH3_GPIO    GPIOPin(&GPIOC,13)
#define DIP_SWITCH2_GPIO    GPIOPin(&GPIOC,14)
#define DIP_SWITCH1_GPIO    GPIOPin(&GPIOC,15)

#define JIG_I2C             I2C1
#define JIG_SCL_GPIO        GPIOPin(&GPIOB, 6)
#define JIG_SDA_GPIO        GPIOPin(&GPIOB, 7)

#define PWR_MEASURE_ADC     Adc::Adc1

#define USB_CURRENT_GPIO    GPIOPin(&GPIOA, 4)
#define USB_CURRENT_DIR_GPIO GPIOPin(&GPIOC, 5)
#define USB_CURRENT_ADC_CH  4

#define V3_CURRENT_GPIO     GPIOPin(&GPIOC, 4)
#define V3_CURRENT_DIR_GPIO GPIOPin(&GPIOB, 2)
#define V3_CURRENT_ADC_CH   14

#endif // _BOARD_TESTJIG_H
