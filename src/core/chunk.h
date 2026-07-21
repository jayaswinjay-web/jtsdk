#ifndef jts_chunk_h
#define jts_chunk_h

#include "common.h"
#include "core/value.h"
#include "vm/opcodes.h"

typedef struct {
    int offset;
    int slot;
    const char* name;
    int name_length;
    int scope_depth;
} DebugLocal;

typedef struct {
    DebugLocal* locals;
    int local_count;
    int capacity;
    const char* function_name;
    int function_name_length;
    int arity;
} DebugFuncInfo;

typedef struct {
    int count;
    int capacity;
    uint8_t* code;
    int* lines;
    bool* breakpoints;
    ValueArray constants;
    DebugFuncInfo* debug_funcs;
    int debug_func_count;
    int debug_func_capacity;
    char* source;
    int source_length;
} Chunk;

void init_chunk(Chunk* chunk);
void free_chunk(Chunk* chunk);
void write_chunk(Chunk* chunk, uint8_t byte, int line);
int add_constant(Chunk* chunk, Value value);
void chunk_set_breakpoint(Chunk* chunk, int offset, bool enabled);
bool chunk_has_breakpoint(Chunk* chunk, int offset);
void chunk_clear_all_breakpoints(Chunk* chunk);
DebugFuncInfo* chunk_add_debug_func(Chunk* chunk, const char* name, int name_length, int arity);
void debug_func_add_local(DebugFuncInfo* func, int offset, int slot, const char* name, int name_length, int scope_depth);
void chunk_store_source(Chunk* chunk, const char* source, int length);

#endif
