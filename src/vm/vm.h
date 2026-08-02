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
    ObjClosure* closure;
    uint8_t* ip;
    Value* slots;
    int arg_count;
    ObjGenerator* generator;
} CallFrame;

typedef struct {
    uint8_t* handler_ip;
    Value* saved_stack_top;
    int saved_frame_count;
} ExceptionHandler;

typedef enum {
    DEBUG_NONE,
    DEBUG_CONTINUE,
    DEBUG_STEP_IN,
    DEBUG_STEP_OVER,
    DEBUG_STEP_OUT,
    DEBUG_PAUSE
} DebugMode;

typedef struct {
    CallFrame frames[FRAMES_MAX];
    int frame_count;
    Value stack[STACK_MAX];
    Value* stack_top;
    Table globals;
    Table strings;
    Table scrolls_loaded;
    Obj* objects;
    ObjUpvalue* open_upvalues;
    ExceptionHandler handlers[MAX_EXCEPTION_HANDLERS];
    int handler_count;

    const char* program_dir;
    const char* install_dir;

    bool debug_enabled;
    DebugMode debug_mode;
    int debug_step_frame_depth;
    int debug_step_offset;
    bool debug_paused;
    bool debug_just_stopped;
    bool debug_resume_skip;
    int debug_stop_line;
    const char* debug_stop_reason;
    char* debug_source;
    int debug_source_length;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
    INTERPRET_YIELD
} InterpretResult;

extern VM vm;

void init_vm(void);
void free_vm(void);
InterpretResult interpret(const char* source);
InterpretResult interpret_isolated(const char* source);
InterpretResult vm_exec(void);
bool vm_call(Value func, int arg_count);
void push(Value value);
Value pop(void);
void vm_set_dirs(const char* program_dir, const char* install_dir);

void vm_set_debug_enabled(bool enabled);
bool vm_is_debug_paused(void);
void vm_debug_continue(void);
void vm_debug_step_in(void);
void vm_debug_step_over(void);
void vm_debug_step_out(void);
void vm_debug_pause(void);
int vm_get_current_line(void);
const char* vm_get_current_file(void);
const char* vm_get_debug_stop_reason(void);
void vm_set_breakpoints(Chunk* chunk, int* offsets, int count);
void vm_get_stack_frame_names(const char** names, int* lines, int* count);
void vm_get_variables(int frame, const char** names, int* name_lengths, Value* values, int* count);
void vm_get_globals(const char** names, Value* values, int* count, int* total);
const char* vm_get_source(void);
int vm_get_source_length(void);

InterpretResult vm_resume_generator(ObjGenerator* generator);

#endif
