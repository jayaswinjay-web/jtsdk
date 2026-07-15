#ifndef jts_compiler_h
#define jts_compiler_h

#include "common.h"
#include "core/chunk.h"
#include "core/object.h"
#include "compiler/scanner.h"

#define MAX_LOCALS 256
#define MAX_SCOPE_DEPTH 64

typedef struct {
    Token name;
    int depth;
} Local;

typedef struct Compiler {
    struct Compiler* parent;
    Scanner* scanner;
    ObjFunction* function;
    Token current;
    Token previous;
    bool had_error;
    bool panic_mode;
    Local locals[MAX_LOCALS];
    int local_count;
    int scope_depth;
} Compiler;

bool compile(const char* source, Chunk* chunk);

#endif
