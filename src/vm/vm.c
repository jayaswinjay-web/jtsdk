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

static void reset_stack(void) {
    vm.stack_top = vm.stack;
    vm.frame_count = 0;
    vm.handler_count = 0;
}

void init_vm(void) {
    reset_stack();
    init_table(&vm.globals);
    init_table(&vm.strings);
    vm.objects = NULL;
    register_native_functions();

    vm.debug_enabled = false;
    vm.debug_mode = DEBUG_NONE;
    vm.debug_step_frame_depth = 0;
    vm.debug_step_offset = 0;
    vm.debug_paused = false;
    vm.debug_just_stopped = false;
    vm.debug_stop_line = 0;
    vm.debug_stop_reason = NULL;
    vm.debug_source = NULL;
    vm.debug_source_length = 0;

    const char* native_names[] = {
        "str", "tensor", "matrix", "matmul", "sigmoid", "relu", "mse",
        "http_server", "http_start", "http_request", "sqrt", "math",
        "upper", "lower", "trim", "split", "contains", "replace",
        "substring", "starts_with", "ends_with", "length",
        "remove", "pop", "sort",
        "read_file", "write_file", "import_file",
        NULL
    };
    for (int i = 0; native_names[i] != NULL; i++) {
        ObjString* name = copy_string(native_names[i], (int)strlen(native_names[i]));
        push(OBJ_VAL(name));
        table_set(&vm.globals, name, OBJ_VAL(name));
        pop();
    }
}

void free_vm(void) {
    free_table(&vm.globals);
    free_table(&vm.strings);
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

static bool call(ObjFunction* function, int arg_count) {
    if (arg_count != function->arity) {
        runtime_error("Expected %d arguments but got %d.",
                      function->arity, arg_count);
        return false;
    }

    if (vm.frame_count == FRAMES_MAX) {
        runtime_error("Stack overflow.");
        return false;
    }

    CallFrame* frame = &vm.frames[vm.frame_count++];
    frame->function = function;
    frame->ip = function->chunk.code;
    frame->slots = vm.stack_top - arg_count - 1;
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
}

void vm_debug_step_in(void) {
    vm.debug_mode = DEBUG_STEP_IN;
    vm.debug_paused = false;
}

void vm_debug_step_over(void) {
    vm.debug_mode = DEBUG_STEP_OVER;
    vm.debug_step_frame_depth = vm.frame_count;
    vm.debug_paused = false;
}

void vm_debug_step_out(void) {
    vm.debug_mode = DEBUG_STEP_OUT;
    vm.debug_step_frame_depth = vm.frame_count;
    vm.debug_paused = false;
}

void vm_debug_pause(void) {
    vm.debug_mode = DEBUG_PAUSE;
    vm.debug_paused = false;
}

int vm_get_current_line(void) {
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
        int offset = (int)(frame->ip - frame->function->chunk.code - 1);
        if (offset >= 0 && offset < frame->function->chunk.count) {
            lines[i] = frame->function->chunk.lines[offset];
        } else {
            lines[i] = 0;
        }
    }
}

void vm_get_variables(int frame_idx, const char** names, Value* values, int* count) {
    if (frame_idx < 0 || frame_idx >= vm.frame_count) {
        *count = 0;
        return;
    }

    CallFrame* frame = &vm.frames[frame_idx];
    ObjFunction* func = frame->function;

    int max_locals = func->arity;
    for (int i = 0; i < func->chunk.debug_func_count; i++) {
        DebugFuncInfo* df = &func->chunk.debug_funcs[i];
        if (df->local_count > max_locals) {
            max_locals = df->local_count;
        }
    }

    int idx = 0;

    if (func->arity > 0 && idx < 256) {
        names[idx] = "self";
        values[idx] = frame->slots[0];
        idx++;
    }

    for (int i = 1; i < max_locals && idx < 256; i++) {
        if (&frame->slots[i] < vm.stack_top) {
            char* buf = (char*)malloc(32);
            snprintf(buf, 32, "var%d", i);
            names[idx] = buf;
            values[idx] = frame->slots[i];
            idx++;
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
        if (vm.debug_enabled) {
            int current_offset = (int)(frame->ip - frame->function->chunk.code);
            bool should_stop = false;

            if (vm.debug_mode == DEBUG_STEP_IN) {
                should_stop = true;
                vm.debug_stop_reason = "step";
            } else if (vm.debug_mode == DEBUG_STEP_OVER) {
                if (vm.frame_count <= vm.debug_step_frame_depth) {
                    should_stop = true;
                    vm.debug_stop_reason = "step";
                }
            } else if (vm.debug_mode == DEBUG_STEP_OUT) {
                if (vm.frame_count < vm.debug_step_frame_depth) {
                    should_stop = true;
                    vm.debug_stop_reason = "step";
                }
            } else if (vm.debug_mode == DEBUG_CONTINUE || vm.debug_mode == DEBUG_PAUSE) {
                if (chunk_has_breakpoint(&frame->function->chunk, current_offset)) {
                    should_stop = true;
                    vm.debug_stop_reason = "breakpoint";
                }
            }

            if (should_stop && current_offset > 0) {
                vm.debug_mode = DEBUG_NONE;
                vm.debug_paused = true;
                vm.debug_just_stopped = true;
                int line = frame->function->chunk.lines[current_offset - 1];
                vm.debug_stop_line = line;
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

        case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
        case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break;
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

        case OP_GREATER:      BINARY_OP(BOOL_VAL, >); break;
        case OP_LESS:         BINARY_OP(BOOL_VAL, <); break;
        case OP_GREATER_EQUAL: BINARY_OP(BOOL_VAL, >=); break;
        case OP_LESS_EQUAL:    BINARY_OP(BOOL_VAL, <=); break;

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
                Value result;
                if (call_native(name->chars, arg_count,
                                vm.stack_top - arg_count, &result)) {
                    vm.stack_top -= arg_count + 1;
                    push(result);
                    break;
                }
            }

            if (!IS_OBJ(callee) || AS_OBJ(callee)->type != OBJ_FUNCTION) {
                runtime_error("Can only call functions.");
                return INTERPRET_RUNTIME_ERROR;
            }

            ObjFunction* function = AS_FUNCTION(callee);
            if (!call(function, arg_count)) {
                return INTERPRET_RUNTIME_ERROR;
            }
            frame = &vm.frames[vm.frame_count - 1];
            break;
        }

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
            if (!list_set(list, i, value)) {
                runtime_error("List index out of bounds.");
                return INTERPRET_RUNTIME_ERROR;
            }
            push(value);
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
                    ObjBoundMethod* bound = new_bound_method(obj, AS_FUNCTION(method));
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
            if (!call(AS_FUNCTION(method_val), arg_count + 1)) {
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
            ObjBoundMethod* super_bound = new_bound_method(bound->receiver, AS_FUNCTION(method));
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
            ObjFunction* func = AS_FUNCTION(method_val);
            if (!call(func, arg_count)) {
                return INTERPRET_RUNTIME_ERROR;
            }
            frame = &vm.frames[vm.frame_count - 1];
            break;
        }

        case OP_RETURN: {
            Value result = pop();
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
    call(function, 0);

    InterpretResult result = run();

    free_chunk(&chunk);
    return result;
}

InterpretResult vm_exec(void) {
    return run();
}

bool vm_call(ObjFunction* func, int arg_count) {
    return call(func, arg_count);
}
