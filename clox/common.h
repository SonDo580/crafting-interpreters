#ifndef clox_common_h
#define clox_common_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// #define NAN_BOXING            // use NaN-boxed form instead of tagged union for Value
#define DEBUG_PRINT_CODE      // show emitted code after compiling each function
#define DEBUG_TRACE_EXECUTION // show current instruction and stack content

// #define DEBUG_STRESS_GC // run GC on every allocation
#define DEBUG_LOG_GC

#define UINT8_COUNT (UINT8_MAX + 1)

#endif