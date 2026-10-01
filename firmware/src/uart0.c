/*
 * uart0.c - UART0 : émission non bloquante + réception de commandes "DBGx"
 *
 * Émission : les trames sont écrites dans une FIFO circulaire de 256 octets,
 * vidée dans la boucle principale vers la FIFO matérielle (16 octets).
 * Aucune fonction d'envoi n'attend l'UART : la mesure n'est jamais retardée.
 *
 * Réception (interruption) : la commande "DBG0".."DBG3" + Entrée change le
 * mode de debug quand les interrupteurs DBG sont en position 11.
 */
#include "uart0.h"
#include "board.h"

#define LSR_RDR   (1u << 0)
#define LSR_THRE  (1u << 5)
#define HW_FIFO   16u

#define TX_SIZE   256u                       /* puissance de 2 */
static uint8_t  tx_buf[TX_SIZE];
static uint16_t tx_w, tx_r;                  /* utilisés hors interruption uniquement */

static volatile int dbg_cmd = -1;

void uart0_init(void)
{
    LPC_SC->PCONP    |= (1u << 3);           /* UART0 alimenté            */
    LPC_SC->PCLKSEL0 &= ~(3u << 6);          /* PCLK = CCLK/4 = 25 MHz    */

    /* P0.2 = TXD0, P0.3 = RXD0 (fonction 01) */
    LPC_PINCON->PINSEL0 &= ~((3u << 4) | (3u << 6));
    LPC_PINCON->PINSEL0 |=  ((1u << 4) | (1u << 6));

    /* Débit = PCLK / (16 x (256 x DLM + DLL) x (1 + DIVADDVAL / MULVAL))
     *       = 25 MHz / (16 x 12 x (1 + 1/8)) = 115 741 bauds (+0,5 % vs 115 200)
     * DLL = 12, MULVAL = 8, DIVADDVAL = 1 (contrainte du LPC17xx : MULVAL >= 1,
     * DIVADDVAL < MULVAL). */
    LPC_UART0->LCR = 0x83;                   /* 8N1 (bits 1:0 = 11), DLAB = 1 : accès aux diviseurs */
    LPC_UART0->DLM = 0;
    LPC_UART0->DLL = 12;
    LPC_UART0->FDR = (8u << 4) | 1u;         /* MULVAL = 8, DIVADDVAL = 1 */
    LPC_UART0->LCR = 0x03;                   /* 8N1, DLAB = 0              */
    LPC_UART0->FCR = 0x07;                   /* bit 0 : FIFO activées, bits 1-2 : vidage RX et TX */
    LPC_UART0->IER = 1u;                     /* interruption en réception */

    NVIC_SetPriority(UART0_IRQn, 2);
    NVIC_EnableIRQ(UART0_IRQn);
}

static void put(uint8_t c)
{
    uint16_t next = (uint16_t)((tx_w + 1u) & (TX_SIZE - 1u));
    if (next != tx_r) {                      /* FIFO pleine : octet perdu */
        tx_buf[tx_w] = c;
        tx_w = next;
    }
}

void uart0_puts(const char *s)
{
    while (*s)
        put((uint8_t)*s++);
}

void uart0_putu(uint32_t v)
{
    char d[10];
    uint8_t n = 0;

    do {
        d[n++] = (char)('0' + v % 10u);
        v /= 10u;
    } while (v);
    while (n)
        put((uint8_t)d[--n]);
}

void uart0_puthex(uint32_t v, uint8_t n)
{
    static const char hex[] = "0123456789ABCDEF";
    while (n--)
        put((uint8_t)hex[(v >> (4u * n)) & 0xFu]);
}

void uart0_flush(void)
{
    uint8_t k;

    if (!(LPC_UART0->LSR & LSR_THRE))
        return;
    /* THRE = FIFO matérielle vide : on peut y déposer 16 octets d'un coup */
    for (k = 0; k < HW_FIFO && tx_r != tx_w; k++) {
        LPC_UART0->THR = tx_buf[tx_r];
        tx_r = (uint16_t)((tx_r + 1u) & (TX_SIZE - 1u));
    }
}

int uart0_dbg_command(void)
{
    int c = dbg_cmd;
    dbg_cmd = -1;
    return c;
}

void UART0_IRQHandler(void)
{
    static char    cmd[4];
    static uint8_t pos;

    while (LPC_UART0->LSR & LSR_RDR) {
        char c = (char)LPC_UART0->RBR;

        if (c == '\r' || c == '\n') {
            if (pos == 4 && cmd[0] == 'D' && cmd[1] == 'B' && cmd[2] == 'G' &&
                cmd[3] >= '0' && cmd[3] <= '3')
                dbg_cmd = cmd[3] - '0';
            pos = 0;
        } else if (pos < 4) {
            cmd[pos++] = c;
        } else {
            pos = 5;                         /* commande trop longue : ignorée */
        }
    }
}
