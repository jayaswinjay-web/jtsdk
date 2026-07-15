#ifndef jts_common_h
#define jts_common_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <stdarg.h>

#define JTS_VERSION_MAJOR 2
#define JTS_VERSION_MINOR 0
#define JTS_VERSION_PATCH 0

#define UINT8_COUNT (UINT8_MAX + 1)

/* #define DEBUG_TRACE_EXECUTION */
/* #define DEBUG_PRINT_CODE */
/* #define DEBUG_SCANNER */

typedef enum {
    TOOL_JTSC,
    TOOL_JTSVM,
    TOOL_JTS
} ToolType;

#endif
