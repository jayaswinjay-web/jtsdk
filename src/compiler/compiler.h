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
    bool is_captured;
} Local;

typedef struct {
    uint8_t index;
    bool is_local;
} Upvalue;

typedef enum {
    TYPE_SCRIPT,
    TYPE_FUNCTION
} CompilerFunctionType;

typedef struct Compiler {
    struct Compiler* parent;
    Scanner* scanner;
    ObjFunction* function;
    Token current;
    Token previous;
    bool had_error;
    bool panic_mode;
    bool assign_created_local;
    bool detect_hoists;
    Token hoist_names[MAX_LOCALS];
    int hoist_count;
    CompilerFunctionType function_type;
    Local locals[MAX_LOCALS];
    int local_count;
    int scope_depth;
    Upvalue upvalues[UINT8_COUNT];
    int upvalue_count;
    DebugFuncInfo* debug_func;
    bool has_yield;
    bool has_superclass;
    Token superclass_name;
} Compiler;

bool compile(const char* source, Chunk* chunk);

#endif
