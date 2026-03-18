F2C output code hacked by ChatGPT codex 
This is the C code input to the WASM flow.

**advent_common_init.c** A (likely uneccessary) file created during debugging the f2c flow to force a variable to the correct value prior to running Main().<br>
**advent_nosave_f2c2.c** The f2c output code<br>
**dbg.h**  adda a debugging header<br>
**f2c_compat.h** Undefine min(),max(), abs() to prevent colission with stdlib.h / main headers<br>
**f2c.h** F2C header<br>
**getin_shim.c** robust Fortran-style input for Emscripten<br>
**main_stub.c** Provides a real main() to call F2C MAIN__()<br>
**timewrap.c** implements TIME() and LTIME() expected by the FORTRAN style runtime<br>
