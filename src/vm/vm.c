#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "vm/vm.h"
#include "vm/native.h"
#include "core/object.h"
#include "core/memory.h"
#include "core/table.h"
#include "compiler/compiler.h"

#ifdef DEBUG_TRACE_EXECUTION
#include "vm/debug.h"
#endif

VM vm;

void vm_set_dirs(const char* program_dir, const char* install_dir) {
    static char pd[1024];
    static char id[1024];
    if (program_dir != NULL) {
        strncpy(pd, program_dir, sizeof(pd) - 1);
        pd[sizeof(pd) - 1] = '\0';
        vm.program_dir = pd;
    } else {
        vm.program_dir = NULL;
    }
    if (install_dir != NULL) {
        strncpy(id, install_dir, sizeof(id) - 1);
        id[sizeof(id) - 1] = '\0';
        vm.install_dir = id;
    } else {
        vm.install_dir = NULL;
    }
}

static void reset_stack(void) {
    vm.stack_top = vm.stack;
    vm.frame_count = 0;
    vm.handler_count = 0;
    vm.open_upvalues = NULL;
}

void init_vm(void) {
    reset_stack();
    init_table(&vm.globals);
    init_table(&vm.strings);
    init_table(&vm.scrolls_loaded);
    vm.objects = NULL;
    vm.open_upvalues = NULL;
    register_native_functions();

    vm.debug_enabled = false;
    vm.debug_mode = DEBUG_NONE;
    vm.debug_step_frame_depth = 0;
    vm.debug_step_offset = 0;
    vm.debug_paused = false;
    vm.debug_just_stopped = false;
    vm.debug_resume_skip = false;
    vm.debug_stop_line = 0;
    vm.debug_stop_reason = NULL;
    vm.debug_source = NULL;
    vm.debug_source_length = 0;

    const char* native_names[] = {
        "str", "tensor", "matrix", "matmul", "sigmoid", "relu", "mse",
        "http_server", "http_route", "http_start", "http_request", "sqrt", "math",
        "sin", "cos", "tan", "log", "exp",
        "upper", "lower", "trim", "split", "contains", "replace",
        "substring", "starts_with", "ends_with", "length",
        "remove", "pop", "sort",
        "read_file", "write_file", "import_file", "file_exists",
        "range", "abs", "min", "max", "sum", "pow", "round",
        "floor", "ceil", "rand", "randint", "seed", "shuffle",
        "int", "float", "bool",
        "find", "count", "capitalize", "title", "swapcase",
        "is_digit", "is_alpha", "is_alnum", "is_space", "is_upper", "is_lower",
        "zfill", "ljust", "rjust", "center", "join", "lstrip", "rstrip", "splitlines",
        "format",
        "insert", "extend", "clear", "copy", "reverse", "index",
        "keys", "values", "items", "get", "has", "update",
        "set", "set_add", "set_remove", "set_contains", "set_union", "set_intersection", "set_difference",
        "json_parse", "json_stringify",
        "now", "sleep", "strftime",
        "env", "args", "exit", "cwd",
        "next",
        NULL
    };
    for (int i = 0; native_names[i] != NULL; i++) {
        ObjString* name = copy_string(native_names[i], (int)strlen(native_names[i]));
        push(OBJ_VAL(name));
        table_set(&vm.globals, name, OBJ_VAL(name));
        pop();
    }

    ObjString* pi_name = copy_string("pi", 2);
    push(OBJ_VAL(pi_name));
    table_set(&vm.globals, pi_name, NUMBER_VAL(3.14159265358979323846));
    pop();
    ObjString* e_name = copy_string("e", 1);
    push(OBJ_VAL(e_name));
    table_set(&vm.globals, e_name, NUMBER_VAL(2.71828182845904523536));
    pop();
}

void free_vm(void) {
    free_table(&vm.globals);
    free_table(&vm.strings);
    free_table(&vm.scrolls_loaded);
    free_objects();
    if (vm.debug_source) {
        free(vm.debug_source);
        vm.debug_source = NULL;
    }
}

void push(Value value) {
    *vm.stack_top = value;
    vm.stack_top++;
}

Value pop(void) {
    vm.stack_top--;
    return *vm.stack_top;
}

static Value peek(int distance) {
    return vm.stack_top[-1 - distance];
}

static void runtime_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for (int i = vm.frame_count - 1; i >= 0; i--) {
        CallFrame* frame = &vm.frames[i];
        ObjFunction* function = frame->function;
        size_t instruction = frame->ip - function->chunk.code - 1;
        fprintf(stderr, "[line %d] in ",
                function->chunk.lines[instruction]);
        if (function->name != NULL) {
            fprintf(stderr, "%s()\n", function->name->chars);
        } else {
            fprintf(stderr, "script\n");
        }
    }

    reset_stack();
}

static ObjUpvalue* capture_upvalue(Value* local) {
    ObjUpvalue* prev_upvalue = NULL;
    ObjUpvalue* upvalue = vm.open_upvalues;
    while (upvalue != NULL && upvalue->location > local) {
        prev_upvalue = upvalue;
        upvalue = upvalue->next;
    }
    if (upvalue != NULL && upvalue->location == local) {
        return upvalue;
    }
    ObjUpvalue* created_upvalue = new_upvalue(local);
    created_upvalue->next = upvalue;
    if (prev_upvalue == NULL) {
        vm.open_upvalues = created_upvalue;
    } else {
        prev_upvalue->next = created_upvalue;
    }
    return created_upvalue;
}

static void close_upvalues(Value* last) {
    while (vm.open_upvalues != NULL && vm.open_upvalues->location >= last) {
        ObjUpvalue* upvalue = vm.open_upvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        vm.open_upvalues = upvalue->next;
    }
}

static bool call(Value callee, int arg_count) {
    ObjClosure* closure = NULL;
    ObjFunction* function;
    if (IS_CLOSURE(callee)) {
        closure = AS_CLOSURE(callee);
        function = closure->function;
    } else if (IS_FUNCTION(callee)) {
        function = AS_FUNCTION(callee);
    } else {
        return false;
    }

    int min_arity = function->arity;
    int max_arity = function->max_arity;
    if (max_arity < min_arity) max_arity = min_arity;
    if (arg_count < min_arity || arg_count > max_arity) {
        if (min_arity == max_arity) {
            runtime_error("Expected %d arguments but got %d.",
                          min_arity, arg_count);
        } else {
            runtime_error("Expected %d to %d arguments but got %d.",
                          min_arity, max_arity, arg_count);
        }
        return false;
    }

    if (vm.frame_count == FRAMES_MAX) {
        runtime_error("Stack overflow.");
        return false;
    }

    int padded = arg_count;
    if (max_arity > arg_count) {
        for (int i = arg_count; i < max_arity; i++) {
            push(NIL_VAL);
        }
        padded = max_arity;
    }

    // If the function is a generator, create a generator object instead of calling directly.
    if (function->is_generator) {
        // Create a closure for the function if we don't have one
        ObjClosure* gen_closure = closure;
        if (gen_closure == NULL) {
            gen_closure = new_closure(function);
        }
        ObjGenerator* generator = new_generator(gen_closure);
        // Store the arguments in the generator for later use.
        generator->arg_count = padded;
        generator->args = (Value*)malloc(sizeof(Value) * padded);
        if (generator->args == NULL) {
            runtime_error("Failed to allocate generator args");
            return false;
        }
        for (int i = 0; i < padded; i++) {
            generator->args[i] = vm.stack[vm.stack_top - vm.stack - padded + i];
        }
        // Pop the arguments and the callee from the stack.
        vm.stack_top -= padded + 1; // +1 for the callee
        push(OBJ_VAL(generator));
        return true;
    }

    CallFrame* frame = &vm.frames[vm.frame_count++];
    frame->closure = closure;
    frame->function = function;
    frame->ip = function->chunk.code;
    frame->arg_count = arg_count;
    frame->slots = vm.stack_top - padded - 1;
    frame->generator = NULL;
    return true;
}

