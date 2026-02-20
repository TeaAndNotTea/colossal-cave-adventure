/* fio_read_wraps.c
   Intercept libf2c formatted READs from unit 5 (stdin) or unit 0 (default stdin).
   We leave all WRITEs to libf2c so its formatter runs normally.
   Build with:
     -Wl,--wrap=s_rsfe  -Wl,--wrap=e_rsfe  -Wl,--wrap=do_fio
*/
#include <stdio.h>
#include <string.h>
#include "f2c.h"

/* real libf2c entrypoints */
extern integer __real_s_rsfe(cilist *a);
extern integer __real_e_rsfe(void);
extern integer __real_do_fio(integer *number, char *ptr, ftnlen len);

/* minimal ASCII uppercasing; avoids <ctype.h> */
static inline unsigned char to_upper_ascii(unsigned char c) {
    return (c >= 'a' && c <= 'z') ? (unsigned char)(c - 'a' + 'A') : c;
}

/* simple state for a single active formatted READ */
static int reading_unit = 0;  /* 0 = no, 5 or 0 = active */
static char inbuf[1024];
static size_t inpos = 0, inlen = 0;

static int is_stdin_unit(int u) {
    /* Many f2c builds use 5 for stdin; some use 0 for “default” stdin. */
    return (u == 5 || u == 0);
}

integer __wrap_s_rsfe(cilist *a) {
    if (!a || !is_stdin_unit(a->ciunit)) {
        return __real_s_rsfe(a);
    }
    reading_unit = a->ciunit;

    /* fetch an entire input line from real stdin */
    if (!fgets(inbuf, (int)sizeof inbuf, stdin)) {
        inbuf[0] = '\0';
        inlen = 0;
    } else {
        size_t L = strlen(inbuf);
        if (L && inbuf[L-1] == '\n') inbuf[--L] = '\0';
        for (size_t i = 0; i < L; ++i) inbuf[i] = (char)to_upper_ascii((unsigned char)inbuf[i]);
        inlen = L;
    }
    inpos = 0;
    return 0;
}

integer __wrap_e_rsfe(void) {
    if (reading_unit) {
        reading_unit = 0;
        return 0;
    }
    return __real_e_rsfe();
}

integer __wrap_do_fio(integer *number, char *ptr, ftnlen len) {
    if (reading_unit) {
        /* Serve *number items, each of width 'len' bytes, from the captured line. */
        integer n = number ? *number : 1;
        for (integer i = 0; i < n; ++i) {
            for (ftnlen j = 0; j < len; ++j) {
                char c = (inpos < inlen) ? inbuf[inpos++] : ' ';
                ptr[j] = c;
            }
            ptr += len;
        }
        return 0;
    }
    /* For everything else (including WRITEs), use the real routine. */
    return __real_do_fio(number, ptr, len);
}
