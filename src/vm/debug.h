#ifndef jts_debug_h
#define jts_debug_h

#include "common.h"
#include "core/chunk.h"

int disassemble_instruction(Chunk* chunk, int offset);
void disassemble_chunk(Chunk* chunk, const char* name);

#endif
