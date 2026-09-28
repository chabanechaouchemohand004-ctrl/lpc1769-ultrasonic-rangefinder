/* Faux LPC17xx.h pour les tests sur PC : les registres sont de simples variables. */
#include <stdint.h>
typedef struct { volatile uint32_t PINSEL0,PINSEL1,PINSEL2,PINSEL3,PINSEL4,PINSEL5,PINSEL6,PINSEL7,PINSEL8,PINSEL9,PINSEL10,r[5],PINMODE0,PINMODE1,PINMODE2,PINMODE3,PINMODE4; } PINCON_T;
typedef struct { volatile uint32_t FIODIR,FIOMASK,FIOPIN,FIOSET,FIOCLR; } LPC_GPIO_TypeDef;
typedef struct { volatile uint32_t IR,TCR,TC,PR,PC,MCR,MR0,MR1,MR2,MR3,CCR,CR0,CR1,EMR,CTCR; } TIM_T;
typedef struct { volatile uint32_t PCONP,PCLKSEL0,PCLKSEL1; } SC_T;
typedef struct { volatile uint32_t RBR,THR,DLL,DLM,IER,FCR,LCR,LSR,FDR; } UART_T;
extern PINCON_T pincon; extern LPC_GPIO_TypeDef g0,g1,g2; extern TIM_T t0r,t1r; extern SC_T sc; extern UART_T u0;
#define LPC_PINCON (&pincon)
#define LPC_GPIO0 (&g0)
#define LPC_GPIO1 (&g1)
#define LPC_GPIO2 (&g2)
#define LPC_TIM0 (&t0r)
#define LPC_TIM1 (&t1r)
#define LPC_SC (&sc)
#define LPC_UART0 (&u0)
typedef enum { TIMER0_IRQn=1, TIMER1_IRQn=2, UART0_IRQn=5 } IRQn_Type;
static inline void NVIC_EnableIRQ(IRQn_Type i){(void)i;}
static inline void NVIC_SetPriority(IRQn_Type i,uint32_t p){(void)i;(void)p;}
