/* f2c_compat.h — include f2c.h, then un-poison a few macros for system headers */
#ifndef F2C_COMPAT_H
#define F2C_COMPAT_H

#include "f2c.h"

/* Avoid collisions with stdlib.h / math headers, etc. */
#ifdef abs
#  undef abs
#endif
#ifdef min
#  undef min
#endif
#ifdef max
#  undef max
#endif

#endif /* F2C_COMPAT_H */
