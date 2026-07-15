#include <stdio.h>
#include <string.h>
#include <math.h>

#include "core/memory.h"
#include "core/object.h"
#include "core/value.h"
#include "core/chunk.h"

Obj* objects = NULL;

static Obj* allocate_object(size_t size, ObjType type) {
    Obj* object = (Obj*)reallocate(NULL, 0, size);
    object->type = type;
    object->next = objects;
    objects = object;
    return object;
}

#define ALLOCATE_OBJ(type, objectType) \
    (type*)allocate_object(sizeof(type), objectType)

static uint32_t hash_string(const char* key, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619u;
    }
    return hash;
}

ObjString* take_string(char* chars, int length) {
    uint32_t hash = hash_string(chars, length);
    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;
    return string;
}

ObjString* copy_string(const char* chars, int length) {
    uint32_t hash = hash_string(chars, length);
    char* heap_chars = ALLOCATE(char, length + 1);
    memcpy(heap_chars, chars, length);
    heap_chars[length] = '\0';
    return take_string(heap_chars, length);
}

ObjFunction* new_function(void) {
    ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    function->arity = 0;
    function->name = NULL;
    init_chunk(&function->chunk);
    return function;
}

ObjList* new_list(void) {
    ObjList* list = ALLOCATE_OBJ(ObjList, OBJ_LIST);
    list->capacity = 0;
    list->count = 0;
    list->values = NULL;
    return list;
}

void list_append(ObjList* list, Value value) {
    if (list->capacity < list->count + 1) {
        int old_capacity = list->capacity;
        list->capacity = GROW_CAPACITY(old_capacity);
        list->values = GROW_ARRAY(Value, list->values, old_capacity, list->capacity);
    }
    list->values[list->count] = value;
    list->count++;
}

Value list_get(ObjList* list, int index) {
    if (index < 0 || index >= list->count) {
        return NIL_VAL;
    }
    return list->values[index];
}

bool list_set(ObjList* list, int index, Value value) {
    if (index < 0 || index >= list->count) {
        return false;
    }
    list->values[index] = value;
    return true;
}

ObjDict* new_dict(void) {
    ObjDict* dict = ALLOCATE_OBJ(ObjDict, OBJ_DICT);
    init_table(&dict->entries);
    return dict;
}

void dict_set(ObjDict* dict, ObjString* key, Value value) {
    table_set(&dict->entries, key, value);
}

bool dict_get(ObjDict* dict, ObjString* key, Value* result) {
    return table_get(&dict->entries, key, result);
}

ObjClass* new_class(ObjString* name) {
    ObjClass* klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
    klass->name = name;
    init_table(&klass->methods);
    return klass;
}

ObjInstance* new_instance(ObjClass* klass) {
    ObjInstance* instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
    instance->klass = klass;
    init_table(&instance->fields);
    return instance;
}

ObjBoundMethod* new_bound_method(Value receiver, ObjFunction* method) {
    ObjBoundMethod* bound = ALLOCATE_OBJ(ObjBoundMethod, OBJ_BOUND_METHOD);
    bound->receiver = receiver;
    bound->method = method;
    return bound;
}

ObjNative* new_native(int arity, bool (*function)(int, Value*, Value*)) {
    ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->arity = arity;
    native->function = function;
    return native;
}

ObjHttpServer* new_http_server(int port) {
    ObjHttpServer* server = ALLOCATE_OBJ(ObjHttpServer, OBJ_HTTP_SERVER);
    server->port = port;
    server->handler = NIL_VAL;
    server->running = false;
    return server;
}

ObjTensor* new_tensor(int ndim, int* shape, double* data) {
    ObjTensor* tensor = ALLOCATE_OBJ(ObjTensor, OBJ_TENSOR);
    tensor->ndim = ndim;
    tensor->shape = shape;
    int size = 1;
    for (int i = 0; i < ndim; i++) {
        size *= shape[i];
    }
    tensor->size = size;
    tensor->data = data;
    return tensor;
}

ObjMatrix* new_matrix(int rows, int cols, double** data) {
    ObjMatrix* matrix = ALLOCATE_OBJ(ObjMatrix, OBJ_MATRIX);
    matrix->rows = rows;
    matrix->cols = cols;
    matrix->data = data;
    return matrix;
}

void print_obj(Value value) {
    switch (AS_OBJ(value)->type) {
        case OBJ_STRING:
            printf("%s", AS_CSTRING(value));
            break;
        case OBJ_FUNCTION:
            if (AS_FUNCTION(value)->name != NULL) {
                printf("<func %s>", AS_FUNCTION(value)->name->chars);
            } else {
                printf("<script>");
            }
            break;
        case OBJ_LIST: {
            ObjList* list = AS_LIST(value);
            printf("[");
            for (int i = 0; i < list->count; i++) {
                if (i > 0) printf(", ");
                print_value(list->values[i]);
            }
            printf("]");
            break;
        }
        case OBJ_DICT: {
            ObjDict* dict = AS_DICT(value);
            printf("{");
            bool first = true;
            for (int i = 0; i < dict->entries.capacity; i++) {
                if (dict->entries.entries[i].key != NULL) {
                    if (!first) printf(", ");
                    first = false;
                    print_value(OBJ_VAL(dict->entries.entries[i].key));
                    printf(": ");
                    print_value(dict->entries.entries[i].value);
                }
            }
            printf("}");
            break;
        }
        case OBJ_CLASS:
            printf("<class %s>", AS_CLASS(value)->name->chars);
            break;
        case OBJ_INSTANCE:
            printf("<%s instance>", AS_INSTANCE(value)->klass->name->chars);
            break;
        case OBJ_BOUND_METHOD:
            printf("<bound method %s>", AS_BOUND_METHOD(value)->method->name->chars);
            break;
        case OBJ_NATIVE:
            printf("<native function>");
            break;
        case OBJ_HTTP_SERVER:
            printf("<http server on port %d>", AS_HTTP_SERVER(value)->port);
            break;
        case OBJ_TENSOR: {
            ObjTensor* t = AS_TENSOR(value);
            printf("<tensor shape=(");
            for (int i = 0; i < t->ndim; i++) {
                if (i > 0) printf(", ");
                printf("%d", t->shape[i]);
            }
            printf(")>");
            break;
        }
        case OBJ_MATRIX:
            printf("<matrix %dx%d>", AS_MATRIX(value)->rows, AS_MATRIX(value)->cols);
            break;
    }
}
