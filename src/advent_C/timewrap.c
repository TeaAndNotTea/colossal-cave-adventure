/* timewrap.c : implements TIME() and LTIME() expected by your Fortran */
#include "f2c.h"
#include <time.h>

/* INTEGER FUNCTION TIME() */
integer time_(void) {
    time_t now = time(NULL);
    if (now == (time_t)-1) now = 0;
    return (integer)now;
}

/* SUBROUTINE LTIME(T, TAR)  — TAR(1..9) mapping like struct tm */
int ltime_(integer *t, integer *tar) {
    time_t tt = (time_t)(*t);
    struct tm *lt = localtime(&tt);
    if (!lt) { for (int i=0;i<9;i++) tar[i]=0; return 0; }
    tar[0] = lt->tm_sec;           /* TAR(1) */
    tar[1] = lt->tm_min;           /* TAR(2) */
    tar[2] = lt->tm_hour;          /* TAR(3) */
    tar[3] = lt->tm_mday;          /* TAR(4) */
    tar[4] = lt->tm_mon + 1;       /* TAR(5) */
    tar[5] = lt->tm_year;          /* TAR(6) — years since 1900 */
    tar[6] = lt->tm_wday;          /* TAR(7) */
    tar[7] = lt->tm_yday + 1;      /* TAR(8) — 1..366 */
    tar[8] = lt->tm_isdst;         /* TAR(9) */
    return 0;
}
