/*
 * uart0.h - Liaison série de debug (UART0, 115200 bauds, FIFO logicielle)
 */
#ifndef UART0_H
#define UART0_H

#include <stdint.h>

void uart0_init(void);
void uart0_puts(const char *s);
void uart0_putu(uint32_t v);                 /* entier décimal   */
void uart0_puthex(uint32_t v, uint8_t n);    /* n chiffres hexa  */
void uart0_flush(void);                      /* à appeler dans la boucle principale */

/* Dernière commande "DBGx" reçue (0..3), ou -1 si aucune */
int  uart0_dbg_command(void);

#endif
