#include <stdio.h>
#include <emscripten.h>
int MAIN__() {
    EM_ASM({console.log("[DEBUG] main: entered");});
    printf("Hello from printf in main!\n");
    fflush(stdout);
    EM_ASM({console.log("[DEBUG] main: after printf");});
    return 0;
}
