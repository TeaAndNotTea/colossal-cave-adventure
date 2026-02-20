// dbg.h — minimal stderr logger
#pragma once
#include <stdio.h>
#define DBG(fmt, ...) do { fprintf(stderr, "[DBG] " fmt "\n", ##__VA_ARGS__); fflush(stderr); } while (0)
