/*
 * ultrasonic.h - Émission de la salve 40 kHz et mesure du temps de vol
 */
#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#include <stdint.h>

typedef enum {
    US_NONE = 0,     /* pas de nouveau résultat          */
    US_OK,           /* écho reçu, temps de vol valide   */
    US_TIMEOUT       /* aucun écho avant MAX_CM          */
} us_result_t;

void        us_init(void);
int         us_start(void);                 /* 1 si la mesure est lancée   */
us_result_t us_poll(uint32_t *tof_ticks);   /* lit (et consomme) le résultat */
uint32_t    us_ticks_to_mm(uint32_t tof_ticks);

#endif
