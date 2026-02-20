/* fio_wraps.c
   Intercept libf2c formatted I/O for unit 5 (stdin) and unit 6 (stdout):
   - READ: block on a full line and serve fixed-width CHARACTER fields to do_fio
   - WRITE: send bytes to stdout; terminate record with newline at e_wsfe
   Everything else defers to the real libf2c functions.

   Build with linker wraps:
     -Wl,--wrap=s_rsfe  -Wl,--wrap=e_rsfe  -Wl,--wrap=do_fio
     -Wl,--wrap=s_wsfe  -Wl,--wrap=e_wsfe
*/

#include <stdio.h>
#include <string.h>
#include "f2c.h"

/* Decls for the real libf2c functions we will delegate to when not unit 5/6. */
extern integer __real_s_rsfe(cilist *a);
extern integer __real_e_rsfe(void);
extern integer __real_do_fio(integer *number, char *ptr, ftnlen len);
extern integer __real_s_wsfe(cilist *a);
extern integer __real_e_wsfe(void);

/* Simple ASCII uppercasing, no <ctype.h> needed. */
static inline unsigned char to_upper_ascii(unsigned char c) {
    return (c >= 'a' && c <= 'z') ? (unsigned char)(c - 'a' + 'A') : c;
}

/* State for a single active READ or WRITE on unit 5/6. */
enum { MODE_NONE = 0, MODE_READ = 1, MODE_WRITE = 2 };
static int mode = MODE_NONE;
static int active_unit = -1;

/* READ buffer */
static char inbuf[1024];
static size_t inpos = 0, inlen = 0;

/* ---- READ side ------------------------------------------------------- */

integer __wrap_s_rsfe(cilist *a) {
    /* Only intercept unit 5 (stdin). Others pass through. */
    if (!a || a->ciunit != 5) {
        return __real_s_rsfe(a);
    }
    mode = MODE_READ;
    active_unit = 5;

    /* Block for a whole line from stdin. */
    if (!fgets(inbuf, (int)sizeof inbuf, stdin)) {
        inbuf[0] = '\0';
        inlen = 0;
    } else {
        size_t L = strlen(inbuf);
        if (L && inbuf[L-1] == '\n') inbuf[--L] = '\0';
        /* Uppercase to match the game’s expectations. */
        for (size_t i = 0; i < L; ++i) inbuf[i] = (char)to_upper_ascii((unsigned char)inbuf[i]);
        inlen = L;
    }
    inpos = 0;
    return 0;
}

integer __wrap_e_rsfe(void) {
    if (mode == MODE_READ && active_unit == 5) {
        mode = MODE_NONE;
        active_unit = -1;
        return 0;
    }
    return __real_e_rsfe();
}

/* do_fio is used for CHARACTER transfers in formatted I/O.
   Here we serve fixed-width slices from the captured input line.
*/
integer __wrap_do_fio(integer *number, char *ptr, ftnlen len) {
    if (mode == MODE_READ && active_unit == 5) {
        /* Serve *number items, each of width 'len'. For A (or A5), f2c passes the
           destination length in 'len'. We consume exactly len chars per item. */
        integer n = number ? *number : 1;
        for (integer i = 0; i < n; ++i) {
            /* Copy next 'len' chars (or pad with spaces if short). */
            for (ftnlen j = 0; j < len; ++j) {
                char c = (inpos < inlen) ? inbuf[inpos++] : ' ';
                ptr[j] = c;
            }
            ptr += len;
        }
        return 0;
    }
    if (mode == MODE_WRITE && active_unit == 6) {
        /* WRITE side: dump bytes directly to stdout. */
        integer n = number ? *number : 1;
        for (integer i = 0; i < n; ++i) {
            if (len > 0) fwrite(ptr, (size_t)len, 1, stdout);
            ptr += len;
        }
        return 0;
    }
    /* Otherwise, fall back to the real routine. */
    return __real_do_fio(number, ptr, len);
}

/* ---- WRITE side ------------------------------------------------------ */

integer __wrap_s_wsfe(cilist *a) {
    if (!a || a->ciunit != 6) {
        return __real_s_wsfe(a);
    }
    mode = MODE_WRITE;
    active_unit = 6;
    return 0;
}

integer __wrap_e_wsfe(void) {
    if (mode == MODE_WRITE && active_unit == 6) {
        /* Terminate the formatted record (newline). */
        fputc('\n', stdout);
        fflush(stdout);
        mode = MODE_NONE;
        active_unit = -1;
        return 0;
    }
    return __real_e_wsfe();
}
