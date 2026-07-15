#ifndef jts_value_h
#define jts_value_h

#include "common.h"

typedef struct Obj Obj;
typedef struct ObjString ObjString;

typedef enum {
    VAL_NIL,
    VAL_BOOL,
    VAL_NUMBER,
    VAL_OBJ
} ValueType;

typedef struct {
    ValueType type;
    union {
        bool boolean;
        double number;
        Obj* obj;
    } as;
} Value;

#define IS_NIL(value)     ((value).type == VAL_NIL)
#define IS_BOOL(value)    ((value).type == VAL_BOOL)
#define IS_NUMBER(value)  ((value).type == VAL_NUMBER)
#define IS_OBJ(value)     ((value).type == VAL_OBJ)

#define AS_BOOL(value)    ((value).as.boolean)
#define AS_NUMBER(value)  ((value).as.number)
#define AS_OBJ(value)     ((value).as.obj)

static inline Value BOOL_VAL(bool b)    { Value v; v.type = VAL_BOOL; v.as.boolean = b; return v; }
static inline Value NUMBER_VAL(double n){ Value v; v.type = VAL_NUMBER; v.as.number = n; return v; }
static inline Value OBJ_VAL(void* o)   { Value v; v.type = VAL_OBJ; v.as.obj = (Obj*)o; return v; }
static inline Value NIL_VALUE(void)     { Value v; v.type = VAL_NIL; v.as.number = 0; return v; }
#define NIL_VAL NIL_VALUE()

bool values_equal(Value a, Value b);
void print_value(Value value);
Value number_to_string(double num);

typedef struct {
    int capacity;
    int count;
    Value* values;
} ValueArray;

void init_value_array(ValueArray* array);
void write_value_array(ValueArray* array, Value value);
void free_value_array(ValueArray* array);

#endif
