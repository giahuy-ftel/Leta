#ifndef __STM32F1XX_H__
#define __STM32F1XX_H__

#include "stm32f10x.h"

#ifndef USB_BASE
#define USB_BASE      (APB1PERIPH_BASE + 0x00005C00UL)
#endif

#ifndef USB_PMAADDR
#define USB_PMAADDR   (APB1PERIPH_BASE + 0x00006000UL)
#endif

#endif
