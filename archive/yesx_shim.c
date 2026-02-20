/* yesx_shim.c
   Drop-in, blocking, line-oriented replacement for f2c's yesx_().
   Signature matches your generated C exactly.
*/
#include <stdio.h>
#include <string.h>
#include "f2c.h"   /* for integer, logical, TRUE_/FALSE_, S_fp */

/* ASCII-only uppercase to avoid <ctype.h>/locale. */
static inline unsigned char to_upper_ascii(unsigned char c) {
    return (c >= 'a' && c <= 'z') ? (unsigned char)(c - 'a' + 'A') : c;
}

logical __wrap_yesx_(integer *x, integer *y, integer *z__, S_fp spk)
{
    char buf[512];

    for (;;) {
        /* Print the prompt message X (if nonzero), each time through the loop,
           just like the original Fortran (label 1 before GETIN and GOTO 1). */
        if (x && *x) {
            (*spk)(x);
            fflush(stdout);
        }

        /* Read a whole line from stdin (blocking). Treat EOF as NO. */
        if (!fgets(buf, (int)sizeof buf, stdin)) {
            if (z__ && *z__) (*spk)(z__);
            return FALSE_;
        }

        /* Trim newline, uppercase ASCII, skip leading spaces. */
        size_t len = strlen(buf);
        if (len && buf[len - 1] == '\n') buf[--len] = '\0';
        for (size_t i = 0; i < len; ++i)
            buf[i] = (char)to_upper_ascii((unsigned char)buf[i]);

        size_t i = 0;
        while (i < len && buf[i] == ' ') i++;

        /* Accept YES/Y and NO/N (any trailing junk is ignored, like Fortran A5 match). */
        if (i < len && (buf[i] == 'Y')) {
            if (y && *y) (*spk)(y);
            return TRUE_;
        }
        if (i < len && (buf[i] == 'N')) {
            if (z__ && *z__) (*spk)(z__);
            return FALSE_;
        }

        /* Invalid; print the same message the original used and loop. */
        fputs("\n Please answer the question.\n", stdout);
        fflush(stdout);
        /* Loop repeats (and reprints X, matching the Fortran). */
    }
}
