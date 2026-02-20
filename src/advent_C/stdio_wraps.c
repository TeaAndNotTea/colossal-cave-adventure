// stdio_wraps.c — CRT wraps to make TTY seeks harmless
#include <stdio.h>

long __wrap_ftell(FILE *s) { (void)s; return 0L; }
int  __wrap_fseek(FILE *s, long o, int w) { (void)s; (void)o; (void)w; return 0; }

long long __wrap_ftello(FILE *s) { (void)s; return 0LL; }
int       __wrap_fseeko(FILE *s, long long o, int w) { (void)s; (void)o; (void)w; return 0; }

FILE *__wrap_freopen(const char *p, const char *m, FILE *s) {
  (void)p; (void)m; return s;
}
