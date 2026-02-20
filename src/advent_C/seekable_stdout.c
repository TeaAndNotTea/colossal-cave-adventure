// seekable_stdout.c — make stdout seekable for f2c's formatted writes.
#include <stdio.h>

// Run before main; redirect stdout to a MEMFS file we can tail from JS.
__attribute__((constructor))
static void make_seekable_stdout(void) {
    // Create/replace a seekable file at /stdout.txt and attach stdout to it.
    // "w+" lets us read it from JS to mirror output.
    FILE *f = freopen("/stdout.txt", "w+", stdout);
    if (f) {
        // Unbuffer to push data promptly (we'll tail the file).
        setvbuf(stdout, NULL, _IONBF, 0);
    }
    // (Optional) do the same for stderr if you want:
    // freopen("/stderr.txt", "w+", stderr);
    // setvbuf(stderr, NULL, _IONBF, 0);
}
