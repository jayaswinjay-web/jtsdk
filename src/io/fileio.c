#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "io/fileio.h"
#include "core/memory.h"
#include "core/object.h"

static const uint32_t JBC_MAGIC = 0x4A545342;

char* read_file(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "JTS GO: Could not open file '%s'\n", path);
        exit(74);
    }

    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(file_size + 1);
    if (buffer == NULL) {
        fprintf(stderr, "JTS GO: Not enough memory to read '%s'\n", path);
        fclose(file);
        exit(74);
    }

    size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
    if (bytes_read < file_size) {
        fprintf(stderr, "JTS GO: Could not read file '%s'\n", path);
        free(buffer);
        fclose(file);
        exit(74);
    }

    buffer[bytes_read] = '\0';
    fclose(file);
    return buffer;
}

void free_file(char* buffer) {
    free(buffer);
}

bool save_bytecode(Chunk* chunk, const char* path) {
    FILE* file = fopen(path, "wb");
    if (file == NULL) {
        fprintf(stderr, "JTS GO: Could not open file '%s' for writing\n", path);
        return false;
    }

    fwrite(&JBC_MAGIC, sizeof(uint32_t), 1, file);

    fwrite(&chunk->constants.count, sizeof(int), 1, file);
    for (int i = 0; i < chunk->constants.count; i++) {
        Value value = chunk->constants.values[i];
        fwrite(&value.type, sizeof(ValueType), 1, file);
        switch (value.type) {
            case VAL_NIL: break;
            case VAL_BOOL: {
                bool b = AS_BOOL(value);
                fwrite(&b, sizeof(bool), 1, file);
                break;
            }
            case VAL_NUMBER: {
                double n = AS_NUMBER(value);
                fwrite(&n, sizeof(double), 1, file);
                break;
            }
            case VAL_OBJ: {
                if (IS_STRING(value)) {
                    ObjString* str = AS_STRING(value);
                    fwrite(&str->length, sizeof(int), 1, file);
                    fwrite(str->chars, sizeof(char), str->length, file);
                } else {
                    int len = 0;
                    fwrite(&len, sizeof(int), 1, file);
                }
                break;
            }
        }
    }

    fwrite(&chunk->count, sizeof(int), 1, file);
    fwrite(chunk->code, sizeof(uint8_t), chunk->count, file);
    fwrite(chunk->lines, sizeof(int), chunk->count, file);

    fclose(file);
    return true;
}

bool load_bytecode(Chunk* chunk, const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "JTS GO: Could not open bytecode file '%s'\n", path);
        return false;
    }

    uint32_t magic;
    fread(&magic, sizeof(uint32_t), 1, file);
    if (magic != JBC_MAGIC) {
        fprintf(stderr, "JTS GO: '%s' is not a valid JTS GO bytecode file\n", path);
        fclose(file);
        return false;
    }

    int const_count;
    fread(&const_count, sizeof(int), 1, file);
    for (int i = 0; i < const_count; i++) {
        ValueType type;
        fread(&type, sizeof(ValueType), 1, file);
        switch (type) {
            case VAL_NIL:
                add_constant(chunk, NIL_VAL);
                break;
            case VAL_BOOL: {
                bool b;
                fread(&b, sizeof(bool), 1, file);
                add_constant(chunk, BOOL_VAL(b));
                break;
            }
            case VAL_NUMBER: {
                double n;
                fread(&n, sizeof(double), 1, file);
                add_constant(chunk, NUMBER_VAL(n));
                break;
            }
            case VAL_OBJ: {
                int len;
                fread(&len, sizeof(int), 1, file);
                if (len > 0) {
                    char* chars = ALLOCATE(char, len + 1);
                    fread(chars, sizeof(char), len, file);
                    chars[len] = '\0';
                    ObjString* str = take_string(chars, len);
                    add_constant(chunk, OBJ_VAL(str));
                } else {
                    ObjString* str = copy_string("", 0);
                    add_constant(chunk, OBJ_VAL(str));
                }
                break;
            }
        }
    }

    int code_count;
    fread(&code_count, sizeof(int), 1, file);

    while (chunk->capacity < code_count) {
        int old_cap = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(old_cap);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, old_cap, chunk->capacity);
        chunk->lines = GROW_ARRAY(int, chunk->lines, old_cap, chunk->capacity);
    }

    chunk->count = code_count;
    fread(chunk->code, sizeof(uint8_t), code_count, file);
    fread(chunk->lines, sizeof(int), code_count, file);

    fclose(file);
    return true;
}
