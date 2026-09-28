/* system_LPC17xx.h - interface CMSIS de system_LPC17xx.c */
#ifndef SYSTEM_LPC17xx_H
#define SYSTEM_LPC17xx_H
#include <stdint.h>
extern uint32_t SystemCoreClock;      /* fréquence CPU en Hz */
void SystemInit(void);                /* PLL0 -> CCLK = 100 MHz */
void SystemCoreClockUpdate(void);
#endif
