/*
 * startup_lpc17xx.c - Démarrage minimal pour arm-none-eabi-gcc
 * Table des vecteurs, copie de .data, mise à zéro de .bss, appel de main().
 */
#include <stdint.h>

extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss, _estack;
extern int main(void);

void Reset_Handler(void);
static void Default_Handler(void) { while (1) { } }

/* Gestionnaires faibles : redéfinis par le firmware quand ils sont utilisés */
void NMI_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void)     __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void)    __attribute__((weak, alias("Default_Handler")));
void TIMER0_IRQHandler(void)  __attribute__((weak, alias("Default_Handler")));
void TIMER1_IRQHandler(void)  __attribute__((weak, alias("Default_Handler")));
void UART0_IRQHandler(void)   __attribute__((weak, alias("Default_Handler")));

/* 16 vecteurs Cortex-M3 + 35 interruptions LPC17xx.
   Le mot 7 reçoit la somme de contrôle LPC (écrite par tools/lpc_checksum.py). */
__attribute__((section(".isr_vector"), used))
void (* const vector_table[16 + 35])(void) = {
    (void (*)(void))&_estack,
    Reset_Handler, NMI_Handler, HardFault_Handler, MemManage_Handler,
    BusFault_Handler, UsageFault_Handler, 0, 0, 0, 0,
    SVC_Handler, DebugMon_Handler, 0, PendSV_Handler, SysTick_Handler,
    /* IRQ 0..34 */
    Default_Handler,     /*  0 WDT    */
    TIMER0_IRQHandler,   /*  1 TIMER0 */
    TIMER1_IRQHandler,   /*  2 TIMER1 */
    Default_Handler,     /*  3 TIMER2 */
    Default_Handler,     /*  4 TIMER3 */
    UART0_IRQHandler,    /*  5 UART0  */
    Default_Handler, Default_Handler, Default_Handler, Default_Handler,
    Default_Handler, Default_Handler, Default_Handler, Default_Handler,
    Default_Handler, Default_Handler, Default_Handler, Default_Handler,
    Default_Handler, Default_Handler, Default_Handler, Default_Handler,
    Default_Handler, Default_Handler, Default_Handler, Default_Handler,
    Default_Handler, Default_Handler, Default_Handler, Default_Handler,
    Default_Handler, Default_Handler, Default_Handler, Default_Handler,
    Default_Handler,
};

void Reset_Handler(void)
{
    uint32_t *src = &_sidata, *dst = &_sdata;
    while (dst < &_edata) *dst++ = *src++;
    for (dst = &_sbss; dst < &_ebss; ) *dst++ = 0u;
    (void)main();                   /* main() appelle SystemInit() */
    while (1) { }
}
