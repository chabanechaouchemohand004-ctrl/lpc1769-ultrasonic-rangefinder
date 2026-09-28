/*
 * ultrasonic.c - Télémètre ultrason : émission + mesure du temps de vol
 *
 * Tout repose sur TIMER1, qui tourne librement à 25 MHz (1 tick = 40 ns) :
 *
 *   t0            t0+200 µs        t0+290 µs                 t0+14,5 ms
 *   |-- salve --->|--- zone aveugle ->|---- écoute (capture) ---->| timeout
 *     MR0 : 16 fronts   MR0 : fin         CAP1.0 : front montant      MR1
 *     sur P2.13         du masquage       du comparateur LM311
 *
 *  - MR0 génère la salve : un basculement de P2.13 toutes les 12,5 µs
 *    (312 puis 313 ticks, soit exactement 40,000 kHz en moyenne).
 *  - Pendant la zone aveugle, la capture est désactivée : on ignore le
 *    couplage direct émetteur -> récepteur et la résonance du transducteur.
 *  - Temps de vol = CR0 - t0 (instant du front montant du comparateur).
 *  - MR1 arrête l'écoute si aucun écho n'arrive avant MAX_CM.
 *
 * Aucune attente active : toute la séquence se déroule en interruption.
 */
#include "ultrasonic.h"
#include "board.h"

/* Bits des registres TIMER */
#define IR_MR0    (1u << 0)
#define IR_MR1    (1u << 1)
#define IR_CR0    (1u << 4)
#define MCR_MR0I  (1u << 0)
#define MCR_MR1I  (1u << 3)
#define CCR_RISE  (1u << 0)    /* capture sur front montant de CAP1.0 */
#define CCR_INT   (1u << 2)    /* interruption sur capture            */

#define HALF_PERIOD_TICKS (PCLK_HZ / US_FREQ_HZ / 2u)      /* 312 (312,5) */
#define BURST_EDGES       (2u * US_BURST_CYCLES)           /* 16 fronts   */

typedef enum { ST_IDLE, ST_BURST, ST_BLANK, ST_LISTEN, ST_DONE } us_state_t;

static volatile us_state_t  state = ST_IDLE;
static volatile us_result_t result = US_NONE;
static volatile uint32_t    t0;          /* début de la salve (ticks)   */
static volatile uint32_t    tof;         /* temps de vol (ticks)        */
static volatile uint32_t    edges;       /* fronts de salve déjà émis   */

void us_init(void)
{
    /* P2.13 en GPIO, sortie, niveau bas */
    LPC_PINCON->PINSEL4 &= ~(3u << 26);
    TRIG_GPIO->FIOCLR   = (1u << TRIG_PIN);
    TRIG_GPIO->FIODIR  |= (1u << TRIG_PIN);

    /* P1.18 en CAP1.0 (fonction 11) */
    LPC_PINCON->PINSEL3 |= (3u << 4);

    /* TIMER1 : alimentation, PCLK = CCLK/4, pas de prescaler (25 MHz) */
    LPC_SC->PCONP    |= (1u << 2);
    LPC_SC->PCLKSEL0 &= ~(3u << 4);
    LPC_TIM1->TCR  = 2u;            /* reset */
    LPC_TIM1->CTCR = 0u;            /* mode timer */
    LPC_TIM1->PR   = 0u;
    LPC_TIM1->MCR  = 0u;            /* pas de reset sur match : TC libre */
    LPC_TIM1->CCR  = 0u;
    LPC_TIM1->IR   = 0x3Fu;
    LPC_TIM1->TCR  = 1u;            /* démarrage */

    NVIC_SetPriority(TIMER1_IRQn, 0);   /* le plus prioritaire : timing de la salve */
    NVIC_EnableIRQ(TIMER1_IRQn);
}

int us_start(void)
{
    if (state != ST_IDLE)
        return 0;

    edges  = 0;
    result = US_NONE;
    t0 = LPC_TIM1->TC + 50u;        /* premier front 2 µs plus tard */

    LPC_TIM1->MR0 = t0;
    LPC_TIM1->MR1 = t0 + TIMEOUT_TICKS;
    LPC_TIM1->IR  = IR_MR0 | IR_MR1 | IR_CR0;
    state = ST_BURST;
    LPC_TIM1->MCR = MCR_MR0I;       /* la salve démarre au match MR0 */
    return 1;
}

static void finish(us_result_t r)
{
    LPC_TIM1->MCR = 0u;
    LPC_TIM1->CCR = 0u;
    result = r;
    state  = ST_DONE;
}

void TIMER1_IRQHandler(void)
{
    uint32_t ir = LPC_TIM1->IR;
    LPC_TIM1->IR = ir;              /* acquittement (écriture de 1) */

    if ((ir & IR_MR0) && state == ST_BURST) {
        edges++;
        if (edges & 1u)                          /* front montant / descendant */
            TRIG_GPIO->FIOSET = (1u << TRIG_PIN);
        else
            TRIG_GPIO->FIOCLR = (1u << TRIG_PIN);
        if (edges < BURST_EDGES) {
            /* 312 puis 313 ticks : période moyenne de 25 µs exactement */
            LPC_TIM1->MR0 += HALF_PERIOD_TICKS + (edges & 1u);
        } else {
            TRIG_GPIO->FIOCLR = (1u << TRIG_PIN);
            LPC_TIM1->MR0 = t0 + BLANK_TICKS;    /* fin de la zone aveugle */
            state = ST_BLANK;
        }
    }
    else if ((ir & IR_MR0) && state == ST_BLANK) {
        /* on commence à écouter : capture du front montant du comparateur */
        LPC_TIM1->IR  = IR_CR0;
        LPC_TIM1->CCR = CCR_RISE | CCR_INT;
        LPC_TIM1->MCR = MCR_MR1I;
        state = ST_LISTEN;
    }
    else if (state == ST_LISTEN) {
        if (ir & IR_CR0) {
            tof = LPC_TIM1->CR0 - t0;
            finish(US_OK);
        } else if (ir & IR_MR1) {
            finish(US_TIMEOUT);
        }
    }
}

us_result_t us_poll(uint32_t *tof_ticks)
{
    us_result_t r;

    if (state != ST_DONE)
        return US_NONE;

    r = result;
    if (tof_ticks)
        *tof_ticks = tof;
    state = ST_IDLE;
    return r;
}

uint32_t us_ticks_to_mm(uint32_t tof_ticks)
{
    /* d = c * t / 2, t = ticks / 25e6  ->  d_mm = ticks * 343 / 50 000
       (tof max ~ 362 500 ticks : le produit tient sur 32 bits) */
    return (tof_ticks * SOUND_SPEED_M_S) / (2u * PCLK_HZ / 1000u);
}
