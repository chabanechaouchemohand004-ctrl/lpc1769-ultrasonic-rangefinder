/*
 * main.c - Télémètre ultrason 40 kHz sur LPC1769 (bare-metal, Keil µVision)
 *
 *  TIMER0  : cadence des mesures (10 / 15 / 20 / 25 Hz selon 2 interrupteurs)
 *  TIMER1  : salve 40 kHz, zone aveugle, capture de l'écho, timeout (ultrasonic.c)
 *  UART0   : trames de debug (uart0.c)
 *
 *  Modes de debug (interrupteurs P1.31/P1.30, ou commande "DBGx" si 11) :
 *    00 -> distance         "T 102 cm"
 *    01 -> temps de vol brut en ticks de 40 ns   "T0x0243B0"
 *    10 -> fréquence de mesure                   "T 10 mes/sec"
 *    11 -> silencieux, commandes DBG0..DBG3 acceptées sur l'UART
 */
#include "board.h"
#include "ultrasonic.h"
#include "uart0.h"

static volatile uint8_t enabled;           /* interrupteur marche/arrêt   */
static uint8_t rate_mode = 0xFF;           /* 0..3, 0xFF = pas encore lu  */
static uint8_t debug_mode;                 /* 0..3                        */
static uint8_t uart_debug_mode = 3;        /* mode choisi par "DBGx"      */

static const uint32_t rate_period_us[4] = { 40000u, 50000u, 66667u, 100000u };
static const uint8_t  rate_hz[4]        = { 25u, 20u, 15u, 10u };

/* ---------------------------------------------------------------------- */
static void gpio_init(void)
{
    /* P0.28, P0.29, P0.30 en GPIO (PINSEL1), entrées */
    LPC_PINCON->PINSEL1 &= ~((3u << 24) | (3u << 26) | (3u << 28));
    LPC_GPIO0->FIODIR   &= ~((1u << SW_FREQ0_PIN) | (1u << SW_FREQ1_PIN) | (1u << SW_EN_PIN));

    /* P1.30, P1.31 en GPIO (PINSEL3), entrées avec pull-up interne (PINMODE = 00) */
    LPC_PINCON->PINSEL3  &= ~((3u << 28) | (3u << 30));
    LPC_PINCON->PINMODE3 &= ~((3u << 28) | (3u << 30));
    LPC_GPIO1->FIODIR    &= ~((1u << SW_DBG0_PIN) | (1u << SW_DBG1_PIN));

    /* P0.22 : LED, sortie */
    LPC_PINCON->PINSEL1 &= ~(3u << 12);
    LPC_GPIO0->FIOCLR    = (1u << LED_PIN);
    LPC_GPIO0->FIODIR   |= (1u << LED_PIN);
}

static uint8_t read_2bits(LPC_GPIO_TypeDef *port, uint32_t pin_lsb, uint32_t pin_msb)
{
    uint32_t v = port->FIOPIN;
    return (uint8_t)(((v >> pin_lsb) & 1u) | (((v >> pin_msb) & 1u) << 1));
}

/* ---------------------------------------------------------------------- */
static void timer0_init(void)
{
    LPC_SC->PCONP    |= (1u << 1);
    LPC_SC->PCLKSEL0 &= ~(3u << 2);          /* 25 MHz                    */
    LPC_TIM0->TCR = 2u;                      /* reset                      */
    LPC_TIM0->PR  = TICKS_PER_US - 1u;       /* 1 tick = 1 µs              */
    LPC_TIM0->MCR = 3u;                      /* interruption + reset sur MR0 */
    NVIC_SetPriority(TIMER0_IRQn, 1);
    NVIC_EnableIRQ(TIMER0_IRQn);
}

/* Changer MR0 seulement quand le mode change, timer remis à zéro :
   si MR0 devenait inférieur à TC, le compteur tournerait ~71 min avant le match. */
static void timer0_set_rate(uint8_t mode)
{
    LPC_TIM0->TCR = 2u;
    LPC_TIM0->MR0 = rate_period_us[mode];
    LPC_TIM0->TCR = 1u;
}

void TIMER0_IRQHandler(void)
{
    LPC_TIM0->IR = 1u;
    if (enabled && us_start())
        LPC_GPIO0->FIOSET = (1u << LED_PIN); /* LED allumée pendant la mesure */
}

/* ---------------------------------------------------------------------- */
static void report(us_result_t r, uint32_t tof)
{
    if (debug_mode == 3)
        return;

    if (r == US_TIMEOUT) {
        uart0_puts("HORS PORTEE\r\n");
        return;
    }

    switch (debug_mode) {
    case 0:
        uart0_puts("T ");
        uart0_putu((us_ticks_to_mm(tof) + 5u) / 10u);
        uart0_puts(" cm\r\n");
        break;
    case 1:
        uart0_puts("T0x");
        uart0_puthex(tof, 6);
        uart0_puts("\r\n");
        break;
    case 2:
        uart0_puts("T ");
        uart0_putu(rate_hz[rate_mode]);
        uart0_puts(" mes/sec\r\n");
        break;
    default:
        break;
    }
}

int main(void)
{
    uint32_t tof;
    us_result_t r;
    uint8_t m;
    int cmd;

    SystemInit();                            /* CCLK = 100 MHz */
    gpio_init();
    uart0_init();
    us_init();
    timer0_init();

    uart0_puts("Telemetre US 40 kHz - pret\r\n");

    while (1) {
        /* a) interrupteurs */
        enabled = (uint8_t)((LPC_GPIO0->FIOPIN >> SW_EN_PIN) & 1u);

        m = read_2bits(LPC_GPIO0, SW_FREQ0_PIN, SW_FREQ1_PIN);
        if (m != rate_mode) {
            rate_mode = m;
            timer0_set_rate(m);
        }

        cmd = uart0_dbg_command();
        if (cmd >= 0)
            uart_debug_mode = (uint8_t)cmd;

        m = read_2bits(LPC_GPIO1, SW_DBG0_PIN, SW_DBG1_PIN);
        debug_mode = (m == 3u) ? uart_debug_mode : m;

        /* b) résultat de mesure */
        r = us_poll(&tof);
        if (r != US_NONE) {
            LPC_GPIO0->FIOCLR = (1u << LED_PIN);
            report(r, tof);
        }

        /* c) envoi des trames en attente */
        uart0_flush();
    }
}
