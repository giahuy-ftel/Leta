#include <stdint.h>

#include "system.h"

extern void dcd_int_handler(uint8_t rhport);

void SysTick_Handler(void)
{
	system_tick();
}

void USB_HP_CAN1_TX_IRQHandler(void)
{
	dcd_int_handler(0);
}

void USB_LP_CAN1_RX0_IRQHandler(void)
{
	dcd_int_handler(0);
}
