#ifndef jts_native_h
#define jts_native_h

#include "common.h"
#include "core/value.h"
#include "vm/vm.h"

void register_native_functions(void);
bool call_native(const char* name, int arg_count, Value* args, Value* result);

#endif
