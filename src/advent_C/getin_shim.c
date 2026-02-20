// getin_shim.c -- robust Fortran-style input for Emscripten, both Node.js and browser
#include <stdio.h>
#include <string.h>
#include "f2c.h"

#include <emscripten.h>

extern void get_js_input(char* buf, int maxlen);

// Uppercase ASCII only, no locale
static void upcase_ascii(char *s, ftnlen n) {
    for (ftnlen i = 0; i < n; ++i) {
        if (s[i] >= 'a' && s[i] <= 'z') s[i] -= 32;
    }
}

// Fill with spaces
static void blank(char *dst, ftnlen n) {
    for (ftnlen i = 0; i < n; ++i) dst[i] = ' ';
}

/*
 * Fortran: SUBROUTINE GETIN(WORD1,WORD1X,WORD2,WORD2X)
 * All args are CHARACTER*5 (so 5 chars), passed as char*, len params.
 */
int getin_(char *word1, char *word1x, char *word2, char *word2x,
           ftnlen word1_len, ftnlen word1x_len, ftnlen word2_len, ftnlen word2x_len)
{
    char line[128] = {0};
    char w1[6] = {0}, w2[6] = {0};

    // Print prompt for debug (stderr, so visible even if stdout is captured)
    //fprintf(stderr, "[getin_] called\n[getin_] > "); fflush(stderr);

    // ** Call JS to fetch a line of user input **
    get_js_input(line, sizeof(line));
    //fprintf(stderr, "[getin_] got line: '%s'\n", line);

    // Remove trailing newline, if any
    char *p = strchr(line, '\n');
    if (p) *p = 0;

    // Parse first and (optionally) second word
    sscanf(line, "%5s %5s", w1, w2);

    upcase_ascii(w1, 5);
    upcase_ascii(w2, 5);

    blank(word1, word1_len);
    blank(word1x, word1x_len);
    blank(word2, word2_len);
    blank(word2x, word2x_len);

    // Copy up to 5 chars into Fortran string slots
    memcpy(word1, w1, strlen(w1));
    // word1x not used in classic input

    memcpy(word2, w2, strlen(w2));
    // word2x not used in classic input

    // Debug output
    //fprintf(stderr, "[getin_] parsed: w1='%s', w2='%s'\n", w1, w2);

    return 0;
}
