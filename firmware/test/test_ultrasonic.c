/*
 * test_ultrasonic.c - Tests sur PC du module ultrasonic.c
 *
 * TIMER1 est simulé tick par tick (40 ns) : on déclenche les matchs MR0/MR1
 * et la capture CAP1.0 comme le ferait le matériel, puis on appelle l'ISR.
 * Vérifie : salve 8 x 40 kHz, zone aveugle, temps de vol, timeout,
 * débordement du compteur 32 bits, conversion en mm, configuration des broches.
 *
 *   gcc -I. -I../src test_ultrasonic.c ../src/ultrasonic.c ../src/uart0.c -o t && ./t
 */
#include <stdio.h>
#include "board.h"
#include "ultrasonic.h"
#include "uart0.h"
PINCON_T pincon; LPC_GPIO_TypeDef g0,g1,g2; TIM_T t0r,t1r; SC_T sc; UART_T u0;
void TIMER1_IRQHandler(void);
static int fails=0;
#define CHECK(c,...) do{ if(!(c)){fails++;printf("FAIL: ");printf(__VA_ARGS__);printf("\n");} }while(0)

/* run one measurement; echo_edges: list of rising-edge times relative to t0 (ticks) */
static us_result_t run(const uint32_t *echo, int n, uint32_t *tof, uint32_t *edge_t, int *nedges, uint32_t *t_end)
{
    uint32_t start_tc = t1r.TC; (void)start_tc;
    int pin=0; *nedges=0;
    if(!us_start()) return (us_result_t)99;
    uint32_t base = t1r.MR0; /* t0 */
    us_result_t r = US_NONE;
    for (uint32_t k=0; k<25000u*20u; k++) {
        t1r.TC++;
        uint32_t f=0, tc=t1r.TC;
        if ((t1r.MCR & 1u) && tc==t1r.MR0) f|=1u;
        if ((t1r.MCR & 8u) && tc==t1r.MR1) f|=2u;
        for (int i=0;i<n;i++) if (tc==base+echo[i] && (t1r.CCR & 1u)) { t1r.CR0=tc; if (t1r.CCR & 4u) f|=0x10u; }
        if (f) { t1r.IR=f; TIMER1_IRQHandler(); t1r.IR=0;
            if (g2.FIOSET & (1u<<TRIG_PIN)) { if(!pin){edge_t[(*nedges)++]=tc-base;} pin=1; g2.FIOSET=0; }
            if (g2.FIOCLR & (1u<<TRIG_PIN)) { if(pin){edge_t[(*nedges)++]=tc-base;} pin=0; g2.FIOCLR=0; }
        }
        r = us_poll(tof);
        if (r!=US_NONE){ *t_end=tc-base; return r; }
    }
    return r;
}

int main(void){
    uint32_t tof=0, e[64], tend=0; int ne; us_result_t r;
    t1r.TC = 0xFFFFFFFFu - 100000u; /* test wrap-around of free-running TC */
    us_init(); g2.FIOSET = 0; g2.FIOCLR = 0;

    /* 1) burst timing + echo at 5.936 ms (simulation value) */
    uint32_t ech1[] = { 5936u*25u };
    r = run(ech1,1,&tof,e,&ne,&tend);
    CHECK(r==US_OK,"r=%d",r);
    CHECK(ne==16,"edges=%d",ne);
    CHECK(e[0]==0,"first edge %u",e[0]);
    uint32_t sum=0; for(int i=1;i<16;i++){ uint32_t d=e[i]-e[i-1]; CHECK(d==312||d==313,"half period %u",d); sum+=d; }
    (void)sum;
    CHECK(e[14]-e[0]==7u*625u,"7 periods = %u ticks",e[14]-e[0]);
    printf("salve : %d fronts, 7 periodes = %u ticks -> %.1f Hz\n", ne, e[14]-e[0], 25e6*7/(e[14]-e[0]));
    printf("echo a 5,936 ms : tof = %u ticks (%.3f ms) -> %u mm\n", tof, tof/25000.0, us_ticks_to_mm(tof));
    CHECK(tof==5936u*25u,"tof");
    CHECK(us_ticks_to_mm(tof)==1018,"mm=%u",us_ticks_to_mm(tof));

    /* 2) crosstalk during burst/blanking must be ignored */
    uint32_t ech2[] = { 100u*25u, 250u*25u, 1000u*25u };
    r = run(ech2,3,&tof,e,&ne,&tend);
    CHECK(r==US_OK && tof==1000u*25u,"crosstalk r=%d tof=%u",r,tof);
    printf("couplage a 100/250 us ignore, echo a 1 ms -> %u mm\n", us_ticks_to_mm(tof));

    /* 3) no echo -> timeout at MAX_CM */
    r = run(NULL,0,&tof,e,&ne,&tend);
    CHECK(r==US_TIMEOUT,"timeout r=%d",r);
    CHECK(tend==TIMEOUT_TICKS,"tend=%u",tend);
    printf("pas d'echo -> hors portee apres %.2f ms\n", tend/25000.0);

    /* 4) echo just after blanking (min distance) */
    uint32_t ech4[] = { BLANK_TICKS + 25u };
    r = run(ech4,1,&tof,e,&ne,&tend);
    CHECK(r==US_OK,"min r=%d",r);
    printf("echo a %u us (distance min) -> %u mm\n", (unsigned)(tof/25), us_ticks_to_mm(tof));

    /* 5) us_start refused while busy */
    CHECK(us_start()==1,"start"); CHECK(us_start()==0,"busy start accepted");

    /* 6) configuration des broches et de l'UART */
    uart0_init();
    CHECK(u0.DLL==12 && u0.FDR==0x81, "baud regs");
    CHECK((pincon.PINSEL0 & 0xF0u)==0x50u, "uart pins %x", pincon.PINSEL0);
    CHECK((pincon.PINSEL3 & 0x30u)==0x30u, "cap1.0 pinsel3 %x", pincon.PINSEL3);
    printf("UART0 : %.0f bauds (cible 115200)\n", 25e6/(16*12*(1+1.0/8)));

    printf("%s (%d failures)\n", fails?"TESTS FAILED":"ALL TESTS PASSED", fails);
    return fails!=0;
}
