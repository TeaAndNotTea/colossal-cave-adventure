/* advent_common_init.c
 *
 * Fixes missing DATA initialization for COMMON /VOCCOM/ in f2c output.
 * Your FORTRAN declares:
 *   COMMON /VOCCOM/ KTAB, ATAB, TABSIZ
 *   DATA TABSIZ/300/
 *
 * Some f2c builds lose that scalar init at link time, leaving TABSIZ=0.
 * That causes the Section 4 (vocabulary) loop to do DO 1..TABSIZ with
 * TABSIZ==0 and immediately call BUG(4).
 *
 * This companion unit mirrors the /VOCCOM/ layout and sets TABSIZ=300
 * before main() runs (constructor). No other code changes needed.
 */

#include "f2c.h"

/* Mirror the layout of COMMON /VOCCOM/ KTAB(300), ATAB(300*5), TABSIZ */
struct voccom_1_ {
    integer ktab[300];   /* KTAB(300) */
    char    atab[5*300]; /* ATAB(300) as A5 -> 5*300 bytes */
    integer tabsiz;      /* TABSIZ scalar, should be 300 per DATA */
};

/* This symbol is defined by your f2c-generated C from the FORTRAN COMMON. */
extern struct voccom_1_ voccom_;

/* Run before main(): set TABSIZ if the DATA init didn't land. */
static void __attribute__((constructor)) init_common_sizes(void)
{
    if (voccom_.tabsiz <= 0) {
        voccom_.tabsiz = 300; /* from FORTRAN: DATA ... TABSIZ/300/ */
    }
}
