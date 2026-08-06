#include <stdio.h>
#include "vm/debug.h"
#include "vm/opcodes.h"
#include "core/object.h"

static int simple_instruction(const char* name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

static int byte_instruction(const char* name, Chunk* chunk, int offset) {
    uint8_t slot = chunk->code[offset + 1];
    printf("%-16s %4d\n", name, slot);
    return offset + 2;
}

static int jump_instruction(const char* name, int sign, Chunk* chunk, int offset) {
    uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
    jump |= chunk->code[offset + 2];
    printf("%-16s %4d -> %d\n", name, offset,
           offset + 3 + sign * jump);
    return offset + 3;
}

int disassemble_instruction(Chunk* chunk, int offset) {
    printf("%04d ", offset);
    if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1]) {
        printf("   | ");
    } else {
        printf("%4d ", chunk->lines[offset]);
    }

    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case OP_CONSTANT:       return byte_instruction("OP_CONSTANT", chunk, offset);
        case OP_VOID:            return simple_instruction("OP_VOID", offset);
        case OP_TRUE:           return simple_instruction("OP_TRUE", offset);
        case OP_FALSE:          return simple_instruction("OP_FALSE", offset);
        case OP_POP:            return simple_instruction("OP_POP", offset);
        case OP_DEFINE_GLOBAL:  return byte_instruction("OP_DEFINE_GLOBAL", chunk, offset);
        case OP_GET_GLOBAL:     return byte_instruction("OP_GET_GLOBAL", chunk, offset);
        case OP_SET_GLOBAL:     return byte_instruction("OP_SET_GLOBAL", chunk, offset);
        case OP_ADD:            return simple_instruction("OP_ADD", offset);
        case OP_SUBTRACT:       return simple_instruction("OP_SUBTRACT", offset);
        case OP_MULTIPLY:       return simple_instruction("OP_MULTIPLY", offset);
        case OP_DIVIDE:         return simple_instruction("OP_DIVIDE", offset);
        case OP_MODULO:         return simple_instruction("OP_MODULO", offset);
        case OP_POWER:          return simple_instruction("OP_POWER", offset);
        case OP_FLOOR_DIV:      return simple_instruction("OP_FLOOR_DIV", offset);
        case OP_BIT_OR:         return simple_instruction("OP_BIT_OR", offset);
        case OP_BIT_AND:        return simple_instruction("OP_BIT_AND", offset);
        case OP_BIT_XOR:        return simple_instruction("OP_BIT_XOR", offset);
        case OP_BIT_NOT:        return simple_instruction("OP_BIT_NOT", offset);
        case OP_SHIFT_LEFT:     return simple_instruction("OP_SHIFT_LEFT", offset);
        case OP_SHIFT_RIGHT:    return simple_instruction("OP_SHIFT_RIGHT", offset);
        case OP_NEGATE:         return simple_instruction("OP_NEGATE", offset);
        case OP_NOT:            return simple_instruction("OP_NOT", offset);
        case OP_EQUAL:          return simple_instruction("OP_EQUAL", offset);
        case OP_NOT_EQUAL:      return simple_instruction("OP_NOT_EQUAL", offset);
        case OP_IS:             return simple_instruction("OP_IS", offset);
        case OP_ASSERT:         return simple_instruction("OP_ASSERT", offset);
        case OP_DEL_GLOBAL:     return byte_instruction("OP_DEL_GLOBAL", chunk, offset);
        case OP_DEL_INDEX:      return simple_instruction("OP_DEL_INDEX", offset);
        case OP_GREATER:        return simple_instruction("OP_GREATER", offset);
        case OP_LESS:           return simple_instruction("OP_LESS", offset);
        case OP_GREATER_EQUAL:  return simple_instruction("OP_GREATER_EQUAL", offset);
        case OP_LESS_EQUAL:     return simple_instruction("OP_LESS_EQUAL", offset);
        case OP_SAY:          return simple_instruction("OP_SAY", offset);
        case OP_JUMP:           return jump_instruction("OP_JUMP", 1, chunk, offset);
        case OP_JUMP_IF_FALSE:  return jump_instruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
        case OP_LOOP:           return jump_instruction("OP_LOOP", -1, chunk, offset);
        case OP_GET_LOCAL:      return byte_instruction("OP_GET_LOCAL", chunk, offset);
        case OP_SET_LOCAL:      return byte_instruction("OP_SET_LOCAL", chunk, offset);
        case OP_CALL:           return byte_instruction("OP_CALL", chunk, offset);
        case OP_CLOSURE: {
            uint8_t constant = chunk->code[offset + 1];
            printf("%-16s %4d ", "OP_CLOSURE", constant);
            print_value(chunk->constants.values[constant]);
            printf("\n");
            int upvalue_count = chunk->code[offset + 2];
            offset += 3;
            for (int i = 0; i < upvalue_count; i++) {
                int is_local = chunk->code[offset];
                int index = chunk->code[offset + 1];
                printf("%04d    |                     %s %d\n", offset - 1,
                       is_local ? "local" : "upvalue", index);
                offset += 2;
            }
            return offset;
        }
        case OP_GET_UPVALUE:    return byte_instruction("OP_GET_UPVALUE", chunk, offset);
        case OP_SET_UPVALUE:    return byte_instruction("OP_SET_UPVALUE", chunk, offset);
        case OP_CLOSE_UPVALUE:  return simple_instruction("OP_CLOSE_UPVALUE", offset);
        case OP_LIST:           return byte_instruction("OP_LIST", chunk, offset);
        case OP_SET_LITERAL:    return byte_instruction("OP_SET_LITERAL", chunk, offset);
        case OP_INDEX:          return simple_instruction("OP_INDEX", offset);
        case OP_INDEX_SET:      return simple_instruction("OP_INDEX_SET", offset);
        case OP_RETURN:         return simple_instruction("OP_RETURN", offset);
        case OP_YIELD:          return simple_instruction("OP_YIELD", offset);
        // OOP
        case OP_CLASS:          return byte_instruction("OP_CLASS", chunk, offset);
        case OP_INHERIT:        return simple_instruction("OP_INHERIT", offset);
        case OP_METHOD:         return byte_instruction("OP_METHOD", chunk, offset);
        case OP_GET_FIELD:      return byte_instruction("OP_GET_FIELD", chunk, offset);
        case OP_SET_FIELD:      return byte_instruction("OP_SET_FIELD", chunk, offset);
        case OP_INVOKE:         return byte_instruction("OP_INVOKE", chunk, offset);
        case OP_SUPER:          return byte_instruction("OP_SUPER", chunk, offset);
        case OP_NEW_INSTANCE:   return byte_instruction("OP_NEW_INSTANCE", chunk, offset);
        case OP_GET_PROPERTY:   return byte_instruction("OP_GET_PROPERTY", chunk, offset);
        case OP_SET_PROPERTY:   return byte_instruction("OP_SET_PROPERTY", chunk, offset);
        case OP_INVOKE_WITH: {
            uint8_t name_idx = chunk->code[offset + 1];
            uint8_t argc = chunk->code[offset + 2];
            printf("%-16s %4d arg_count: %d\n", "OP_INVOKE_WITH", name_idx, argc);
            return offset + 3;
        }
        case OP_SUPER_INVOKE: {
            uint8_t name_idx = chunk->code[offset + 1];
            uint8_t argc = chunk->code[offset + 2];
            printf("%-16s %4d arg_count: %d\n", "OP_SUPER_INVOKE", name_idx, argc);
            return offset + 3;
        }
        case OP_HTTP_SERVER:    return simple_instruction("OP_HTTP_SERVER", offset);
        case OP_TENSOR_OP:      return byte_instruction("OP_TENSOR_OP", chunk, offset);
        case OP_MATRIX_OP:      return byte_instruction("OP_MATRIX_OP", chunk, offset);
        // New opcodes
        case OP_BREAK:          return simple_instruction("OP_BREAK", offset);
        case OP_CONTINUE:       return simple_instruction("OP_CONTINUE", offset);
        case OP_SWAP:           return simple_instruction("OP_SWAP", offset);
        case OP_DICT:           return simple_instruction("OP_DICT", offset);
        case OP_DICT_GET:       return simple_instruction("OP_DICT_GET", offset);
        case OP_DICT_SET:       return simple_instruction("OP_DICT_SET", offset);
        case OP_THROW:          return simple_instruction("OP_THROW", offset);
        case OP_TRY_SET_IP:     return jump_instruction("OP_TRY_SET_IP", 1, chunk, offset);
        case OP_POP_TRY:        return simple_instruction("OP_POP_TRY", offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}

void disassemble_chunk(Chunk* chunk, const char* name) {
    printf("== %s ==\n", name);
    for (int offset = 0; offset < chunk->count;) {
        offset = disassemble_instruction(chunk, offset);
    }
}