void vm_set_debug_enabled(bool enabled) {
    vm.debug_enabled = enabled;
    if (enabled) {
        vm.debug_mode = DEBUG_CONTINUE;
    }
}

bool vm_is_debug_paused(void) {
    return vm.debug_paused;
}

void vm_debug_continue(void) {
    vm.debug_mode = DEBUG_CONTINUE;
    vm.debug_paused = false;
    vm.debug_resume_skip = true;
}

void vm_debug_step_in(void) {
    vm.debug_mode = DEBUG_STEP_IN;
    vm.debug_paused = false;
    vm.debug_resume_skip = false;
}

void vm_debug_step_over(void) {
    vm.debug_mode = DEBUG_STEP_OVER;
    vm.debug_step_frame_depth = vm.frame_count;
    vm.debug_paused = false;
    vm.debug_resume_skip = false;
}

void vm_debug_step_out(void) {
    vm.debug_mode = DEBUG_STEP_OUT;
    vm.debug_step_frame_depth = vm.frame_count;
    vm.debug_paused = false;
    vm.debug_resume_skip = false;
}

void vm_debug_pause(void) {
    vm.debug_mode = DEBUG_PAUSE;
    vm.debug_paused = false;
    vm.debug_resume_skip = true;
}

int vm_get_current_line(void) {
    /* If we just stopped, return the precise stop line computed during pause. */
    if (vm.debug_just_stopped && vm.debug_stop_line > 0) {
        return vm.debug_stop_line;
    }
    if (vm.frame_count == 0) return 0;
    CallFrame* frame = &vm.frames[vm.frame_count - 1];
    int offset = (int)(frame->ip - frame->function->chunk.code - 1);
    if (offset >= 0 && offset < frame->function->chunk.count) {
        return frame->function->chunk.lines[offset];
    }
    return 0;
}

const char* vm_get_current_file(void) {
    if (vm.frame_count == 0) return "<script>";
    CallFrame* frame = &vm.frames[vm.frame_count - 1];
    if (frame->function->name != NULL) {
        return frame->function->name->chars;
    }
    return "<script>";
}

const char* vm_get_debug_stop_reason(void) {
    return vm.debug_stop_reason ? vm.debug_stop_reason : "step";
}

void vm_set_breakpoints(Chunk* chunk, int* offsets, int count) {
    chunk_clear_all_breakpoints(chunk);
    for (int i = 0; i < count; i++) {
        chunk_set_breakpoint(chunk, offsets[i], true);
    }
}

void vm_get_stack_frame_names(const char** names, int* lines, int* count) {
    *count = vm.frame_count;
    for (int i = 0; i < vm.frame_count; i++) {
        CallFrame* frame = &vm.frames[i];
        if (frame->function->name != NULL) {
            names[i] = frame->function->name->chars;
        } else {
            names[i] = "<script>";
        }
        if (i == vm.frame_count - 1 && vm.debug_just_stopped && vm.debug_stop_line > 0) {
            lines[i] = vm.debug_stop_line;
            continue;
        }
        int offset = (int)(frame->ip - frame->function->chunk.code - 1);
        if (offset >= 0 && offset < frame->function->chunk.count) {
            lines[i] = frame->function->chunk.lines[offset];
        } else {
            lines[i] = 0;
        }
    }
}

void vm_get_variables(int frame_idx, const char** names, int* name_lengths, Value* values, int* count) {
    if (frame_idx < 0 || frame_idx >= vm.frame_count) {
        *count = 0;
        return;
    }

    CallFrame* frame = &vm.frames[frame_idx];
    ObjFunction* func = frame->function;

    /* Find the DebugFuncInfo matching this function by name. */
    DebugFuncInfo* match = NULL;
    for (int i = 0; i < func->chunk.debug_func_count; i++) {
        DebugFuncInfo* df = &func->chunk.debug_funcs[i];
        if (func->name != NULL && df->function_name != NULL &&
            df->function_name_length == (int)strlen(func->name->chars) &&
            strncmp(df->function_name, func->name->chars, df->function_name_length) == 0) {
            match = df;
            break;
        }
    }
    if (match == NULL && func->name == NULL && func->chunk.debug_func_count > 0) {
        /* <script> */
        match = &func->chunk.debug_funcs[0];
    }

    int idx = 0;

    if (match != NULL) {
        for (int i = 0; i < match->local_count && idx < 256; i++) {
            DebugLocal* local = &match->locals[i];
            int slot = local->slot;
            if (local->scope_depth == -1) continue; /* uninitialized */
            if (slot >= 0 && &frame->slots[slot] < vm.stack_top) {
                names[idx] = local->name;
                name_lengths[idx] = local->name_length;
                values[idx] = frame->slots[slot];
                idx++;
            }
        }
    }

    *count = idx;
}

void vm_get_globals(const char** names, Value* values, int* count, int* total) {
    int cap = *total;
    int idx = 0;

    for (int i = 0; i < vm.globals.capacity && idx < cap; i++) {
        Entry* entry = &vm.globals.entries[i];
        if (entry->key != NULL) {
            names[idx] = entry->key->chars;
            values[idx] = entry->value;
            idx++;
        }
    }

    *count = idx;
}

const char* vm_get_source(void) {
    if (vm.frame_count == 0) return NULL;
    CallFrame* frame = &vm.frames[vm.frame_count - 1];
    if (frame->function->chunk.source != NULL) {
        return frame->function->chunk.source;
    }
    return NULL;
}

int vm_get_source_length(void) {
    if (vm.frame_count == 0) return 0;
    CallFrame* frame = &vm.frames[vm.frame_count - 1];
    return frame->function->chunk.source_length;
}

