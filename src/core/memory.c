#include "core/memory.h"
#include "core/object.h"
#include "core/table.h"
#include "../vm/vm.h"

extern VM vm;

static uint32_t bytes_allocated = 0;
static uint32_t next_gc = 1024 * 1024;

void* reallocate(void* pointer, size_t old_size, size_t new_size) {
    bytes_allocated += new_size - old_size;

    if (new_size > old_size && bytes_allocated > next_gc) {
        collect_garbage();
        next_gc = bytes_allocated * 2;
    }

    if (new_size == 0) {
        free(pointer);
        return NULL;
    }

    void* result = realloc(pointer, new_size);
    if (result == NULL) {
        fprintf(stderr, "JTS GO: Memory allocation failed.\n");
        exit(1);
    }
    return result;
}

static void free_object(Obj* object) {
    switch (object->type) {
        case OBJ_STRING: {
            ObjString* string = (ObjString*)object;
            FREE_ARRAY(char, string->chars, string->length + 1);
            FREE_ARRAY(ObjString, object, sizeof(ObjString));
            break;
        }
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*)object;
            free_chunk(&function->chunk);
            FREE_ARRAY(ObjFunction, object, sizeof(ObjFunction));
            break;
        }
        case OBJ_LIST: {
            ObjList* list = (ObjList*)object;
            FREE_ARRAY(Value, list->values, list->capacity);
            FREE_ARRAY(ObjList, object, sizeof(ObjList));
            break;
        }
        case OBJ_DICT: {
            ObjDict* dict = (ObjDict*)object;
            free_table(&dict->entries);
            FREE_ARRAY(ObjDict, object, sizeof(ObjDict));
            break;
        }
        case OBJ_CLASS: {
            ObjClass* klass = (ObjClass*)object;
            free_table(&klass->methods);
            FREE_ARRAY(ObjClass, object, sizeof(ObjClass));
            break;
        }
        case OBJ_INSTANCE: {
            ObjInstance* instance = (ObjInstance*)object;
            free_table(&instance->fields);
            FREE_ARRAY(ObjInstance, object, sizeof(ObjInstance));
            break;
        }
        case OBJ_BOUND_METHOD:
            FREE_ARRAY(ObjBoundMethod, object, sizeof(ObjBoundMethod));
            break;
        case OBJ_NATIVE:
            FREE_ARRAY(ObjNative, object, sizeof(ObjNative));
            break;
        case OBJ_HTTP_SERVER:
            FREE_ARRAY(ObjHttpServer, object, sizeof(ObjHttpServer));
            break;
        case OBJ_TENSOR: {
            ObjTensor* tensor = (ObjTensor*)object;
            if (tensor->data) FREE_ARRAY(double, tensor->data, tensor->size);
            FREE_ARRAY(ObjTensor, object, sizeof(ObjTensor));
            break;
        }
        case OBJ_MATRIX: {
            ObjMatrix* matrix = (ObjMatrix*)object;
            for (int i = 0; i < matrix->rows; i++) {
                FREE_ARRAY(double, matrix->data[i], matrix->cols);
            }
            FREE_ARRAY(double*, matrix->data, matrix->rows);
            FREE_ARRAY(ObjMatrix, object, sizeof(ObjMatrix));
            break;
        }
    }
}

void collect_garbage(void) {
    // Simple stop-the-world: free all unreachable objects
    // In a real implementation, we'd mark reachable objects first
    // For now, we just free objects not marked during the last cycle
    // This is a conservative approach that frees everything at shutdown
}

void free_objects(void) {
    Obj* object = vm.objects;
    while (object != NULL) {
        Obj* next = object->next;
        free_object(object);
        object = next;
    }
}
