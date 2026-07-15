#ifndef jts_vm_h
#define jts_vm_h

#include "common.h"
#include "core/chunk.h"
#include "core/table.h"
#include "core/object.h"

#define STACK_MAX 256
#define FRAMES_MAX 64
#define MAX_EXCEPTION_HANDLERS 64

typedef struct {
    ObjFunction* function;
    uint8_t* ip;
    Value* slots;
} CallFrame;

typedef struct {
    uint8_t* handler_ip;
    Value* saved_stack_top;
    int saved_frame_count;
} ExceptionHandler;

typedef struct {
    CallFrame frames[FRAMES_MAX];
    int frame_count;
    Value stack[STACK_MAX];
    Value* stack_top;
    Table globals;
    Table strings;
    Obj* objects;
    ExceptionHandler handlers[MAX_EXCEPTION_HANDLERS];
    int handler_count;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

void init_vm(void);
void free_vm(void);
InterpretResult interpret(const char* source);
InterpretResult vm_exec(void);
bool vm_call(ObjFunction* func, int arg_count);
void push(Value value);
Value pop(void);

#endif
