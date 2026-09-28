/*
 * board.h - Brochage et constantes du télémètre ultrason (LPC1769)
 *
 * Horloges : CCLK = 100 MHz, PCLK = CCLK/4 = 25 MHz (TIMER0, TIMER1, UART0)
 */
#ifndef BOARD_H
#define BOARD_H

#include "LPC17xx.h"

/* ---- Brochage ------------------------------------------------------------ */
/* Émission : GPIO -> driver de grille -> MOSFET -> transducteur 40 kHz       */
#define TRIG_GPIO      LPC_GPIO2
#define TRIG_PIN       13u          /* P2.13 */

/* Réception : sortie comparateur LM311 -> entrée de capture CAP1.0         */
#define ECHO_PIN       18u          /* P1.18 = CAP1.0 (PINSEL3 = 11)          */

/* Interrupteurs (niveau haut = interrupteur ouvert, résistance de tirage)   */
/* P0.28 est open-drain et P0.29/P0.30 n'ont pas de pull-up interne :        */
/* résistances de tirage EXTERNES obligatoires sur ces trois broches.        */
#define SW_EN_PIN      30u          /* P0.30 : marche / arrêt                 */
#define SW_FREQ0_PIN   28u          /* P0.28 : fréquence de mesure, bit 0     */
#define SW_FREQ1_PIN   29u          /* P0.29 : fréquence de mesure, bit 1     */
#define SW_DBG0_PIN    30u          /* P1.30 : mode de debug, bit 0           */
#define SW_DBG1_PIN    31u          /* P1.31 : mode de debug, bit 1           */

#define LED_PIN        22u          /* P0.22 : LED allumée pendant la mesure  */

/* UART0 : P0.2 = TXD0, P0.3 = RXD0, 115200 bauds 8N1                        */

/* ---- Constantes de mesure ------------------------------------------------ */
#define PCLK_HZ        25000000u
#define TICKS_PER_US   (PCLK_HZ / 1000000u)       /* 25 ticks par µs (40 ns)  */

#define US_FREQ_HZ     40000u                     /* fréquence du transducteur*/
#define US_BURST_CYCLES 8u                        /* 8 périodes = 200 µs      */

#define SOUND_SPEED_M_S 343u                      /* à 20 °C                  */
#define MIN_CM         5u                         /* zone aveugle             */
#define MAX_CM         250u                       /* au-delà : hors portée    */

/* Aller-retour : t = 2d / c  ->  environ 58 µs par cm                       */
#define US_PER_CM      58u
#define BLANK_TICKS    (MIN_CM * US_PER_CM * TICKS_PER_US)   /* 290 µs  */
#define TIMEOUT_TICKS  (MAX_CM * US_PER_CM * TICKS_PER_US)   /* 14,5 ms */

#endif
