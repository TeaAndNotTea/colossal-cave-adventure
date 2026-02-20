
/* main_stub.c - provides a real C main() that calls the f2c entry MAIN__ */
#include "f2c.h"
extern int MAIN__(void);
int main(int argc, char **argv) {
    (void)argc; (void)argv;
    return MAIN__();
}
