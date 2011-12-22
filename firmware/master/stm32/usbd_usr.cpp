/**
  ******************************************************************************
  * @file    usbd_usr.c
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    22-July-2011
  * @brief   This file includes the user application layer
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "hardware.h"

#include "usb_istr.h"

IRQ_HANDLER ISR_UsbOtg_FS() {
//    USBD_OTG_ISR_Handler(&USB_OTG_dev);
    STM32_PCD_OTG_ISR_Handler();
}

IRQ_HANDLER ISR_UsbOtg_FS_Wakeup() {
//    if(USB_OTG_dev.cfg.low_power) {
//        *(uint32_t *)(0xE000ED10) &= 0xFFFFFFF9 ;
//        SystemInit();
//        USB_OTG_UngateClock(&USB_OTG_dev);
//    }
//    EXTI_SIFTEO.PR = (1 << 18);    // clear usb wakeup interrupt
}
