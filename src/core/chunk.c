#include "core/chunk.h"
#include "core/memory.h"

void init_chunk(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    chunk->breakpoints = NULL;
    init_value_array(&chunk->constants);
    chunk->debug_funcs = NULL;
    chunk->debug_func_count = 0;
    chunk->debug_func_capacity = 0;
    chunk->source = NULL;
    chunk->source_length = 0;
}

void free_chunk(Chunk* chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(int, chunk->lines, chunk->capacity);
    if (chunk->breakpoints) {
        FREE_ARRAY(bool, chunk->breakpoints, chunk->capacity);
    }
    free_value_array(&chunk->constants);
    if (chunk->debug_funcs) {
        for (int i = 0; i < chunk->debug_func_count; i++) {
            if (chunk->debug_funcs[i].locals) {
                FREE_ARRAY(DebugLocal, chunk->debug_funcs[i].locals,
                           chunk->debug_funcs[i].capacity);
            }
        }
        FREE_ARRAY(DebugFuncInfo, chunk->debug_funcs, chunk->debug_func_capacity);
    }
    if (chunk->source) {
        FREE_ARRAY(char, chunk->source, chunk->source_length);
    }
    init_chunk(chunk);
}

void write_chunk(Chunk* chunk, uint8_t byte, int line) {
    if (chunk->capacity < chunk->count + 1) {
        int old_capacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(old_capacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code,
                                 old_capacity, chunk->capacity);
        chunk->lines = GROW_ARRAY(int, chunk->lines,
                                  old_capacity, chunk->capacity);
        chunk->breakpoints = GROW_ARRAY(bool, chunk->breakpoints,
                                        old_capacity, chunk->capacity);
        for (int i = old_capacity; i < chunk->capacity; i++) {
            chunk->breakpoints[i] = false;
        }
    }
    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count] = line;
    chunk->breakpoints[chunk->count] = false;
    chunk->count++;
}

int add_constant(Chunk* chunk, Value value) {
    write_value_array(&chunk->constants, value);
    return chunk->constants.count - 1;
}

void chunk_set_breakpoint(Chunk* chunk, int offset, bool enabled) {
    if (offset >= 0 && offset < chunk->count) {
        chunk->breakpoints[offset] = enabled;
    }
}

bool chunk_has_breakpoint(Chunk* chunk, int offset) {
    if (offset >= 0 && offset < chunk->count) {
        return chunk->breakpoints[offset];
    }
    return false;
}

void chunk_clear_all_breakpoints(Chunk* chunk) {
    for (int i = 0; i < chunk->count; i++) {
        chunk->breakpoints[i] = false;
    }
}

DebugFuncInfo* chunk_add_debug_func(Chunk* chunk, const char* name, int name_length, int arity) {
    if (chunk->debug_func_count >= chunk->debug_func_capacity) {
        int old_cap = chunk->debug_func_capacity;
        chunk->debug_func_capacity = GROW_CAPACITY(old_cap);
        chunk->debug_funcs = GROW_ARRAY(DebugFuncInfo, chunk->debug_funcs,
                                         old_cap, chunk->debug_func_capacity);
    }
    DebugFuncInfo* info = &chunk->debug_funcs[chunk->debug_func_count++];
    info->function_name = name;
    info->function_name_length = name_length;
    info->arity = arity;
    info->locals = NULL;
    info->local_count = 0;
    info->capacity = 0;
    return info;
}

void debug_func_add_local(DebugFuncInfo* func, int offset, int slot,
                           const char* name, int name_length, int scope_depth) {
    if (func->local_count >= func->capacity) {
        int old_cap = func->capacity;
        func->capacity = GROW_CAPACITY(old_cap);
        func->locals = GROW_ARRAY(DebugLocal, func->locals, old_cap, func->capacity);
    }
    DebugLocal* local = &func->locals[func->local_count++];
    local->offset = offset;
    local->slot = slot;
    local->name = name;
    local->name_length = name_length;
    local->scope_depth = scope_depth;
}

void chunk_store_source(Chunk* chunk, const char* source, int length) {
    if (chunk->source) {
        FREE_ARRAY(char, chunk->source, chunk->source_length);
    }
    chunk->source = ALLOCATE(char, length + 1);
    memcpy(chunk->source, source, length);
    chunk->source[length] = '\0';
    chunk->source_length = length;
}
