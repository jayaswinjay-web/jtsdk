#ifndef jts_fileio_h
#define jts_fileio_h

#include "common.h"
#include "core/chunk.h"

char* read_file(const char* path);
void free_file(char* buffer);
bool save_bytecode(Chunk* chunk, const char* path);
bool load_bytecode(Chunk* chunk, const char* path);

#endif