static InterpretResult run(void) {
    CallFrame* frame = &vm.frames[vm.frame_count - 1];

#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() \
    (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONSTANT() \
    (frame->function->chunk.constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())

#define BINARY_OP(valueType, op) \
    do { \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
            runtime_error("Operands must be numbers."); \
            return INTERPRET_RUNTIME_ERROR; \
        } \
        double b = AS_NUMBER(pop()); \
        double a = AS_NUMBER(pop()); \
        push(valueType(a op b)); \
    } while (0)

    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        int tr_off = (int)(frame->ip - frame->function->chunk.code);
        disassemble_instruction(&frame->function->chunk, tr_off);
        printf("stack:");
        for (Value* v = vm.stack; v < vm.stack_top; v++) {
            printf(" [");
            print_value(*v);
            printf("]");
        }
        printf("\n");
#endif
        if (vm.debug_enabled) {
            int current_offset = (int)(frame->ip - frame->function->chunk.code);
            int current_line = 0;
            if (current_offset >= 0 && current_offset < frame->function->chunk.count) {
                current_line = frame->function->chunk.lines[current_offset];
            }
            bool should_stop = false;

            if (vm.debug_mode == DEBUG_STEP_IN) {
                if (current_line != vm.debug_stop_line) {
                    should_stop = true;
                    vm.debug_stop_reason = "step";
                }
            } else if (vm.debug_mode == DEBUG_STEP_OVER) {
                if (vm.frame_count <= vm.debug_step_frame_depth &&
                    current_line != vm.debug_stop_line) {
                    should_stop = true;
                    vm.debug_stop_reason = "step";
                }
            } else if (vm.debug_mode == DEBUG_STEP_OUT) {
                if (vm.frame_count < vm.debug_step_frame_depth &&
                    current_line != vm.debug_stop_line) {
                    should_stop = true;
                    vm.debug_stop_reason = "step";
                }
            } else if (vm.debug_mode == DEBUG_CONTINUE || vm.debug_mode == DEBUG_PAUSE) {
                /* After resuming from a stop, skip breakpoint checks while we are
                   still on the line where we stopped (a line spans multiple
                   instructions, and the breakpoint would otherwise re-trigger). */
                bool on_stop_line = (vm.debug_resume_skip && current_line == vm.debug_stop_line);
                if (!on_stop_line) {
                    vm.debug_resume_skip = false;
                    if (chunk_has_breakpoint(&frame->function->chunk, current_offset)) {
                        should_stop = true;
                        vm.debug_stop_reason = "breakpoint";
                    }
                }
            }

            if (should_stop) {
                vm.debug_mode = DEBUG_NONE;
                vm.debug_paused = true;
                vm.debug_just_stopped = true;
                vm.debug_stop_line = current_line;
                return INTERPRET_OK;
            }
        }

        uint8_t instruction;
        switch (instruction = READ_BYTE()) {

        case OP_CONSTANT: {
            Value constant = READ_CONSTANT();
            push(constant);
            break;
        }

        case OP_NIL:     push(NIL_VAL); break;
        case OP_TRUE:    push(BOOL_VAL(true)); break;
        case OP_FALSE:   push(BOOL_VAL(false)); break;

        case OP_POP:     pop(); break;

        case OP_DEFINE_GLOBAL: {
            ObjString* name = READ_STRING();
            table_set(&vm.globals, name, peek(0));
            pop();
            break;
        }

        case OP_GET_GLOBAL: {
            ObjString* name = READ_STRING();
            Value value;
            if (!table_get(&vm.globals, name, &value)) {
                runtime_error("Undefined variable '%s'.", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            push(value);
            break;
        }

        case OP_SET_GLOBAL: {
            ObjString* name = READ_STRING();
            table_set(&vm.globals, name, peek(0));
            break;
        }

        case OP_GET_LOCAL: {
            uint8_t slot = READ_BYTE();
            push(frame->slots[slot]);
            break;
        }

        case OP_SET_LOCAL: {
            uint8_t slot = READ_BYTE();
            frame->slots[slot] = peek(0);
            break;
        }

        case OP_ADD: {
            if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                ObjString* b = AS_STRING(pop());
                ObjString* a = AS_STRING(pop());
                int length = a->length + b->length;
                char* chars = ALLOCATE(char, length + 1);
                memcpy(chars, a->chars, a->length);
                memcpy(chars + a->length, b->chars, b->length);
                chars[length] = '\0';
                push(OBJ_VAL(take_string(chars, length)));
            } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                push(NUMBER_VAL(a + b));
            } else if (IS_NUMBER(peek(0)) && IS_STRING(peek(1))) {
                Value num_val = pop();
                ObjString* str_val = AS_STRING(pop());
                Value str_num = number_to_string(AS_NUMBER(num_val));
                ObjString* s_num = AS_STRING(str_num);
                int length = str_val->length + s_num->length;
                char* chars = ALLOCATE(char, length + 1);
                memcpy(chars, str_val->chars, str_val->length);
                memcpy(chars + str_val->length, s_num->chars, s_num->length);
                chars[length] = '\0';
                push(OBJ_VAL(take_string(chars, length)));
            } else if (IS_STRING(peek(0)) && IS_NUMBER(peek(1))) {
                ObjString* str_val = AS_STRING(pop());
                Value num_val = pop();
                Value str_num = number_to_string(AS_NUMBER(num_val));
                ObjString* s_num = AS_STRING(str_num);
                int length = s_num->length + str_val->length;
                char* chars = ALLOCATE(char, length + 1);
                memcpy(chars, s_num->chars, s_num->length);
                memcpy(chars + s_num->length, str_val->chars, str_val->length);
                chars[length] = '\0';
                push(OBJ_VAL(take_string(chars, length)));
            } else {
                runtime_error("Operands must be two numbers, two strings, or a string and a number.");
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        }

        case OP_SUBTRACT: {
            if (IS_SET(peek(0)) || IS_SET(peek(1))) {
                if (!IS_SET(peek(0)) || !IS_SET(peek(1))) {
                    runtime_error("Both operands must be sets for set difference.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjSet* b = AS_SET(pop());
                ObjSet* a = AS_SET(pop());
                ObjSet* out = new_set();
                for (int i = 0; i < a->count; i++) {
                    if (!set_contains(b, a->values[i])) set_add(out, a->values[i]);
                }
                push(OBJ_VAL(out));
                break;
            }
            BINARY_OP(NUMBER_VAL, -);
            break;
        }
        case OP_MULTIPLY: {
            if (IS_STRING(peek(0)) && IS_NUMBER(peek(1))) {
                ObjString* str = AS_STRING(pop());
                Value num = pop();
                int n = (int)AS_NUMBER(num);
                if (n < 0) n = 0;
                int total = str->length * n;
                if (total == 0 || n == 0) {
                    push(OBJ_VAL(copy_string("", 0)));
                } else {
                    char* chars = ALLOCATE(char, total + 1);
                    for (int i = 0; i < n; i++) {
                        memcpy(chars + i * str->length, str->chars, str->length);
                    }
                    chars[total] = '\0';
                    push(OBJ_VAL(take_string(chars, total)));
                }
                break;
            } else if (IS_NUMBER(peek(0)) && IS_STRING(peek(1))) {
                Value num = pop();
                ObjString* str = AS_STRING(pop());
                int n = (int)AS_NUMBER(num);
                if (n < 0) n = 0;
                int total = str->length * n;
                if (total == 0 || n == 0) {
                    push(OBJ_VAL(copy_string("", 0)));
                } else {
                    char* chars = ALLOCATE(char, total + 1);
                    for (int i = 0; i < n; i++) {
                        memcpy(chars + i * str->length, str->chars, str->length);
                    }
                    chars[total] = '\0';
                    push(OBJ_VAL(take_string(chars, total)));
                }
                break;
            }
            BINARY_OP(NUMBER_VAL, *);
            break;
        }
        case OP_DIVIDE: {
            if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
                runtime_error("Operands must be numbers.");
                return INTERPRET_RUNTIME_ERROR;
            }
            double b = AS_NUMBER(pop());
            double a = AS_NUMBER(pop());
            if (b == 0) {
                runtime_error("Division by zero.");
                return INTERPRET_RUNTIME_ERROR;
            }
            push(NUMBER_VAL(a / b));
            break;
        }

        case OP_MODULO: {
            if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
                runtime_error("Operands must be numbers.");
                return INTERPRET_RUNTIME_ERROR;
            }
            double b = AS_NUMBER(pop());
            double a = AS_NUMBER(pop());
            if (b == 0) {
                runtime_error("Division by zero.");
                return INTERPRET_RUNTIME_ERROR;
            }
            push(NUMBER_VAL(fmod(a, b)));
            break;
        }

        case OP_POWER: {
            if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
                runtime_error("Operands must be numbers.");
                return INTERPRET_RUNTIME_ERROR;
            }
            double b = AS_NUMBER(pop());
            double a = AS_NUMBER(pop());
            push(NUMBER_VAL(pow(a, b)));
            break;
        }

        case OP_FLOOR_DIV: {
            if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
                runtime_error("Operands must be numbers.");
                return INTERPRET_RUNTIME_ERROR;
            }
            double b = AS_NUMBER(pop());
            double a = AS_NUMBER(pop());
            if (b == 0) {
                runtime_error("Division by zero.");
                return INTERPRET_RUNTIME_ERROR;
            }
            push(NUMBER_VAL(floor(a / b)));
            break;
        }

        case OP_BIT_AND: {
            if (IS_SET(peek(0)) || IS_SET(peek(1))) {
                if (!IS_SET(peek(0)) || !IS_SET(peek(1))) {
                    runtime_error("Both operands must be sets for set intersection.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjSet* b = AS_SET(pop());
                ObjSet* a = AS_SET(pop());
                ObjSet* out = new_set();
                for (int i = 0; i < a->count; i++) {
                    if (set_contains(b, a->values[i])) set_add(out, a->values[i]);
                }
                push(OBJ_VAL(out));
                break;
            }
            if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
                runtime_error("Operands must be numbers.");
                return INTERPRET_RUNTIME_ERROR;
            }
            long long b = (long long)AS_NUMBER(pop());
            long long a = (long long)AS_NUMBER(pop());
            push(NUMBER_VAL((double)(a & b)));
            break;
        }

        case OP_BIT_OR: {
            if (IS_SET(peek(0)) || IS_SET(peek(1))) {
                if (!IS_SET(peek(0)) || !IS_SET(peek(1))) {
                    runtime_error("Both operands must be sets for set union.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjSet* b = AS_SET(pop());
                ObjSet* a = AS_SET(pop());
                ObjSet* out = new_set();
                for (int i = 0; i < a->count; i++) set_add(out, a->values[i]);
                for (int i = 0; i < b->count; i++) set_add(out, b->values[i]);
                push(OBJ_VAL(out));
                break;
            }
            if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
                runtime_error("Operands must be numbers.");
                return INTERPRET_RUNTIME_ERROR;
            }
            long long b = (long long)AS_NUMBER(pop());
            long long a = (long long)AS_NUMBER(pop());
            push(NUMBER_VAL((double)(a | b)));
            break;
        }

        case OP_BIT_XOR: {
            if (IS_SET(peek(0)) || IS_SET(peek(1))) {
                if (!IS_SET(peek(0)) || !IS_SET(peek(1))) {
                    runtime_error("Both operands must be sets for symmetric difference.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjSet* b = AS_SET(pop());
                ObjSet* a = AS_SET(pop());
                ObjSet* out = new_set();
                for (int i = 0; i < a->count; i++) {
                    if (!set_contains(b, a->values[i])) set_add(out, a->values[i]);
                }
                for (int i = 0; i < b->count; i++) {
                    if (!set_contains(a, b->values[i])) set_add(out, b->values[i]);
                }
                push(OBJ_VAL(out));
                break;
            }
            if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
                runtime_error("Operands must be numbers.");
                return INTERPRET_RUNTIME_ERROR;
            }
            long long b = (long long)AS_NUMBER(pop());
            long long a = (long long)AS_NUMBER(pop());
            push(NUMBER_VAL((double)(a ^ b)));
            break;
        }

        case OP_BIT_NOT: {
            if (!IS_NUMBER(peek(0))) {
                runtime_error("Operand must be a number.");
                return INTERPRET_RUNTIME_ERROR;
            }
            long long a = (long long)AS_NUMBER(pop());
            push(NUMBER_VAL((double)(~a)));
            break;
        }

        case OP_SHIFT_LEFT: {
            if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
                runtime_error("Operands must be numbers.");
                return INTERPRET_RUNTIME_ERROR;
            }
            long long b = (long long)AS_NUMBER(pop());
            long long a = (long long)AS_NUMBER(pop());
            if (b < 0 || b >= 64) {
                runtime_error("Shift amount out of range.");
                return INTERPRET_RUNTIME_ERROR;
            }
            push(NUMBER_VAL((double)(a << b)));
            break;
        }

        case OP_SHIFT_RIGHT: {
            if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
                runtime_error("Operands must be numbers.");
                return INTERPRET_RUNTIME_ERROR;
            }
            long long b = (long long)AS_NUMBER(pop());
            long long a = (long long)AS_NUMBER(pop());
            if (b < 0 || b >= 64) {
                runtime_error("Shift amount out of range.");
                return INTERPRET_RUNTIME_ERROR;
            }
            push(NUMBER_VAL((double)(a >> b)));
            break;
        }

        case OP_NEGATE: {
            if (!IS_NUMBER(peek(0))) {
                runtime_error("Operand must be a number.");
                return INTERPRET_RUNTIME_ERROR;
            }
            push(NUMBER_VAL(-AS_NUMBER(pop())));
            break;
        }

        case OP_NOT: {
            Value val = pop();
            bool is_falsy = IS_NIL(val) || (IS_BOOL(val) && !AS_BOOL(val)) ||
                            (IS_NUMBER(val) && AS_NUMBER(val) == 0) ||
                            (IS_STRING(val) && AS_STRING(val)->length == 0);
            push(BOOL_VAL(is_falsy));
            break;
        }

        case OP_EQUAL: {
            Value b = pop();
            Value a = pop();
            push(BOOL_VAL(values_equal(a, b)));
            break;
        }

        case OP_NOT_EQUAL: {
            Value b = pop();
            Value a = pop();
            push(BOOL_VAL(!values_equal(a, b)));
            break;
        }

        case OP_IS: {
            Value b = pop();
            Value a = pop();
            bool identical = false;
            if (a.type != b.type) {
                identical = false;
            } else {
                switch (a.type) {
                    case VAL_NIL:    identical = true; break;
                    case VAL_BOOL:   identical = AS_BOOL(a) == AS_BOOL(b); break;
                    case VAL_NUMBER: identical = AS_NUMBER(a) == AS_NUMBER(b); break;
                    case VAL_OBJ:    identical = AS_OBJ(a) == AS_OBJ(b); break;
                }
            }
            push(BOOL_VAL(identical));
            break;
        }

        case OP_GREATER:
        case OP_LESS:
        case OP_GREATER_EQUAL:
        case OP_LESS_EQUAL: {
            if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                ObjString* b = AS_STRING(pop());
                ObjString* a = AS_STRING(pop());
                int minlen = a->length < b->length ? a->length : b->length;
                int cmp = memcmp(a->chars, b->chars, (size_t)minlen);
                if (cmp == 0) {
                    cmp = (a->length > b->length) - (a->length < b->length);
                }
                bool result;
                switch (instruction) {
                    case OP_GREATER:      result = cmp > 0; break;
                    case OP_LESS:         result = cmp < 0; break;
                    case OP_GREATER_EQUAL: result = cmp >= 0; break;
                    default:              result = cmp <= 0; break;
                }
                push(BOOL_VAL(result));
                break;
            }
            if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
                runtime_error("Operands must be numbers or strings.");
                return INTERPRET_RUNTIME_ERROR;
            }
            double b = AS_NUMBER(pop());
            double a = AS_NUMBER(pop());
            bool result;
            switch (instruction) {
                case OP_GREATER:      result = a > b; break;
                case OP_LESS:         result = a < b; break;
                case OP_GREATER_EQUAL: result = a >= b; break;
                default:              result = a <= b; break;
            }
            push(BOOL_VAL(result));
            break;
        }

        case OP_IN: {
            Value container = pop();
            Value item = pop();
            bool found = false;
            if (IS_LIST(container)) {
                ObjList* list = AS_LIST(container);
                for (int i = 0; i < list->count; i++) {
                    if (values_equal(item, list->values[i])) {
                        found = true;
                        break;
                    }
                }
            } else if (IS_STRING(container)) {
                if (IS_STRING(item)) {
                    ObjString* str = AS_STRING(container);
                    ObjString* sub = AS_STRING(item);
                    if (sub->length == 0) {
                        found = true;
                    } else if (sub->length <= str->length) {
                        for (int i = 0; i <= str->length - sub->length; i++) {
                            if (memcmp(&str->chars[i], sub->chars, sub->length) == 0) {
                                found = true;
                                break;
                            }
                        }
                    }
                }
            } else if (IS_DICT(container)) {
                if (IS_STRING(item)) {
                    Value result;
                    found = dict_get(AS_DICT(container), AS_STRING(item), &result);
                }
            } else if (IS_SET(container)) {
                found = set_contains(AS_SET(container), item);
            } else {
                runtime_error("Right operand of 'in' must be a list, string, dict, or set.");
                return INTERPRET_RUNTIME_ERROR;
            }
            push(BOOL_VAL(found));
            break;
        }

        case OP_ASSERT: {
            Value message = pop();
            Value condition = pop();
            bool is_falsy = IS_NIL(condition) || (IS_BOOL(condition) && !AS_BOOL(condition)) ||
                            (IS_NUMBER(condition) && AS_NUMBER(condition) == 0) ||
                            (IS_STRING(condition) && AS_STRING(condition)->length == 0);
            if (is_falsy) {
                if (IS_STRING(message) && AS_STRING(message)->length > 0) {
                    runtime_error("Assertion failed: %s", AS_CSTRING(message));
                } else {
                    runtime_error("Assertion failed.");
                }
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        }

        case OP_PRINT: {
            Value value = pop();
            print_value(value);
            printf("\n");
            break;
        }

        case OP_JUMP: {
            uint16_t offset = READ_SHORT();
            frame->ip += offset;
            break;
        }

        case OP_JUMP_IF_FALSE: {
            uint16_t offset = READ_SHORT();
            Value val = peek(0);
            bool is_falsy = IS_NIL(val) || (IS_BOOL(val) && !AS_BOOL(val)) ||
                            (IS_NUMBER(val) && AS_NUMBER(val) == 0) ||
                            (IS_STRING(val) && AS_STRING(val)->length == 0);
            if (is_falsy) {
                frame->ip += offset;
            }
            break;
        }

        case OP_LOOP: {
            uint16_t offset = READ_SHORT();
            frame->ip -= offset;
            break;
        }

        case OP_CALL: {
            int arg_count = READ_BYTE();
            Value callee = peek(arg_count);

            if (IS_OBJ(callee) && AS_OBJ(callee)->type == OBJ_STRING) {
                ObjString* name = AS_STRING(callee);

                if (arg_count >= 1 && IS_DICT(vm.stack_top[-arg_count])) {
                    ObjDict* receiver_dict = AS_DICT(vm.stack_top[-arg_count]);
                    Value member;
                    bool member_found = dict_get(receiver_dict, name, &member);
                    if (!member_found) {
                        Value marker;
                        if (dict_get(receiver_dict, copy_string("\x1fns", 3), &marker) &&
                            table_get(&vm.globals, name, &member)) {
                            member_found = true;
                        }
                    }
                    if (member_found) {
                        int receiver_idx = (int)(vm.stack_top - vm.stack) - arg_count;
                        int m_argc = arg_count - 1;
                        if (IS_FUNCTION(member) || IS_CLOSURE(member)) {
                            for (int i = 0; i < m_argc; i++) {
                                vm.stack[receiver_idx + i] = vm.stack[receiver_idx + 1 + i];
                            }
                            vm.stack_top--;
                            vm.stack[receiver_idx - 1] = member;
                            if (!call(member, m_argc)) {
                                return INTERPRET_RUNTIME_ERROR;
                            }
                            frame = &vm.frames[vm.frame_count - 1];
                        } else if (IS_STRING(member)) {
                            Value result;
                            Value* m_args = &vm.stack[receiver_idx + 1];
                            if (!call_native(AS_CSTRING(member), m_argc, m_args, &result)) {
                                return INTERPRET_RUNTIME_ERROR;
                            }
                            vm.stack_top -= arg_count + 1;
                            push(result);
                        } else if (IS_NATIVE(member)) {
                            Value result;
                            Value* m_args = &vm.stack[receiver_idx + 1];
                            if (!AS_NATIVE(member)->function(m_argc, m_args, &result)) {
                                return INTERPRET_RUNTIME_ERROR;
                            }
                            vm.stack_top -= arg_count + 1;
                            push(result);
                        } else {
                            runtime_error("Namespace member '%s' is not callable.", name->chars);
                            return INTERPRET_RUNTIME_ERROR;
                        }
                        break;
                    }
                }

                Value result;
                if (call_native(name->chars, arg_count,
                                vm.stack_top - arg_count, &result)) {
                    vm.stack_top -= arg_count + 1;
                    push(result);
                    break;
                }
            }

            if (IS_BOUND_METHOD(callee)) {
                ObjBoundMethod* bound = AS_BOUND_METHOD(callee);
                vm.stack_top[-arg_count - 1] = bound->receiver;
                if (!call(bound->method, arg_count)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frame_count - 1];
                break;
            }

            if (!IS_OBJ(callee) ||
                (AS_OBJ(callee)->type != OBJ_FUNCTION && AS_OBJ(callee)->type != OBJ_CLOSURE)) {
                runtime_error("Can only call functions.");
                return INTERPRET_RUNTIME_ERROR;
            }

            if (!call(callee, arg_count)) {
                return INTERPRET_RUNTIME_ERROR;
            }
            frame = &vm.frames[vm.frame_count - 1];
            break;
        }

        case OP_CLOSURE: {
            uint8_t const_idx = READ_BYTE();
            ObjFunction* function = AS_FUNCTION(frame->function->chunk.constants.values[const_idx]);
            ObjClosure* closure = new_closure(function);
            push(OBJ_VAL(closure));
            uint8_t upvalue_count = READ_BYTE();
            for (int i = 0; i < upvalue_count; i++) {
                uint8_t is_local = READ_BYTE();
                uint8_t index = READ_BYTE();
                if (is_local) {
                    closure->upvalues[i] = capture_upvalue(frame->slots + index);
                } else {
                    closure->upvalues[i] = frame->closure->upvalues[index];
                }
            }
            break;
        }

        case OP_GET_UPVALUE: {
            uint8_t slot = READ_BYTE();
            push(*frame->closure->upvalues[slot]->location);
            break;
        }

        case OP_SET_UPVALUE: {
            uint8_t slot = READ_BYTE();
            *frame->closure->upvalues[slot]->location = peek(0);
            break;
        }

        case OP_CLOSE_UPVALUE:
            close_upvalues(vm.stack_top - 1);
            pop();
            break;

        case OP_LIST: {
            int count = READ_BYTE();
            ObjList* list = new_list();
            for (int i = count - 1; i >= 0; i--) {
                list_append(list, peek(i));
            }
            vm.stack_top -= count;
            push(OBJ_VAL(list));
            break;
        }

        case OP_SET_LITERAL: {
            int count = READ_BYTE();
            ObjSet* set = new_set();
            for (int i = count - 1; i >= 0; i--) {
                set_add(set, peek(i));
            }
            vm.stack_top -= count;
            push(OBJ_VAL(set));
            break;
        }

        case OP_DICT: {
            ObjDict* dict = new_dict();
            int pair_count = (int)AS_NUMBER(pop());
            for (int i = 0; i < pair_count; i++) {
                Value value = pop();
                ObjString* key = AS_STRING(pop());
                dict_set(dict, key, value);
            }
            push(OBJ_VAL(dict));
            break;
        }

        case OP_INDEX: {
            Value index = pop();
            Value obj = pop();
            if (IS_STRING(obj) && IS_NUMBER(index)) {
                ObjString* str = AS_STRING(obj);
                int i = (int)AS_NUMBER(index);
                if (i < 0) i += str->length;
                if (i < 0 || i >= str->length) {
                    runtime_error("String index out of bounds.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(OBJ_VAL(copy_string(&str->chars[i], 1)));
                break;
            }
            if (IS_DICT(obj)) {
                if (!IS_STRING(index)) {
                    runtime_error("Dict key must be a string.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                Value result;
                if (!dict_get(AS_DICT(obj), AS_STRING(index), &result)) {
                    push(NIL_VAL);
                } else {
                    push(result);
                }
                break;
            }
            if (!IS_OBJ(obj) || !IS_LIST(obj)) {
                runtime_error("Can only index lists.");
                return INTERPRET_RUNTIME_ERROR;
            }
            if (!IS_NUMBER(index)) {
                runtime_error("List index must be a number.");
                return INTERPRET_RUNTIME_ERROR;
            }
            int i = (int)AS_NUMBER(index);
            ObjList* list = AS_LIST(obj);
            if (i < 0) i += list->count;
            if (i < 0 || i >= list->count) {
                runtime_error("List index out of bounds.");
                return INTERPRET_RUNTIME_ERROR;
            }
            push(list_get(list, i));
            break;
        }

        case OP_INDEX_SET: {
            Value value = pop();
            Value index = pop();
            Value obj = pop();
            if (!IS_OBJ(obj) || !IS_LIST(obj)) {
                runtime_error("Can only index lists.");
                return INTERPRET_RUNTIME_ERROR;
            }
            if (!IS_NUMBER(index)) {
                runtime_error("List index must be a number.");
                return INTERPRET_RUNTIME_ERROR;
            }
            int i = (int)AS_NUMBER(index);
            ObjList* list = AS_LIST(obj);
            if (i < 0) i += list->count;
            if (!list_set(list, i, value)) {
                runtime_error("List index out of bounds.");
                return INTERPRET_RUNTIME_ERROR;
            }
            push(value);
            break;
        }

        case OP_DEL_GLOBAL: {
            ObjString* name = READ_STRING();
            table_delete(&vm.globals, name);
            break;
        }

        case OP_DEL_INDEX: {
            Value index = pop();
            Value obj = pop();
            if (IS_DICT(obj)) {
                if (!IS_STRING(index)) {
                    runtime_error("Dict key must be a string.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                if (!table_delete(&AS_DICT(obj)->entries, AS_STRING(index))) {
                    runtime_error("Dict key not found.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            if (!IS_OBJ(obj) || !IS_LIST(obj)) {
                runtime_error("Can only delete from lists or dicts.");
                return INTERPRET_RUNTIME_ERROR;
            }
            if (!IS_NUMBER(index)) {
                runtime_error("List index must be a number.");
                return INTERPRET_RUNTIME_ERROR;
            }
            ObjList* list = AS_LIST(obj);
            int i = (int)AS_NUMBER(index);
            if (i < 0) i += list->count;
            if (i < 0 || i >= list->count) {
                runtime_error("List index out of bounds.");
                return INTERPRET_RUNTIME_ERROR;
            }
            for (int j = i; j < list->count - 1; j++) {
                list->values[j] = list->values[j + 1];
            }
            list->count--;
            break;
        }

        case OP_SLICE: {
            Value step_v = pop();
            Value end_v = pop();
            Value start_v = pop();
            Value obj = pop();
            int len = 0;
            if (IS_STRING(obj)) {
                len = AS_STRING(obj)->length;
            } else if (IS_LIST(obj)) {
                len = AS_LIST(obj)->count;
            } else {
                runtime_error("Can only slice strings and lists.");
                return INTERPRET_RUNTIME_ERROR;
            }
            int step = IS_NUMBER(step_v) ? (int)AS_NUMBER(step_v) : 1;
            if (step == 0) {
                runtime_error("Slice step cannot be zero.");
                return INTERPRET_RUNTIME_ERROR;
            }
            int start, end;
            if (step > 0) {
                start = IS_NUMBER(start_v) ? (int)AS_NUMBER(start_v) : 0;
                end = IS_NUMBER(end_v) ? (int)AS_NUMBER(end_v) : len;
                if (start < 0) start += len;
                if (end < 0) end += len;
                if (start < 0) start = 0;
                if (end > len) end = len;
            } else {
                start = IS_NUMBER(start_v) ? (int)AS_NUMBER(start_v) : len - 1;
                end = IS_NUMBER(end_v) ? (int)AS_NUMBER(end_v) : -len - 1;
                if (start < 0) start += len;
                if (end < 0) end += len;
                if (start >= len) start = len - 1;
                if (end < -1) end = -1;
            }
            if (IS_STRING(obj)) {
                ObjString* str = AS_STRING(obj);
                int cap = len + 1;
                char* chars = ALLOCATE(char, cap);
                int count = 0;
                for (int i = start; step > 0 ? i < end : i > end; i += step) {
                    if (i < 0 || i >= len) break;
                    chars[count++] = str->chars[i];
                }
                chars[count] = '\0';
                push(OBJ_VAL(take_string(chars, count)));
            } else {
                ObjList* list = AS_LIST(obj);
                ObjList* result = new_list();
                for (int i = start; step > 0 ? i < end : i > end; i += step) {
                    if (i < 0 || i >= len) break;
                    list_append(result, list->values[i]);
                }
                push(OBJ_VAL(result));
            }
            break;
        }

        case OP_LEN: {
            Value obj = pop();
            if (IS_STRING(obj)) {
                push(NUMBER_VAL((double)AS_STRING(obj)->length));
            } else if (IS_LIST(obj)) {
                push(NUMBER_VAL((double)AS_LIST(obj)->count));
            } else if (IS_DICT(obj)) {
                push(NUMBER_VAL((double)AS_DICT(obj)->entries.count));
            } else if (IS_SET(obj)) {
                push(NUMBER_VAL((double)AS_SET(obj)->count));
            } else if (IS_TENSOR(obj)) {
                push(NUMBER_VAL((double)AS_TENSOR(obj)->size));
            } else if (IS_MATRIX(obj)) {
                push(NUMBER_VAL((double)(AS_MATRIX(obj)->rows * AS_MATRIX(obj)->cols)));
            } else {
                runtime_error("Cannot get length of this value.");
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        }

        case OP_ITER_VALUE: {
            Value index = pop();
            Value iterable = pop();
            if (!IS_NUMBER(index)) {
                runtime_error("Iterator index must be a number.");
                return INTERPRET_RUNTIME_ERROR;
            }
            int i = (int)AS_NUMBER(index);
            if (IS_LIST(iterable)) {
                ObjList* list = AS_LIST(iterable);
                if (i < 0 || i >= list->count) {
                    runtime_error("List index out of bounds.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(list->values[i]);
            } else if (IS_STRING(iterable)) {
                ObjString* str = AS_STRING(iterable);
                if (i < 0 || i >= str->length) {
                    runtime_error("String index out of bounds.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(OBJ_VAL(copy_string(&str->chars[i], 1)));
            } else if (IS_DICT(iterable)) {
                ObjDict* dict = AS_DICT(iterable);
                int found = -1;
                int idx = 0;
                for (int j = 0; j < dict->entries.capacity; j++) {
                    if (dict->entries.entries[j].key != NULL) {
                        if (idx == i) {
                            found = j;
                            break;
                        }
                        idx++;
                    }
                }
                if (found == -1) {
                    runtime_error("Dict index out of bounds.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(OBJ_VAL(dict->entries.entries[found].key));
            } else if (IS_SET(iterable)) {
                ObjSet* set = AS_SET(iterable);
                if (i < 0 || i >= set->count) {
                    runtime_error("Set index out of bounds.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(set->values[i]);
            } else {
                runtime_error("Cannot iterate over this value.");
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        }

        case OP_UNPACK: {
            int count = READ_BYTE();
            Value list_v = pop();
            if (!IS_LIST(list_v)) {
                runtime_error("Cannot unpack a non-list value.");
                return INTERPRET_RUNTIME_ERROR;
            }
            ObjList* list = AS_LIST(list_v);
            if (list->count != count) {
                runtime_error("Cannot unpack %d values into %d targets.", list->count, count);
                return INTERPRET_RUNTIME_ERROR;
            }
            for (int i = 0; i < count; i++) {
                push(list->values[i]);
            }
            break;
        }

        case OP_APPEND_LIST: {
            Value element = pop();
            Value list_v = peek(0);
            if (!IS_LIST(list_v)) {
                runtime_error("Cannot append to a non-list value.");
                return INTERPRET_RUNTIME_ERROR;
            }
            list_append(AS_LIST(list_v), element);
            break;
        }

        case OP_GET_ARGCOUNT: {
            push(NUMBER_VAL((double)frame->arg_count));
            break;
        }

        case OP_BREAK: {
            runtime_error("break reached outside of loop patching.");
            return INTERPRET_RUNTIME_ERROR;
        }

        case OP_CONTINUE: {
            runtime_error("continue reached outside of loop patching.");
            return INTERPRET_RUNTIME_ERROR;
        }

        case OP_SWAP: {
            Value a = pop();
            Value b = pop();
            push(a);
            push(b);
            break;
        }

        case OP_THROW: {
            Value error = pop();
            if (vm.handler_count > 0) {
                ExceptionHandler handler = vm.handlers[--vm.handler_count];
                vm.stack_top = handler.saved_stack_top;
                vm.frame_count = handler.saved_frame_count;
                frame = &vm.frames[vm.frame_count - 1];
                frame->ip = handler.handler_ip;
                push(error);
                break;
            }
            if (IS_STRING(error)) {
                fprintf(stderr, "[line %d] Runtime Error: %s\n",
                        frame->function->chunk.lines[frame->ip - frame->function->chunk.code - 1],
                        AS_CSTRING(error));
            } else {
                fprintf(stderr, "[line %d] Runtime Error: ",
                        frame->function->chunk.lines[frame->ip - frame->function->chunk.code - 1]);
                print_value(error);
                fprintf(stderr, "\n");
            }
            return INTERPRET_RUNTIME_ERROR;
        }

        case OP_TRY_SET_IP: {
            uint16_t offset = READ_SHORT();
            if (vm.handler_count < MAX_EXCEPTION_HANDLERS) {
                ExceptionHandler* h = &vm.handlers[vm.handler_count++];
                h->handler_ip = frame->ip + offset;
                h->saved_stack_top = vm.stack_top;
                h->saved_frame_count = vm.frame_count;
            }
            break;
        }

        case OP_POP_TRY: {
            if (vm.handler_count > 0) {
                vm.handler_count--;
            }
            break;
        }

        case OP_CLASS: {
            ObjString* name = READ_STRING();
            ObjClass* klass = new_class(name);
            push(OBJ_VAL(klass));
            break;
        }

        case OP_INHERIT: {
            Value superclass_val = peek(0);
            Value subclass_val = peek(1);
            if (!IS_CLASS(superclass_val)) {
                runtime_error("Superclass must be a class.");
                return INTERPRET_RUNTIME_ERROR;
            }
            ObjClass* super = AS_CLASS(superclass_val);
            ObjClass* subclass = AS_CLASS(subclass_val);
            table_add_all(&super->methods, &subclass->methods);
            pop();
            break;
        }

        case OP_METHOD: {
            ObjString* name = READ_STRING();
            Value method = peek(0);
            ObjClass* klass = AS_CLASS(peek(1));
            table_set(&klass->methods, name, method);
            pop();
            break;
        }

        case OP_GET_PROPERTY: {
            ObjString* name = READ_STRING();
            Value obj = peek(0);
            if (IS_DICT(obj)) {
                ObjDict* dict = AS_DICT(obj);
                Value value;
                if (dict_get(dict, name, &value)) {
                    pop();
                    push(value);
                    break;
                }
                Value marker;
                if (dict_get(dict, copy_string("\x1fns", 3), &marker)) {
                    if (table_get(&vm.globals, name, &value)) {
                        pop();
                        push(value);
                        break;
                    }
                }
                runtime_error("Undefined member '%s' in namespace.", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            if (!IS_OBJ(obj)) {
                runtime_error("Only instances have properties.");
                return INTERPRET_RUNTIME_ERROR;
            }
            if (AS_OBJ(obj)->type == OBJ_INSTANCE) {
                ObjInstance* instance = AS_INSTANCE(obj);
                Value value;
                if (table_get(&instance->fields, name, &value)) {
                    pop();
                    push(value);
                    break;
                }
                Value method;
                if (table_get(&instance->klass->methods, name, &method)) {
                    ObjBoundMethod* bound = new_bound_method(obj, method);
                    pop();
                    push(OBJ_VAL(bound));
                    break;
                }
                runtime_error("Undefined property '%s'.", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            runtime_error("Only instances have properties.");
            return INTERPRET_RUNTIME_ERROR;
        }

        case OP_SET_PROPERTY: {
            ObjString* name = READ_STRING();
            Value obj = peek(1);
            if (!IS_OBJ(obj) || AS_OBJ(obj)->type != OBJ_INSTANCE) {
                runtime_error("Only instances have fields.");
                return INTERPRET_RUNTIME_ERROR;
            }
            ObjInstance* instance = AS_INSTANCE(obj);
            table_set(&instance->fields, name, peek(0));
            Value value = pop();
            pop();
            push(value);
            break;
        }

        case OP_INVOKE_WITH: {
            ObjString* method_name = READ_STRING();
            int arg_count = READ_BYTE();
            Value receiver = peek(arg_count);
            if (IS_DICT(receiver)) {
                ObjDict* dict = AS_DICT(receiver);
                Value method_val;
                bool found = dict_get(dict, method_name, &method_val);
                if (!found) {
                    Value marker;
                    if (dict_get(dict, copy_string("\x1fns", 3), &marker)) {
                        found = table_get(&vm.globals, method_name, &method_val);
                    }
                }
                if (!found) {
                    runtime_error("Undefined member '%s' in namespace.", method_name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                int base = (int)(vm.stack_top - vm.stack) - arg_count - 1;
                if (IS_FUNCTION(method_val) || IS_CLOSURE(method_val)) {
                    vm.stack[base] = method_val;
                    if (!call(method_val, arg_count)) {
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    frame = &vm.frames[vm.frame_count - 1];
                } else if (IS_STRING(method_val)) {
                    Value result;
                    if (!call_native(AS_CSTRING(method_val), arg_count,
                                     vm.stack_top - arg_count, &result)) {
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    vm.stack_top -= arg_count + 1;
                    push(result);
                } else if (IS_NATIVE(method_val)) {
                    ObjNative* nat = AS_NATIVE(method_val);
                    Value result;
                    if (!nat->function(arg_count, &vm.stack[base + 1], &result)) {
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    vm.stack_top = &vm.stack[base];
                    push(result);
                } else {
                    runtime_error("Namespace member '%s' is not callable.", method_name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            if (!IS_OBJ(receiver) || AS_OBJ(receiver)->type != OBJ_INSTANCE) {
                runtime_error("Only instances have methods.");
                return INTERPRET_RUNTIME_ERROR;
            }
            ObjInstance* instance = AS_INSTANCE(receiver);
            Value method_val;
            if (!table_get(&instance->klass->methods, method_name, &method_val)) {
                runtime_error("Undefined method '%s'.", method_name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            int base = (int)(vm.stack_top - vm.stack) - arg_count - 1;
            for (int i = arg_count; i >= 0; i--) {
                vm.stack[base + i + 1] = vm.stack[base + i];
            }
            vm.stack[base] = method_val;
            vm.stack_top++;
            if (!call(method_val, arg_count + 1)) {
                return INTERPRET_RUNTIME_ERROR;
            }
            frame = &vm.frames[vm.frame_count - 1];
            break;
        }

        case OP_NEW_INSTANCE: {
            ObjString* name = READ_STRING();
            Value klass_val;
            if (!table_get(&vm.globals, name, &klass_val)) {
                runtime_error("Undefined class '%s'.", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            if (!IS_CLASS(klass_val)) {
                runtime_error("'%s' is not a class.", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            ObjInstance* instance = new_instance(AS_CLASS(klass_val));
            push(OBJ_VAL(instance));
            break;
        }

        case OP_SUPER: {
            ObjString* name = READ_STRING();
            ObjBoundMethod* bound = AS_BOUND_METHOD(peek(0));
            ObjClass* superclass = AS_CLASS(peek(1));
            Value method;
            if (!table_get(&superclass->methods, name, &method)) {
                runtime_error("Undefined method '%s' in superclass.", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            ObjBoundMethod* super_bound = new_bound_method(bound->receiver, method);
            pop();
            push(OBJ_VAL(super_bound));
            break;
        }

        case OP_SUPER_INVOKE: {
            ObjString* method = READ_STRING();
            int arg_count = READ_BYTE();
            ObjBoundMethod* bound = AS_BOUND_METHOD(peek(arg_count));
            ObjClass* superclass = AS_CLASS(peek(arg_count + 1));
            Value method_val;
            if (!table_get(&superclass->methods, method, &method_val)) {
                runtime_error("Undefined method '%s' in superclass.", method->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            if (!call(method_val, arg_count)) {
                return INTERPRET_RUNTIME_ERROR;
            }
            frame = &vm.frames[vm.frame_count - 1];
            break;
        }

        case OP_YIELD: {
            Value yielded = pop();
            // Save generator state (IP and local variables)
            if (frame->generator != NULL) {
                ObjGenerator* gen = frame->generator;
                gen->ip = (int)(frame->ip - frame->function->chunk.code);
                int count = (int)(vm.stack_top - frame->slots);
                if (gen->saved_slot_count < count) {
                    gen->saved_slots = (Value*)reallocate(gen->saved_slots,
                        gen->saved_slot_count * sizeof(Value),
                        count * sizeof(Value));
                    gen->saved_slot_count = count;
                }
                for (int i = 0; i < count; i++) {
                    gen->saved_slots[i] = frame->slots[i];
                }
            }
            // Pop the generator frame; the caller (vm_resume_generator)
            // restores the stack to its own saved position.
            close_upvalues(frame->slots);
            vm.frame_count--;
            push(yielded);
            return INTERPRET_YIELD;
        }

        case OP_RETURN: {
            Value result = pop();
            close_upvalues(frame->slots);
            
            // If this frame belongs to a generator, mark it as exhausted
            if (frame->generator != NULL) {
                frame->generator->exhausted = true;
                // Push the return value so vm_resume_generator can grab it,
                // and stop the VM loop to hand control back to the resumer.
                vm.frame_count--;
                push(result);
                return INTERPRET_OK;
            }
            
            vm.frame_count--;
            if (vm.frame_count == 0) {
                pop();
                return INTERPRET_OK;
            }

            vm.stack_top = vm.frames[vm.frame_count].slots;
            push(result);
            frame = &vm.frames[vm.frame_count - 1];
            break;
        }

        }
    }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}

InterpretResult interpret(const char* source) {
    Chunk chunk;
    init_chunk(&chunk);

    if (!compile(source, &chunk)) {
        free_chunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    chunk_store_source(&chunk, source, (int)strlen(source));

    ObjFunction* function = new_function();
    function->chunk = chunk;

    push(OBJ_VAL(function));
    call(OBJ_VAL(function), 0);

    InterpretResult result = run();

    free_chunk(&chunk);
    return result;
}

InterpretResult interpret_isolated(const char* source) {
    // Run a piece of source (e.g. a brought-in scroll) without continuing
    // the outer VM frames. Saves and restores the full frame stack so the
    // caller's execution resumes exactly where it left off.
    Chunk chunk;
    init_chunk(&chunk);

    if (!compile(source, &chunk)) {
        free_chunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    chunk_store_source(&chunk, source, (int)strlen(source));

    ObjFunction* function = new_function();
    function->chunk = chunk;

    CallFrame saved_frames[FRAMES_MAX];
    memcpy(saved_frames, vm.frames, sizeof(vm.frames));
    int saved_frame_count = vm.frame_count;
    Value* saved_top = vm.stack_top;
    int saved_handler_count = vm.handler_count;

    vm.frame_count = 0;
    push(OBJ_VAL(function));
    call(OBJ_VAL(function), 0);

    InterpretResult result = run();

    memcpy(vm.frames, saved_frames, sizeof(vm.frames));
    vm.frame_count = saved_frame_count;
    vm.stack_top = saved_top;
    vm.handler_count = saved_handler_count;

    free_chunk(&chunk);
    return result;
}

InterpretResult vm_exec(void) {
    return run();
}

bool vm_call(Value func, int arg_count) {
    return call(func, arg_count);
}

InterpretResult vm_resume_generator(ObjGenerator* generator) {
    if (generator->exhausted) {
        push(NIL_VAL);
        return INTERPRET_OK;
    }
    
    ObjClosure* closure = generator->closure;
    ObjFunction* function = closure->function;
    int arg_count = generator->arg_count;
    
    // Save the caller's stack position so we can restore it after
    // the generator suspends or completes.
    Value* saved_top = vm.stack_top;
    int saved_frame_count = vm.frame_count;
    
    // Push the closure and arguments onto the stack
    push(OBJ_VAL(closure));
    for (int i = 0; i < arg_count; i++) {
        push(generator->args[i]);
    }
    
    // Set up frame directly, bypassing generator creation logic
    CallFrame* frame = &vm.frames[vm.frame_count++];
    frame->closure = closure;
    frame->function = function;
    frame->ip = (generator->ip > 0) ? (function->chunk.code + generator->ip) : function->chunk.code;
    frame->arg_count = arg_count;
    frame->slots = vm.stack_top - arg_count - 1;
    frame->generator = generator;
    
    // Restore saved local variables if this is a resume
    if (generator->ip > 0 && generator->saved_slot_count > 0) {
        int count = generator->saved_slot_count;
        if (frame->slots + count >= vm.stack + STACK_MAX) {
            runtime_error("Stack overflow.");
            vm.frame_count--;
            vm.stack_top = saved_top;
            return INTERPRET_RUNTIME_ERROR;
        }
        for (int i = 0; i < count; i++) {
            frame->slots[i] = generator->saved_slots[i];
        }
        vm.stack_top = frame->slots + count;
    }
    
    InterpretResult result = run();
    
    // The generator suspended or completed. Pull the produced value off the
    // stack (if any), then restore the caller's stack state and hand the
    // value back on top.
    Value produced = NIL_VAL;
    if (result == INTERPRET_YIELD) {
        produced = pop();
    }
    vm.stack_top = saved_top;
    vm.frame_count = saved_frame_count;
    push(produced);
    return result;
}
