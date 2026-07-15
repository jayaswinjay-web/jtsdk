#include "core/value.h"
#include "core/object.h"
#include "core/memory.h"

void init_value_array(ValueArray* array) {
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void write_value_array(ValueArray* array, Value value) {
    if (array->capacity < array->count + 1) {
        int old_capacity = array->capacity;
        array->capacity = GROW_CAPACITY(old_capacity);
        array->values = GROW_ARRAY(Value, array->values,
                                   old_capacity, array->capacity);
    }
    array->values[array->count] = value;
    array->count++;
}

void free_value_array(ValueArray* array) {
    FREE_ARRAY(Value, array->values, array->capacity);
    init_value_array(array);
}

bool values_equal(Value a, Value b) {
    if (a.type != b.type) {
        if (a.type == VAL_NUMBER && b.type == VAL_NUMBER) {
            return a.as.number == b.as.number;
        }
        return false;
    }
    switch (a.type) {
        case VAL_NIL:    return true;
        case VAL_BOOL:   return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
        case VAL_OBJ:    return AS_OBJ(a) == AS_OBJ(b);
        default:         return false;
    }
}

static void print_number(double number) {
    if (number == (long long)number) {
        printf("%lld", (long long)number);
    } else {
        printf("%g", number);
    }
}

void print_value(Value value) {
    switch (value.type) {
        case VAL_NIL:     printf("nil"); break;
        case VAL_BOOL:    printf(AS_BOOL(value) ? "true" : "false"); break;
        case VAL_NUMBER:  print_number(AS_NUMBER(value)); break;
        case VAL_OBJ:     print_obj(value); break;
    }
}

Value number_to_string(double num) {
    char buffer[64];
    if (num == (long long)num) {
        snprintf(buffer, sizeof(buffer), "%lld", (long long)num);
    } else {
        snprintf(buffer, sizeof(buffer), "%g", num);
    }
    ObjString* result = copy_string(buffer, (int)strlen(buffer));
    return OBJ_VAL(result);
}
