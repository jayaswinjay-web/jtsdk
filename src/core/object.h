#ifndef jts_object_h
#define jts_object_h

#include "common.h"
#include "core/value.h"
#include "core/chunk.h"
#include "core/table.h"

typedef enum {
    OBJ_STRING,
    OBJ_FUNCTION,
    OBJ_CLOSURE,
    OBJ_UPVALUE,
    OBJ_LIST,
    OBJ_DICT,
    OBJ_SET,
    OBJ_CLASS,
    OBJ_INSTANCE,
    OBJ_BOUND_METHOD,
    OBJ_NATIVE,
    OBJ_HTTP_SERVER,
    OBJ_TENSOR,
    OBJ_MATRIX,
    OBJ_GENERATOR,
} ObjType;

struct Obj {
    ObjType type;
    struct Obj* next;
};

struct ObjString {
    Obj obj;
    int length;
    char* chars;
    uint32_t hash;
};

typedef struct ObjFunction {
    Obj obj;
    int arity;
    int max_arity;
    int upvalue_count;
    Chunk chunk;
    ObjString* name;
    bool is_generator;
} ObjFunction;

typedef struct ObjUpvalue {
    Obj obj;
    Value* location;
    Value closed;
    struct ObjUpvalue* next;
} ObjUpvalue;

typedef struct ObjClosure {
    Obj obj;
    ObjFunction* function;
    ObjUpvalue** upvalues;
    int upvalue_count;
} ObjClosure;

typedef struct {
    Obj obj;
    int capacity;
    int count;
    Value* values;
} ObjList;

typedef struct ObjDict {
    Obj obj;
    Table entries;
} ObjDict;

typedef struct {
    Obj obj;
    int capacity;
    int count;
    Value* values;
} ObjSet;

typedef struct ObjClass {
    Obj obj;
    ObjString* name;
    Table methods;
} ObjClass;

typedef struct ObjInstance {
    Obj obj;
    ObjClass* klass;
    Table fields;
} ObjInstance;

typedef struct ObjBoundMethod {
    Obj obj;
    Value receiver;
    Value method;
} ObjBoundMethod;

typedef struct {
    Obj obj;
    int arity;
    bool (*function)(int arg_count, Value* args, Value* result);
} ObjNative;

#define MAX_ROUTES 64
#define MAX_METHOD_LEN 8
#define MAX_PATH_LEN 256

typedef struct {
    char method[MAX_METHOD_LEN];
    char path[MAX_PATH_LEN];
    Value body;
} HttpRoute;

typedef struct ObjHttpServer {
    Obj obj;
    int port;
    Value handler;
    bool running;
    HttpRoute routes[MAX_ROUTES];
    int route_count;
} ObjHttpServer;

typedef struct ObjTensor {
    Obj obj;
    int ndim;
    int* shape;
    int size;
    double* data;
} ObjTensor;

typedef struct ObjMatrix {
    Obj obj;
    int rows;
    int cols;
    double** data;
} ObjMatrix;

typedef struct ObjGenerator {
    Obj obj;
    ObjClosure* closure;
    int ip;
    Value* slots;
    int slot_count;
    Value* args;
    int arg_count;
    bool exhausted;
    Value* saved_slots;
    int saved_slot_count;
} ObjGenerator;

#define IS_STRING(value)    (is_obj_type(value, OBJ_STRING))
#define IS_FUNCTION(value)  (is_obj_type(value, OBJ_FUNCTION))
#define IS_CLOSURE(value)   (is_obj_type(value, OBJ_CLOSURE))
#define IS_UPVALUE(value)   (is_obj_type(value, OBJ_UPVALUE))
#define IS_LIST(value)      (is_obj_type(value, OBJ_LIST))
#define IS_DICT(value)      (is_obj_type(value, OBJ_DICT))
#define IS_SET(value)       (is_obj_type(value, OBJ_SET))
#define IS_CLASS(value)     (is_obj_type(value, OBJ_CLASS))
#define IS_INSTANCE(value)  (is_obj_type(value, OBJ_INSTANCE))
#define IS_BOUND_METHOD(value) (is_obj_type(value, OBJ_BOUND_METHOD))
#define IS_NATIVE(value)    (is_obj_type(value, OBJ_NATIVE))
#define IS_HTTP_SERVER(value) (is_obj_type(value, OBJ_HTTP_SERVER))
#define IS_TENSOR(value)    (is_obj_type(value, OBJ_TENSOR))
#define IS_MATRIX(value)    (is_obj_type(value, OBJ_MATRIX))
#define IS_GENERATOR(value) (is_obj_type(value, OBJ_GENERATOR))

#define AS_STRING(value)    ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)   (((ObjString*)AS_OBJ(value))->chars)
#define AS_FUNCTION(value)  ((ObjFunction*)AS_OBJ(value))
#define AS_CLOSURE(value)   ((ObjClosure*)AS_OBJ(value))
#define AS_UPVALUE(value)   ((ObjUpvalue*)AS_OBJ(value))
#define AS_LIST(value)      ((ObjList*)AS_OBJ(value))
#define AS_DICT(value)      ((ObjDict*)AS_OBJ(value))
#define AS_SET(value)       ((ObjSet*)AS_OBJ(value))
#define AS_CLASS(value)     ((ObjClass*)AS_OBJ(value))
#define AS_INSTANCE(value)  ((ObjInstance*)AS_OBJ(value))
#define AS_BOUND_METHOD(value) ((ObjBoundMethod*)AS_OBJ(value))
#define AS_NATIVE(value)    ((ObjNative*)AS_OBJ(value))
#define AS_HTTP_SERVER(value) ((ObjHttpServer*)AS_OBJ(value))
#define AS_TENSOR(value)    ((ObjTensor*)AS_OBJ(value))
#define AS_MATRIX(value)    ((ObjMatrix*)AS_OBJ(value))
#define AS_GENERATOR(value) ((ObjGenerator*)AS_OBJ(value))

ObjString* take_string(char* chars, int length);
ObjString* copy_string(const char* chars, int length);
ObjFunction* new_function(void);
ObjClosure* new_closure(ObjFunction* function);
ObjUpvalue* new_upvalue(Value* slot);
ObjList* new_list(void);
void list_append(ObjList* list, Value value);
Value list_get(ObjList* list, int index);
bool list_set(ObjList* list, int index, Value value);
ObjDict* new_dict(void);
void dict_set(ObjDict* dict, ObjString* key, Value value);
bool dict_get(ObjDict* dict, ObjString* key, Value* result);
ObjSet* new_set(void);
void set_add(ObjSet* set, Value value);
bool set_contains(ObjSet* set, Value value);
void set_remove(ObjSet* set, Value value);
ObjClass* new_class(ObjString* name);
ObjInstance* new_instance(ObjClass* klass);
ObjBoundMethod* new_bound_method(Value receiver, Value method);
ObjNative* new_native(int arity, bool (*function)(int, Value*, Value*));
ObjHttpServer* new_http_server(int port);
ObjTensor* new_tensor(int ndim, int* shape, double* data);
ObjMatrix* new_matrix(int rows, int cols, double** data);
ObjGenerator* new_generator(ObjClosure* closure);
void print_obj(Value value);

static inline bool is_obj_type(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif
