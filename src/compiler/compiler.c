#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler/compiler.h"
#include "core/object.h"
#include "core/memory.h"

#ifdef DEBUG_PRINT_CODE
#include "vm/debug.h"
#endif

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,
    PREC_OR,
    PREC_AND,
    PREC_EQUALITY,
    PREC_COMPARISON,
    PREC_BIT_OR,
    PREC_BIT_XOR,
    PREC_BIT_AND,
    PREC_SHIFT,
    PREC_TERM,
    PREC_FACTOR,
    PREC_UNARY,
    PREC_CALL,
    PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(Compiler* compiler, bool can_assign);

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

static void expression(Compiler* compiler);
static void statement(Compiler* compiler);
static void declaration(Compiler* compiler);
static void expression_statement(Compiler* compiler);
static void del_statement(Compiler* compiler);
static void assert_statement(Compiler* compiler);
static ParseRule* get_rule(TokenType type);
static void parse_precedence(Compiler* compiler, Precedence precedence);

// Loop control: break/continue need to know current loop depth and patch targets
static int loop_depth = 0;
static int break_jumps[256];
static int break_count = 0;
static int continue_jumps[256];
static int continue_count = 0;

static Chunk* current_chunk(Compiler* compiler) {
    return &compiler->function->chunk;
}

static void error_at(Compiler* compiler, Token* token, const char* message) {
    if (compiler->panic_mode) return;
    compiler->panic_mode = true;
    fprintf(stderr, "[line %d] Error", token->line);
    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type != TOKEN_ERROR) {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }
    fprintf(stderr, ": %s\n", message);
    compiler->had_error = true;
}

static void error(Compiler* compiler, const char* message) {
    error_at(compiler, &compiler->previous, message);
}

static void error_at_current(Compiler* compiler, const char* message) {
    error_at(compiler, &compiler->current, message);
}

static void advance(Compiler* compiler) {
    compiler->previous = compiler->current;
    for (;;) {
        compiler->current = scan_token(compiler->scanner);
        if (compiler->current.type != TOKEN_ERROR) break;
    }
}

static bool match(Compiler* compiler, TokenType type) {
    if (compiler->current.type != type) return false;
    advance(compiler);
    return true;
}

static void consume(Compiler* compiler, TokenType type, const char* message) {
    if (compiler->current.type == type) {
        advance(compiler);
        return;
    }
    error_at_current(compiler, message);
}

static void emit_byte(Compiler* compiler, uint8_t byte) {
    write_chunk(current_chunk(compiler), byte, compiler->previous.line);
}

static void emit_bytes(Compiler* compiler, uint8_t byte1, uint8_t byte2) {
    emit_byte(compiler, byte1);
    emit_byte(compiler, byte2);
}

static int emit_jump(Compiler* compiler, uint8_t instruction) {
    emit_byte(compiler, instruction);
    emit_byte(compiler, 0xff);
    emit_byte(compiler, 0xff);
    return current_chunk(compiler)->count - 2;
}

static void emit_loop(Compiler* compiler, int loop_start) {
    emit_byte(compiler, OP_LOOP);
    int offset = current_chunk(compiler)->count - loop_start + 2;
    if (offset > UINT16_MAX) error(compiler, "Loop body too large");
    emit_byte(compiler, (offset >> 8) & 0xff);
    emit_byte(compiler, offset & 0xff);
}

static void patch_jump(Compiler* compiler, int offset) {
    int jump = current_chunk(compiler)->count - offset - 2;
    if (jump > UINT16_MAX) {
        error(compiler, "Too much code to jump over");
    }
    current_chunk(compiler)->code[offset] = (jump >> 8) & 0xff;
    current_chunk(compiler)->code[offset + 1] = jump & 0xff;
}

static void emit_return(Compiler* compiler) {
    emit_byte(compiler, OP_VOID);
    emit_byte(compiler, OP_RETURN);
}

static uint8_t make_constant(Compiler* compiler, Value value) {
    int constant = add_constant(current_chunk(compiler), value);
    if (constant > UINT8_MAX) {
        error(compiler, "Too many constants in one chunk");
        return 0;
    }
    return (uint8_t)constant;
}

static void emit_constant(Compiler* compiler, Value value) {
    emit_bytes(compiler, OP_CONSTANT, make_constant(compiler, value));
}

static void begin_scope(Compiler* compiler) {
    compiler->scope_depth++;
}

static void end_scope(Compiler* compiler) {
    compiler->scope_depth--;
    while (compiler->local_count > 0 &&
           compiler->locals[compiler->local_count - 1].depth > compiler->scope_depth) {
        if (compiler->locals[compiler->local_count - 1].is_captured) {
            emit_byte(compiler, OP_CLOSE_UPVALUE);
        } else {
            emit_byte(compiler, OP_POP);
        }
        compiler->local_count--;
    }
}

static uint8_t identifier_constant(Compiler* compiler, Token* name) {
    return make_constant(compiler,
        OBJ_VAL(copy_string(name->start, name->length)));
}

static bool identifiers_equal(Token* a, Token* b) {
    if (a->length != b->length) return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

static int resolve_local(Compiler* compiler, Token* name) {
    for (int i = compiler->local_count - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];
        if (identifiers_equal(name, &local->name)) {
            if (local->depth == -1) {
                error(compiler, "Can't read local variable in its own initializer");
            }
            return i;
        }
    }
    return -1;
}

static int add_upvalue(Compiler* compiler, uint8_t index, bool is_local) {
    int upvalue_count = compiler->upvalue_count;
    for (int i = 0; i < upvalue_count; i++) {
        Upvalue* upvalue = &compiler->upvalues[i];
        if (upvalue->index == index && upvalue->is_local == is_local) {
            return i;
        }
    }
    if (upvalue_count == UINT8_COUNT) {
        error(compiler, "Too many closure variables in function");
        return 0;
    }
    compiler->upvalues[upvalue_count].index = index;
    compiler->upvalues[upvalue_count].is_local = is_local;
    compiler->upvalue_count++;
    return compiler->upvalue_count - 1;
}

static int resolve_upvalue(Compiler* compiler, Token* name) {
    if (compiler->parent == NULL) return -1;
    int local = resolve_local(compiler->parent, name);
    if (local != -1) {
        compiler->parent->locals[local].is_captured = true;
        return add_upvalue(compiler, (uint8_t)local, true);
    }
    int upvalue = resolve_upvalue(compiler->parent, name);
    if (upvalue != -1) {
        return add_upvalue(compiler, (uint8_t)upvalue, false);
    }
    return -1;
}

static void add_local(Compiler* compiler, Token name) {
    if (compiler->local_count == MAX_LOCALS) {
        error(compiler, "Too many local variables");
        return;
    }
    Local* local = &compiler->locals[compiler->local_count++];
    local->name = name;
    local->depth = -1;
    local->is_captured = false;

    if (compiler->debug_func != NULL) {
        Chunk* chunk = current_chunk(compiler);
        debug_func_add_local(compiler->debug_func,
                             chunk->count, compiler->local_count - 1,
                             name.start, name.length, compiler->scope_depth);
    }
}

static void declare_variable(Compiler* compiler) {
    if (compiler->scope_depth == 0) return;

    Token* name = &compiler->previous;
    for (int i = compiler->local_count - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];
        if (local->depth != -1 && local->depth < compiler->scope_depth) break;
        if (identifiers_equal(name, &local->name)) {
            error(compiler, "Variable with this name already exists in this scope");
            return;
        }
    }
    add_local(compiler, *name);
}

static uint8_t parse_variable(Compiler* compiler, const char* message) {
    consume(compiler, TOKEN_IDENTIFIER, message);
    declare_variable(compiler);
    if (compiler->scope_depth > 0) return 0;
    return identifier_constant(compiler, &compiler->previous);
}

static void mark_initialized(Compiler* compiler) {
    if (compiler->scope_depth == 0) return;
    compiler->locals[compiler->local_count - 1].depth = compiler->scope_depth;
}

static void define_variable(Compiler* compiler, uint8_t global) {
    if (compiler->scope_depth > 0) {
        mark_initialized(compiler);
        return;
    }
    emit_bytes(compiler, OP_DEFINE_GLOBAL, global);
}

static void expression(Compiler* compiler) {
    parse_precedence(compiler, PREC_ASSIGNMENT);
}

static void print_statement(Compiler* compiler) {
    expression(compiler);
    emit_byte(compiler, OP_SAY);
}

static void input_expression(Compiler* compiler, bool can_assign) {
    emit_constant(compiler, OBJ_VAL(copy_string("ask", 3)));
    consume(compiler, TOKEN_LEFT_PAREN, "Expect '(' after 'ask'");
    if (compiler->current.type != TOKEN_RIGHT_PAREN) {
        expression(compiler);
    } else {
        emit_constant(compiler, OBJ_VAL(copy_string("", 0)));
    }
    consume(compiler, TOKEN_RIGHT_PAREN, "Expect ')' after input arguments");
    emit_bytes(compiler, OP_CALL, 1);
}

static void block(Compiler* compiler) {
    if (compiler->current.type == TOKEN_INDENT) {
        advance(compiler);
    }

    while (compiler->current.type != TOKEN_DEDENT &&
           compiler->current.type != TOKEN_END &&
           compiler->current.type != TOKEN_CATCH &&
           compiler->current.type != TOKEN_FINALLY &&
           compiler->current.type != TOKEN_ELIF &&
           compiler->current.type != TOKEN_ELSE &&
           compiler->current.type != TOKEN_EOF) {
        if (compiler->current.type == TOKEN_NEWLINE) {
            advance(compiler);
            continue;
        }
        declaration(compiler);
    }

    if (compiler->current.type == TOKEN_DEDENT) {
        advance(compiler);
    }
}

static void if_statement(Compiler* compiler) {
    expression(compiler);

    int then_jump = emit_jump(compiler, OP_JUMP_IF_FALSE);
    emit_byte(compiler, OP_POP);

    if (compiler->current.type == TOKEN_NEWLINE) {
        advance(compiler);
    }

    begin_scope(compiler);
    block(compiler);
    end_scope(compiler);
    match(compiler, TOKEN_END);

    int else_jump = emit_jump(compiler, OP_JUMP);

    patch_jump(compiler, then_jump);
    emit_byte(compiler, OP_POP);

    int skip_jumps[256];
    int skip_count = 0;

    while (match(compiler, TOKEN_ELIF)) {
        expression(compiler);
        int elif_jump = emit_jump(compiler, OP_JUMP_IF_FALSE);
        emit_byte(compiler, OP_POP);

        if (compiler->current.type == TOKEN_NEWLINE) {
            advance(compiler);
        }

        begin_scope(compiler);
        block(compiler);
        end_scope(compiler);
        match(compiler, TOKEN_END);

        skip_jumps[skip_count++] = emit_jump(compiler, OP_JUMP);

        patch_jump(compiler, elif_jump);
        emit_byte(compiler, OP_POP);
    }

    if (compiler->current.type == TOKEN_ELSE) {
        advance(compiler);

        if (compiler->current.type == TOKEN_NEWLINE) {
            advance(compiler);
        }
        begin_scope(compiler);
        block(compiler);
        end_scope(compiler);
        match(compiler, TOKEN_END);
    }

    patch_jump(compiler, else_jump);

    for (int i = 0; i < skip_count; i++) {
        patch_jump(compiler, skip_jumps[i]);
    }
}

static void while_statement(Compiler* compiler) {
    int loop_start = current_chunk(compiler)->count;

    expression(compiler);

    int exit_jump = emit_jump(compiler, OP_JUMP_IF_FALSE);
    emit_byte(compiler, OP_POP);

    if (compiler->current.type == TOKEN_NEWLINE) {
        advance(compiler);
    }

    loop_depth++;
    int saved_break_count = break_count;
    int saved_continue_count = continue_count;

    begin_scope(compiler);
    block(compiler);
    end_scope(compiler);
    match(compiler, TOKEN_END);

    // Patch continue jumps to loop start
    for (int i = saved_continue_count; i < continue_count; i++) {
        int offset = current_chunk(compiler)->count - continue_jumps[i] - 2;
        current_chunk(compiler)->code[continue_jumps[i]] = (offset >> 8) & 0xff;
        current_chunk(compiler)->code[continue_jumps[i] + 1] = offset & 0xff;
    }
    continue_count = saved_continue_count;

    emit_loop(compiler, loop_start);

    // The condition value is still on the stack on the false path.
    // JUMP_IF_FALSE lands on this POP; 'break' lands after it so the
    // break path does not over-pop the value stack.
    patch_jump(compiler, exit_jump);
    emit_byte(compiler, OP_POP);

    // Patch break jumps to just after the exit pop
    for (int i = saved_break_count; i < break_count; i++) {
        patch_jump(compiler, break_jumps[i]);
    }
    break_count = saved_break_count;
    loop_depth--;
}

static void for_statement(Compiler* compiler) {
    begin_scope(compiler);

    consume(compiler, TOKEN_IDENTIFIER, "Expect variable name after 'for'");
    Token var_name = compiler->previous;
    uint8_t var_constant = identifier_constant(compiler, &var_name);

    consume(compiler, TOKEN_OF, "Expect 'of' after variable name");
    expression(compiler);

    if (match(compiler, TOKEN_TO)) {
        emit_bytes(compiler, OP_DEFINE_GLOBAL, var_constant);

        expression(compiler);

        Token end_name = (Token){TOKEN_IDENTIFIER, "_jts_for_end", 11, var_name.line};
        add_local(compiler, end_name);
        mark_initialized(compiler);
        int end_slot = compiler->local_count - 1;
        emit_bytes(compiler, OP_SET_LOCAL, (uint8_t)end_slot);

        int loop_start = current_chunk(compiler)->count;

        emit_bytes(compiler, OP_GET_GLOBAL, var_constant);
        emit_bytes(compiler, OP_GET_LOCAL, (uint8_t)end_slot);
        emit_byte(compiler, OP_LESS);

        int exit_jump = emit_jump(compiler, OP_JUMP_IF_FALSE);
        emit_byte(compiler, OP_POP);

        if (compiler->current.type == TOKEN_NEWLINE) {
            advance(compiler);
        }

        loop_depth++;
        int saved_break_count = break_count;
        int saved_continue_count = continue_count;

        begin_scope(compiler);
        block(compiler);
        end_scope(compiler);
        match(compiler, TOKEN_END);

        // Patch continue jumps to increment (they must land ON the
        // increment so the loop variable still advances)
        int increment_start = current_chunk(compiler)->count;

        emit_bytes(compiler, OP_GET_GLOBAL, var_constant);
        emit_constant(compiler, NUMBER_VAL(1));
        emit_byte(compiler, OP_ADD);
        emit_bytes(compiler, OP_SET_GLOBAL, var_constant);
        emit_byte(compiler, OP_POP);

        for (int i = saved_continue_count; i < continue_count; i++) {
            int offset = increment_start - continue_jumps[i] - 2;
            current_chunk(compiler)->code[continue_jumps[i]] = (offset >> 8) & 0xff;
            current_chunk(compiler)->code[continue_jumps[i] + 1] = offset & 0xff;
        }
        continue_count = saved_continue_count;

        emit_loop(compiler, loop_start);

        // Exit pop: JUMP_IF_FALSE lands on the POP (condition still on the
        // stack); 'break' lands after it to keep the stack balanced.
        patch_jump(compiler, exit_jump);
        emit_byte(compiler, OP_POP);

        // Patch break jumps
        for (int i = saved_break_count; i < break_count; i++) {
            patch_jump(compiler, break_jumps[i]);
        }
        break_count = saved_break_count;
        loop_depth--;

        end_scope(compiler);
        return;
    }

    /* Foreach: for var in iterable — iterate lists, strings, dicts, ranges */
    Token iter_name = (Token){TOKEN_IDENTIFIER, "_jts_iter", 9, var_name.line};
    Token idx_name = (Token){TOKEN_IDENTIFIER, "_jts_idx", 8, var_name.line};

    add_local(compiler, iter_name);
    mark_initialized(compiler);
    int iter_slot = compiler->local_count - 1;
    emit_bytes(compiler, OP_SET_LOCAL, (uint8_t)iter_slot);

    add_local(compiler, idx_name);
    mark_initialized(compiler);
    int idx_slot = compiler->local_count - 1;
    emit_constant(compiler, NUMBER_VAL(0));
    emit_bytes(compiler, OP_SET_LOCAL, (uint8_t)idx_slot);

    int loop_start = current_chunk(compiler)->count;

    emit_bytes(compiler, OP_GET_LOCAL, (uint8_t)idx_slot);
    emit_bytes(compiler, OP_GET_LOCAL, (uint8_t)iter_slot);
    emit_byte(compiler, OP_LEN);
    emit_byte(compiler, OP_LESS);

    int exit_jump = emit_jump(compiler, OP_JUMP_IF_FALSE);
    emit_byte(compiler, OP_POP);

    emit_bytes(compiler, OP_GET_LOCAL, (uint8_t)iter_slot);
    emit_bytes(compiler, OP_GET_LOCAL, (uint8_t)idx_slot);
    emit_byte(compiler, OP_ITER_VALUE);
    emit_bytes(compiler, OP_DEFINE_GLOBAL, var_constant);

    if (compiler->current.type == TOKEN_NEWLINE) {
        advance(compiler);
    }

    loop_depth++;
    int saved_break_count = break_count;
    int saved_continue_count = continue_count;

    begin_scope(compiler);
    block(compiler);
    end_scope(compiler);
    match(compiler, TOKEN_END);

    // Patch continue jumps to increment
    int increment_start = current_chunk(compiler)->count;

    emit_bytes(compiler, OP_GET_LOCAL, (uint8_t)idx_slot);
    emit_constant(compiler, NUMBER_VAL(1));
    emit_byte(compiler, OP_ADD);
    emit_bytes(compiler, OP_SET_LOCAL, (uint8_t)idx_slot);
    emit_byte(compiler, OP_POP);

    for (int i = saved_continue_count; i < continue_count; i++) {
        int offset = increment_start - continue_jumps[i] - 2;
        current_chunk(compiler)->code[continue_jumps[i]] = (offset >> 8) & 0xff;
        current_chunk(compiler)->code[continue_jumps[i] + 1] = offset & 0xff;
    }
    continue_count = saved_continue_count;

    emit_loop(compiler, loop_start);

    // Exit pop: JUMP_IF_FALSE lands on the POP (condition still on the
    // stack); 'break' lands after it to keep the stack balanced.
    patch_jump(compiler, exit_jump);
    emit_byte(compiler, OP_POP);

    // Patch break jumps
    for (int i = saved_break_count; i < break_count; i++) {
        patch_jump(compiler, break_jumps[i]);
    }
    break_count = saved_break_count;
    loop_depth--;

    end_scope(compiler);
}

static int argument_list(Compiler* compiler) {
    int arg_count = 0;
    if (compiler->current.type != TOKEN_RIGHT_PAREN) {
        do {
            expression(compiler);
            arg_count++;
            if (arg_count > 255) {
                error_at_current(compiler, "Too many arguments");
            }
        } while (match(compiler, TOKEN_COMMA));
    }
    consume(compiler, TOKEN_RIGHT_PAREN, "Expect ')' after arguments");
    return arg_count;
}

static void call_expr(Compiler* compiler, bool can_assign) {
    int arg_count = argument_list(compiler);
    emit_bytes(compiler, OP_CALL, (uint8_t)arg_count);
}

static void parse_parameters(Compiler* compiler) {
    int total = 0;
    int required = 0;
    bool seen_default = false;
    if (!match(compiler, TOKEN_RIGHT_PAREN)) {
        do {
            if (compiler->current.type == TOKEN_IDENTIFIER ||
                compiler->current.type == TOKEN_SELF) {
                advance(compiler);
            } else {
                consume(compiler, TOKEN_IDENTIFIER, "Expect parameter name");
            }
            declare_variable(compiler);
            mark_initialized(compiler);
            compiler->locals[compiler->local_count - 1].depth = 1;
            int slot = compiler->local_count - 1;
            total++;
            if (match(compiler, TOKEN_EQUAL)) {
                seen_default = true;
                emit_byte(compiler, OP_GET_ARGCOUNT);
                emit_constant(compiler, NUMBER_VAL((double)slot));
                emit_byte(compiler, OP_LESS);
                int skip = emit_jump(compiler, OP_JUMP_IF_FALSE);
                emit_byte(compiler, OP_POP);
                expression(compiler);
                emit_bytes(compiler, OP_SET_LOCAL, (uint8_t)slot);
                emit_byte(compiler, OP_POP);
                int done = emit_jump(compiler, OP_JUMP);
                patch_jump(compiler, skip);
                emit_byte(compiler, OP_POP);
                patch_jump(compiler, done);
            } else {
                if (seen_default) {
                    error(compiler, "Non-default parameter after default parameter");
                }
                required++;
            }
        } while (match(compiler, TOKEN_COMMA));
        consume(compiler, TOKEN_RIGHT_PAREN, "Expect ')' after parameters");
    }
    compiler->function->arity = required;
    compiler->function->max_arity = total;
}

// Set up a fresh function compiler and parse its parameter list. The scanner
// state is captured at the start of the parameter list so the body can be
// recompiled from scratch if hoisted locals are discovered.
static void begin_function_compile(Compiler* outer, Compiler* fn, Token name,
                                   const char* debug_name, bool detect,
                                   Scanner* out_saved, Token* out_cur,
                                   Token* out_prev) {
    *out_saved = *outer->scanner;
    *out_cur = outer->current;
    *out_prev = outer->previous;

    fn->parent = outer;
    fn->scanner = outer->scanner;
    fn->had_error = false;
    fn->panic_mode = false;
    fn->has_yield = false;
    fn->assign_created_local = false;
    fn->detect_hoists = detect;
    fn->hoist_count = 0;
    fn->local_count = 0;
    fn->scope_depth = 0;
    fn->upvalue_count = 0;
    fn->function_type = TYPE_FUNCTION;
    fn->current = outer->current;
    fn->previous = outer->previous;

    fn->function = new_function();
    if (name.length > 0) {
        fn->function->name = copy_string(name.start, name.length);
    } else {
        fn->function->name = NULL;
    }

    Chunk* chunk = current_chunk(fn);
    fn->debug_func = chunk_add_debug_func(chunk, debug_name,
                                          (int)strlen(debug_name), 0);

    Local* local = &fn->locals[fn->local_count++];
    local->depth = 0;
    local->name.start = "";
    local->name.length = 0;
    local->is_captured = false;

    begin_scope(fn);

    consume(fn, TOKEN_LEFT_PAREN, "Expect '(' after function name");

    parse_parameters(fn);
    if (fn->debug_func) {
        fn->debug_func->arity = fn->function->arity;
    }

    if (fn->current.type == TOKEN_NEWLINE) {
        advance(fn);
    }
}

// Compile a function body twice when needed: the first pass runs with
// detect_hoists set so that implicit locals assigned inside loops can be
// discovered. When any are found, the body is recompiled with those names
// hoisted to function scope (pre-filled with OP_VOID before the body) so their
// slots keep stable values across loop iterations instead of relying on the
// stack-adjacent value-as-slot convention.
static bool compile_function_body(Compiler* outer, Compiler* fn, Token name,
                                  const char* debug_name,
                                  Scanner* out_saved, Token* out_cur,
                                  Token* out_prev) {
    block(fn);
    match(fn, TOKEN_END);

    if (fn->hoist_count > 0 && !fn->had_error) {
        Token hoists[MAX_LOCALS];
        int hoist_count = fn->hoist_count;
        for (int i = 0; i < hoist_count; i++) {
            hoists[i] = fn->hoist_names[i];
        }

        *outer->scanner = *out_saved;
        outer->current = *out_cur;
        outer->previous = *out_prev;

        begin_function_compile(outer, fn, name, debug_name, false,
                               out_saved, out_cur, out_prev);

        for (int i = 0; i < hoist_count; i++) {
            add_local(fn, hoists[i]);
            mark_initialized(fn);
            emit_byte(fn, OP_VOID);
        }

        block(fn);
        match(fn, TOKEN_END);
        return true;
    }
    return false;
}

static void func_definition(Compiler* compiler) {
    consume(compiler, TOKEN_IDENTIFIER, "Expect function name");
    Token name = compiler->previous;

    Compiler fn_compiler;
    Scanner saved_scanner;
    Token saved_current;
    Token saved_previous;

    begin_function_compile(compiler, &fn_compiler, name, name.start, true,
                           &saved_scanner, &saved_current, &saved_previous);

    compile_function_body(compiler, &fn_compiler, name, name.start,
                          &saved_scanner, &saved_current, &saved_previous);

    emit_return(&fn_compiler);

    end_scope(&fn_compiler);

    ObjFunction* function = fn_compiler.function;
    function->is_generator = fn_compiler.has_yield;

    compiler->had_error = fn_compiler.had_error;
    compiler->current = fn_compiler.current;
    compiler->previous = fn_compiler.previous;

    if (fn_compiler.had_error) return;

    uint8_t constant = make_constant(compiler, OBJ_VAL(function));
    function->upvalue_count = fn_compiler.upvalue_count;
    emit_bytes(compiler, OP_CLOSURE, constant);
    emit_byte(compiler, (uint8_t)fn_compiler.upvalue_count);
    for (int i = 0; i < fn_compiler.upvalue_count; i++) {
        emit_byte(compiler, (uint8_t)(fn_compiler.upvalues[i].is_local ? 1 : 0));
        emit_byte(compiler, fn_compiler.upvalues[i].index);
    }

    uint8_t name_constant = identifier_constant(compiler, &name);
    compiler->previous = name;
    declare_variable(compiler);
    define_variable(compiler, name_constant);
}

static void class_declaration(Compiler* compiler) {
    consume(compiler, TOKEN_IDENTIFIER, "Expect class name");
    Token class_name = compiler->previous;
    uint8_t name_constant = identifier_constant(compiler, &class_name);
    declare_variable(compiler);

    emit_bytes(compiler, OP_CLASS, name_constant);

    if (match(compiler, TOKEN_EXTENDS)) {
        consume(compiler, TOKEN_IDENTIFIER, "Expect superclass name");
        uint8_t super_name = identifier_constant(compiler, &compiler->previous);

        emit_bytes(compiler, OP_GET_GLOBAL, super_name);
        emit_byte(compiler, OP_INHERIT);
    }

    if (compiler->current.type == TOKEN_NEWLINE) {
        advance(compiler);
    }
    if (compiler->current.type == TOKEN_INDENT) {
        advance(compiler);
    }

    while (compiler->current.type != TOKEN_DEDENT &&
           compiler->current.type != TOKEN_END &&
           compiler->current.type != TOKEN_EOF) {
        if (compiler->current.type == TOKEN_NEWLINE) {
            advance(compiler);
            continue;
        }
        if (compiler->current.type == TOKEN_FUNC) {
            advance(compiler);
            consume(compiler, TOKEN_IDENTIFIER, "Expect method name");
            Token method_name = compiler->previous;
            uint8_t method_constant = identifier_constant(compiler, &method_name);

            Compiler fn_compiler;
            Scanner saved_scanner;
            Token saved_current;
            Token saved_previous;

            begin_function_compile(compiler, &fn_compiler, method_name,
                                   method_name.start, true,
                                   &saved_scanner, &saved_current, &saved_previous);

            compile_function_body(compiler, &fn_compiler, method_name,
                                  method_name.start,
                                  &saved_scanner, &saved_current, &saved_previous);

            // If method is "init", implicitly return self
            if (method_name.length == 4 && memcmp(method_name.start, "init", 4) == 0) {
                emit_bytes(&fn_compiler, OP_GET_LOCAL, 1);
                emit_byte(&fn_compiler, OP_RETURN);
            } else {
                emit_return(&fn_compiler);
            }

            end_scope(&fn_compiler);

            ObjFunction* function = fn_compiler.function;

            compiler->had_error = fn_compiler.had_error;
            compiler->current = fn_compiler.current;
            compiler->previous = fn_compiler.previous;

            if (fn_compiler.had_error) return;

            uint8_t func_const = make_constant(compiler, OBJ_VAL(function));
            emit_bytes(compiler, OP_CONSTANT, func_const);
            emit_bytes(compiler, OP_METHOD, method_constant);
        } else {
            advance(compiler);
        }
    }

    // Consume class closing
    match(compiler, TOKEN_DEDENT);
    consume(compiler, TOKEN_END, "Expect 'end' after class body");
    match(compiler, TOKEN_NEWLINE);

    define_variable(compiler, name_constant);
}

static void return_statement(Compiler* compiler) {
    if (compiler->function->name == NULL) {
        error(compiler, "Can't return from top-level code");
    }
    if (compiler->current.type != TOKEN_NEWLINE &&
        compiler->current.type != TOKEN_DEDENT &&
        compiler->current.type != TOKEN_EOF) {
        expression(compiler);
        int count = 1;
        while (match(compiler, TOKEN_COMMA)) {
            expression(compiler);
            count++;
            if (count > 255) {
                error_at_current(compiler, "Too many return values");
            }
        }
        if (count > 1) {
            emit_bytes(compiler, OP_LIST, (uint8_t)count);
        }
    } else {
        emit_byte(compiler, OP_VOID);
    }
    emit_byte(compiler, OP_RETURN);
}

static void yield_statement(Compiler* compiler) {
    if (compiler->function->name == NULL) {
        error(compiler, "Can't yield from top-level code");
    }
    compiler->has_yield = true;
    if (compiler->current.type != TOKEN_NEWLINE &&
        compiler->current.type != TOKEN_DEDENT &&
        compiler->current.type != TOKEN_EOF) {
        expression(compiler);
    } else {
        emit_byte(compiler, OP_VOID);
    }
    emit_byte(compiler, OP_YIELD);
}

static void statement(Compiler* compiler) {
    if (match(compiler, TOKEN_SAY)) {
        print_statement(compiler);
    } else if (match(compiler, TOKEN_IF)) {
        if_statement(compiler);
    } else if (match(compiler, TOKEN_WHILE)) {
        while_statement(compiler);
    } else if (match(compiler, TOKEN_FOR)) {
        for_statement(compiler);
    } else if (match(compiler, TOKEN_RETURN)) {
        return_statement(compiler);
    } else if (match(compiler, TOKEN_YIELD)) {
        yield_statement(compiler);
    } else if (match(compiler, TOKEN_BREAK)) {
        if (loop_depth == 0) {
            error(compiler, "Can't use 'break' outside of a loop");
            return;
        }
        // Emit a jump to end of loop (will be patched)
        break_jumps[break_count++] = emit_jump(compiler, OP_JUMP);
    } else if (match(compiler, TOKEN_CONTINUE)) {
        if (loop_depth == 0) {
            error(compiler, "Can't use 'continue' outside of a loop");
            return;
        }
        // Emit a jump to loop start (will be patched)
        continue_jumps[continue_count++] = emit_jump(compiler, OP_JUMP);
    } else if (match(compiler, TOKEN_TRY)) {
        // Emit OP_TRY_SET_IP: push handler, offset to catch handler
        int handler_offset = emit_jump(compiler, OP_TRY_SET_IP);

        if (compiler->current.type == TOKEN_NEWLINE) {
            advance(compiler);
        }

        // Try body
        begin_scope(compiler);
        block(compiler);
        end_scope(compiler);

        // Normal path: pop exception handler, then skip catch block
        emit_byte(compiler, OP_POP_TRY);
        int skip_finally = emit_jump(compiler, OP_JUMP);

        // Catch handler start — OP_TRY_SET_IP jumps here on exception
        // (error value is on TOS)
        patch_jump(compiler, handler_offset);

        if (compiler->current.type == TOKEN_CATCH) {
            advance(compiler);
            begin_scope(compiler);
            if (compiler->current.type == TOKEN_IDENTIFIER) {
                Token err_var = compiler->current;
                advance(compiler);
                uint8_t arg = identifier_constant(compiler, &err_var);
                emit_bytes(compiler, OP_DEFINE_GLOBAL, arg);
            } else {
                emit_byte(compiler, OP_POP);
            }
            if (compiler->current.type == TOKEN_NEWLINE) {
                advance(compiler);
            }
            block(compiler);
            end_scope(compiler);
        } else if (compiler->current.type == TOKEN_FINALLY) {
            // No catch, but finally present: on the exception path we must
            // run the finally block then re-throw the original error.
            Scanner saved_scanner = *compiler->scanner;
            Token saved_token = compiler->current;

            static int err_count = 0;
            char err_buf[32];
            snprintf(err_buf, sizeof(err_buf), "_jts_err%d", err_count++);
            uint8_t c_err = make_constant(compiler,
                OBJ_VAL(copy_string(err_buf, (int)strlen(err_buf))));
            emit_bytes(compiler, OP_DEFINE_GLOBAL, c_err);

            *compiler->scanner = saved_scanner;
            compiler->current = saved_token;
            advance(compiler);
            if (compiler->current.type == TOKEN_NEWLINE) {
                advance(compiler);
            }
            begin_scope(compiler);
            block(compiler);
            end_scope(compiler);

            emit_bytes(compiler, OP_GET_GLOBAL, c_err);
            emit_byte(compiler, OP_THROW);

            *compiler->scanner = saved_scanner;
            compiler->current = saved_token;
        } else {
            // No catch and no finally: re-throw so the exception propagates.
            emit_byte(compiler, OP_THROW);
        }

        // Normal path lands here (skipping catch); finally runs on both paths
        patch_jump(compiler, skip_finally);

        if (match(compiler, TOKEN_FINALLY)) {
            if (compiler->current.type == TOKEN_NEWLINE) {
                advance(compiler);
            }
            begin_scope(compiler);
            block(compiler);
            end_scope(compiler);
        }

        match(compiler, TOKEN_END);
    } else if (match(compiler, TOKEN_THROW)) {
        expression(compiler);
        emit_byte(compiler, OP_THROW);
    } else if (match(compiler, TOKEN_DEL)) {
        del_statement(compiler);
    } else if (match(compiler, TOKEN_ASSERT)) {
        assert_statement(compiler);
    } else if (match(compiler, TOKEN_ASK)) {
        input_expression(compiler, false);
        emit_byte(compiler, OP_POP);
    } else if (match(compiler, TOKEN_FUNC)) {
        func_definition(compiler);
    } else if (match(compiler, TOKEN_CLASS)) {
        class_declaration(compiler);
    } else {
        expression_statement(compiler);
    }
}

static void expression_statement(Compiler* compiler) {
    expression(compiler);
    if (compiler->assign_created_local) {
        compiler->assign_created_local = false;
    } else {
        emit_byte(compiler, OP_POP);
    }
}

static void del_statement(Compiler* compiler) {
    if (compiler->current.type == TOKEN_IDENTIFIER) {
        Token name = compiler->current;
        advance(compiler);
        if (match(compiler, TOKEN_LEFT_BRACKET)) {
            uint8_t get_op;
            int arg = resolve_local(compiler, &name);
            if (arg != -1) {
                get_op = OP_GET_LOCAL;
            } else if ((arg = resolve_upvalue(compiler, &name)) != -1) {
                get_op = OP_GET_UPVALUE;
            } else {
                arg = identifier_constant(compiler, &name);
                get_op = OP_GET_GLOBAL;
            }
            emit_bytes(compiler, get_op, (uint8_t)arg);
            expression(compiler);
            consume(compiler, TOKEN_RIGHT_BRACKET, "Expect ']' after index");
            emit_byte(compiler, OP_DEL_INDEX);
        } else {
            int arg = resolve_local(compiler, &name);
            if (arg != -1) {
                emit_byte(compiler, OP_VOID);
                emit_bytes(compiler, OP_SET_LOCAL, (uint8_t)arg);
            } else if ((arg = resolve_upvalue(compiler, &name)) != -1) {
                emit_byte(compiler, OP_VOID);
                emit_bytes(compiler, OP_SET_UPVALUE, (uint8_t)arg);
            } else {
                uint8_t c = identifier_constant(compiler, &name);
                emit_bytes(compiler, OP_DEL_GLOBAL, c);
            }
        }
        return;
    }
    error(compiler, "Expect variable name after 'del'");
}

static void assert_statement(Compiler* compiler) {
    expression(compiler);
    if (match(compiler, TOKEN_COMMA)) {
        expression(compiler);
        emit_byte(compiler, OP_ASSERT);
    } else {
        emit_byte(compiler, OP_VOID);
        emit_byte(compiler, OP_ASSERT);
    }
}

static void declaration(Compiler* compiler) {
    if (compiler->current.type == TOKEN_NEWLINE) {
        advance(compiler);
        return;
    }
    if (compiler->current.type == TOKEN_IMPORT) {
        advance(compiler);
        consume(compiler, TOKEN_STRING, "Expect filename string after 'import'");
        // Compile as: load and execute the imported file at runtime
        // Emit the native callee first (must be on top of stack for OP_CALL)
        emit_constant(compiler, OBJ_VAL(copy_string("import_file", 11)));
        emit_constant(compiler, OBJ_VAL(copy_string(
            compiler->previous.start, compiler->previous.length)));
        emit_bytes(compiler, OP_CALL, 1);
        emit_byte(compiler, OP_POP);
        return;
    }
    if (compiler->current.type == TOKEN_BRING) {
        advance(compiler);
        // Emit native callee first (must be on top of stack for OP_CALL),
        // then the scroll name argument.
        emit_constant(compiler, OBJ_VAL(copy_string("bring_scroll", 12)));
        if (compiler->current.type == TOKEN_STRING) {
            // bring "path/name" — direct path form
            emit_constant(compiler, OBJ_VAL(copy_string(
                compiler->current.start, compiler->current.length)));
            advance(compiler);
        } else {
            // bring A.B.C — dotted scroll name
            char buf[512];
            int len = 0;
            consume(compiler, TOKEN_IDENTIFIER, "Expect scroll name after 'bring'");
            memcpy(buf, compiler->previous.start, compiler->previous.length);
            len = compiler->previous.length;
            while (match(compiler, TOKEN_DOT)) {
                consume(compiler, TOKEN_IDENTIFIER, "Expect scroll name after '.'");
                buf[len++] = '.';
                memcpy(buf + len, compiler->previous.start, compiler->previous.length);
                len += compiler->previous.length;
            }
            buf[len] = '\0';
            emit_constant(compiler, OBJ_VAL(copy_string(buf, len)));
        }
        emit_bytes(compiler, OP_CALL, 1);
        emit_byte(compiler, OP_POP);
        return;
    }
    // Handle type-annotated variable declarations
    if (compiler->current.type == TOKEN_INT ||
        compiler->current.type == TOKEN_FLOAT ||
        compiler->current.type == TOKEN_STRING_KW ||
        compiler->current.type == TOKEN_BOOL_KW ||
        compiler->current.type == TOKEN_LIST_KW ||
        compiler->current.type == TOKEN_VAR) {
        Scanner saved = *compiler->scanner;
        Token peek = scan_token(compiler->scanner);
        *compiler->scanner = saved;
        if (peek.type != TOKEN_LEFT_PAREN) {
            advance(compiler); // consume the type keyword
            Token name = compiler->current;
            consume(compiler, TOKEN_IDENTIFIER, "Expect variable name after type");

            if (match(compiler, TOKEN_EQUAL)) {
                // Type-annotated with value: int x = 10
                expression(compiler);
            } else {
                // Unassigned: int x (default to nil)
                emit_byte(compiler, OP_VOID);
            }

            uint8_t arg = identifier_constant(compiler, &name);
            emit_bytes(compiler, OP_DEFINE_GLOBAL, arg);
            return;
        }
    }
    statement(compiler);
}

static void synchronize(Compiler* compiler) {
    compiler->panic_mode = false;
    while (compiler->current.type != TOKEN_EOF) {
        if (compiler->previous.type == TOKEN_NEWLINE) return;
        switch (compiler->current.type) {
            case TOKEN_FUNC:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_FOR:
            case TOKEN_SAY:
            case TOKEN_RETURN:
            case TOKEN_CLASS:
                return;
            default: ;
        }
        advance(compiler);
    }
}

/* Expression parsing - Pratt parser */

static void binary(Compiler* compiler, bool can_assign) {
    TokenType operator_type = compiler->previous.type;
    ParseRule* rule = get_rule(operator_type);

    if (operator_type == TOKEN_AND) {
        int right_jump = emit_jump(compiler, OP_JUMP_IF_FALSE);
        emit_byte(compiler, OP_POP);
        parse_precedence(compiler, PREC_AND);
        patch_jump(compiler, right_jump);
        return;
    }
    if (operator_type == TOKEN_OR) {
        int else_jump = emit_jump(compiler, OP_JUMP_IF_FALSE);
        int end_jump = emit_jump(compiler, OP_JUMP);
        patch_jump(compiler, else_jump);
        emit_byte(compiler, OP_POP);
        parse_precedence(compiler, PREC_OR);
        patch_jump(compiler, end_jump);
        return;
    }

    parse_precedence(compiler, (Precedence)(rule->precedence + 1));

    switch (operator_type) {
        case TOKEN_PLUS:          emit_byte(compiler, OP_ADD); break;
        case TOKEN_MINUS:         emit_byte(compiler, OP_SUBTRACT); break;
        case TOKEN_STAR:          emit_byte(compiler, OP_MULTIPLY); break;
        case TOKEN_SLASH:         emit_byte(compiler, OP_DIVIDE); break;
        case TOKEN_PERCENT:       emit_byte(compiler, OP_MODULO); break;
        case TOKEN_STAR_STAR:     emit_byte(compiler, OP_POWER); break;
        case TOKEN_SLASH_SLASH:   emit_byte(compiler, OP_FLOOR_DIV); break;
        case TOKEN_AMPERSAND:     emit_byte(compiler, OP_BIT_AND); break;
        case TOKEN_BAR:           emit_byte(compiler, OP_BIT_OR); break;
        case TOKEN_CARET:         emit_byte(compiler, OP_BIT_XOR); break;
        case TOKEN_LESS_LESS:     emit_byte(compiler, OP_SHIFT_LEFT); break;
        case TOKEN_GREATER_GREATER: emit_byte(compiler, OP_SHIFT_RIGHT); break;
        case TOKEN_EQUAL_EQUAL:   emit_byte(compiler, OP_EQUAL); break;
        case TOKEN_BANG_EQUAL:    emit_byte(compiler, OP_NOT_EQUAL); break;
        case TOKEN_IS:            emit_byte(compiler, OP_IS); break;
        case TOKEN_GREATER:       emit_byte(compiler, OP_GREATER); break;
        case TOKEN_LESS:          emit_byte(compiler, OP_LESS); break;
        case TOKEN_GREATER_EQUAL: emit_byte(compiler, OP_GREATER_EQUAL); break;
        case TOKEN_LESS_EQUAL:    emit_byte(compiler, OP_LESS_EQUAL); break;
        case TOKEN_OF:            emit_byte(compiler, OP_OF); break;
        default: return;
    }
}

static void ternary_expr(Compiler* compiler, bool can_assign) {
    int then_jump = emit_jump(compiler, OP_JUMP_IF_FALSE);
    emit_byte(compiler, OP_POP);
    expression(compiler);
    consume(compiler, TOKEN_COLON, "Expect ':' in ternary expression");
    int else_jump = emit_jump(compiler, OP_JUMP);
    patch_jump(compiler, then_jump);
    emit_byte(compiler, OP_POP);
    expression(compiler);
    patch_jump(compiler, else_jump);
}

static void literal(Compiler* compiler, bool can_assign) {
    switch (compiler->previous.type) {
        case TOKEN_FALSE: emit_byte(compiler, OP_FALSE); break;
        case TOKEN_VOID:   emit_byte(compiler, OP_VOID); break;
        case TOKEN_TRUE:  emit_byte(compiler, OP_TRUE); break;
        default: return;
    }
}

static void grouping(Compiler* compiler, bool can_assign) {
    expression(compiler);
    if (match(compiler, TOKEN_COMMA)) {
        int count = 2;
        expression(compiler);
        while (match(compiler, TOKEN_COMMA)) {
            expression(compiler);
            count++;
            if (count > 255) {
                error_at_current(compiler, "Too many tuple elements");
            }
        }
        consume(compiler, TOKEN_RIGHT_PAREN, "Expect ')' after tuple");
        emit_bytes(compiler, OP_LIST, (uint8_t)count);
        return;
    }
    consume(compiler, TOKEN_RIGHT_PAREN, "Expect ')' after expression");
}

static void lambda_expression_impl(Compiler* compiler, bool can_assign) {
    // Anonymous function (lambda) expression. Consumes the 'func' token and parses a function literal.
    // Syntax: func (params) ... end
    // No name, not defined as global.
    Compiler fn_compiler;
    Scanner saved_scanner;
    Token saved_current;
    Token saved_previous;

    begin_function_compile(compiler, &fn_compiler, (Token){0}, "<lambda>", true,
                           &saved_scanner, &saved_current, &saved_previous);

    compile_function_body(compiler, &fn_compiler, (Token){0}, "<lambda>",
                          &saved_scanner, &saved_current, &saved_previous);

    emit_return(&fn_compiler);

    end_scope(&fn_compiler);
    ObjFunction* function = fn_compiler.function;
    function->upvalue_count = fn_compiler.upvalue_count;

    // Propagate updates
    compiler->had_error = fn_compiler.had_error;
    compiler->current = fn_compiler.current;
    compiler->previous = fn_compiler.previous;

    if (fn_compiler.had_error) return;

    // Emit closure; leaves it on stack as a value.
    uint8_t constant = make_constant(compiler, OBJ_VAL(function));
    emit_bytes(compiler, OP_CLOSURE, constant);
    emit_byte(compiler, (uint8_t)fn_compiler.upvalue_count);
    for (int i = 0; i < fn_compiler.upvalue_count; i++) {
        emit_byte(compiler, (uint8_t)(fn_compiler.upvalues[i].is_local ? 1 : 0));
        emit_byte(compiler, fn_compiler.upvalues[i].index);
    }
    // No define_variable – closure stays on stack as a value.
}

static void number_literal_impl(Compiler* compiler, bool can_assign) {
    double value = strtod(compiler->previous.start, NULL);
emit_constant(compiler, NUMBER_VAL(value));
}
#if 0
static void lambda_expression_impl(Compiler* compiler, bool can_assign) {
    // Anonymous function (lambda) expression. Consumes the 'func' token and parses a function literal.
    // Syntax: func (params) ... end
    // No name, not defined as global.
    Compiler fn_compiler;
    fn_compiler.parent = compiler;
    fn_compiler.scanner = compiler->scanner;
    fn_compiler.had_error = false;
    fn_compiler.panic_mode = false;
    fn_compiler.has_yield = false;
    fn_compiler.assign_created_local = false;
    fn_compiler.function_type = TYPE_FUNCTION;
    fn_compiler.local_count = 0;
    fn_compiler.scope_depth = 0;
    fn_compiler.upvalue_count = 0;
    fn_compiler.current = compiler->current;
    fn_compiler.previous = compiler->previous;

    fn_compiler.function = new_function();
    fn_compiler.function->name = NULL; // anonymous

    Chunk* chunk = current_chunk(&fn_compiler);
    fn_compiler.debug_func = chunk_add_debug_func(chunk, "<lambda>", 8, 0);

    // Slot 0 placeholder for the function itself.
    Local* local = &fn_compiler.locals[fn_compiler.local_count++];
    local->depth = 0;
    local->name.start = "";
    local->name.length = 0;
    local->is_captured = false;

    begin_scope(&fn_compiler);

    // Expect '(' after 'func'
    consume(&fn_compiler, TOKEN_LEFT_PAREN, "Expect '(' after 'func' for lambda parameters");
    parse_parameters(&fn_compiler);
    if (fn_compiler.debug_func) {
        fn_compiler.debug_func->arity = fn_compiler.function->arity;
    }

    // Optional newline after parameters
    if (fn_compiler.current.type == TOKEN_NEWLINE) {
        advance(&fn_compiler);
    }

    block(&fn_compiler);
    match(&fn_compiler, TOKEN_END);

    emit_return(&fn_compiler);

    end_scope(&fn_compiler);
    ObjFunction* function = fn_compiler.function;
    function->upvalue_count = fn_compiler.upvalue_count;

    // Propagate updates
    compiler->had_error = fn_compiler.had_error;
    compiler->current = fn_compiler.current;
    compiler->previous = fn_compiler.previous;

    if (fn_compiler.had_error) return;

    // Emit closure; leaves it on stack as a value.
    uint8_t constant = make_constant(compiler, OBJ_VAL(function));
    emit_bytes(compiler, OP_CLOSURE, constant);
    emit_byte(compiler, (uint8_t)fn_compiler.upvalue_count);
    for (int i = 0; i < fn_compiler.upvalue_count; i++) {
        emit_byte(compiler, (uint8_t)(fn_compiler.upvalues[i].is_local ? 1 : 0));
        emit_byte(compiler, fn_compiler.upvalues[i].index);
    }
    // No define_variable – closure stays on stack as a value.
}

static void number_literal_impl(Compiler* compiler, bool can_assign) {
    double value = strtod(compiler->previous.start, NULL);
    emit_constant(compiler, NUMBER_VAL(value));
}
#endif

static void lambda_expression(Compiler* compiler, bool can_assign) {
    lambda_expression_impl(compiler, can_assign);
}
static void number_literal(Compiler* compiler, bool can_assign) {
    number_literal_impl(compiler, can_assign);
}
static ObjString* unescape_string(const char* src, int length) {
    char* buf = ALLOCATE(char, length + 1);
    int pos = 0;
    for (int i = 0; i < length; i++) {
        char c = src[i];
        if (c == '\\' && i + 1 < length) {
            char e = src[++i];
            switch (e) {
                case 'n': buf[pos++] = '\n'; break;
                case 't': buf[pos++] = '\t'; break;
                case 'r': buf[pos++] = '\r'; break;
                case '0': buf[pos++] = '\0'; break;
                case 'a': buf[pos++] = '\a'; break;
                case 'b': buf[pos++] = '\b'; break;
                case 'f': buf[pos++] = '\f'; break;
                case 'v': buf[pos++] = '\v'; break;
                case '\\': buf[pos++] = '\\'; break;
                case '"': buf[pos++] = '"'; break;
                case '\'': buf[pos++] = '\''; break;
                case '\n': break;
                default: buf[pos++] = '\\'; buf[pos++] = e; break;
            }
        } else {
            buf[pos++] = c;
        }
    }
    buf[pos] = '\0';
    ObjString* string = take_string(buf, pos);
    return string;
}

static void string_literal(Compiler* compiler, bool can_assign) {
    ObjString* string = unescape_string(compiler->previous.start,
                                        compiler->previous.length);
    emit_constant(compiler, OBJ_VAL(string));
}

static void unary(Compiler* compiler, bool can_assign) {
    TokenType operator_type = compiler->previous.type;
    parse_precedence(compiler, PREC_UNARY);

    switch (operator_type) {
        case TOKEN_MINUS: emit_byte(compiler, OP_NEGATE); break;
        case TOKEN_NOT:   emit_byte(compiler, OP_NOT); break;
        case TOKEN_TILDE: emit_byte(compiler, OP_BIT_NOT); break;
        default: return;
    }
}

static void named_variable(Compiler* compiler, Token name, bool can_assign) {
    uint8_t get_op, set_op;
    int arg = resolve_local(compiler, &name);
    bool created_local = false;
    if (arg != -1) {
        get_op = OP_GET_LOCAL;
        set_op = OP_SET_LOCAL;
    } else if ((arg = resolve_upvalue(compiler, &name)) != -1) {
        get_op = OP_GET_UPVALUE;
        set_op = OP_SET_UPVALUE;
    } else if (can_assign && compiler->function_type == TYPE_FUNCTION &&
               compiler->scope_depth > 0 &&
               compiler->current.type == TOKEN_EQUAL) {
        arg = compiler->local_count;
        add_local(compiler, name);
        compiler->locals[compiler->local_count - 1].depth = compiler->scope_depth;
        compiler->assign_created_local = true;
        created_local = true;
        get_op = OP_GET_LOCAL;
        set_op = OP_SET_LOCAL;
        if (compiler->detect_hoists && loop_depth > 0) {
            int i;
            for (i = 0; i < compiler->hoist_count; i++) {
                if (identifiers_equal(&compiler->hoist_names[i], &name)) break;
            }
            if (i == compiler->hoist_count &&
                compiler->hoist_count < MAX_LOCALS) {
                compiler->hoist_names[compiler->hoist_count++] = name;
            }
        }
    } else {
        arg = identifier_constant(compiler, &name);
        get_op = OP_GET_GLOBAL;
        set_op = OP_SET_GLOBAL;
    }

    if (can_assign && match(compiler, TOKEN_EQUAL)) {
        expression(compiler);
        if (!created_local) {
            emit_bytes(compiler, set_op, (uint8_t)arg);
        }
    } else if (can_assign && (compiler->current.type == TOKEN_PLUS_EQUAL ||
                              compiler->current.type == TOKEN_MINUS_EQUAL ||
                              compiler->current.type == TOKEN_STAR_EQUAL ||
                              compiler->current.type == TOKEN_SLASH_EQUAL ||
                              compiler->current.type == TOKEN_PERCENT_EQUAL ||
                              compiler->current.type == TOKEN_STAR_STAR_EQUAL ||
                              compiler->current.type == TOKEN_SLASH_SLASH_EQUAL ||
                              compiler->current.type == TOKEN_AMPERSAND_EQUAL ||
                              compiler->current.type == TOKEN_BAR_EQUAL ||
                              compiler->current.type == TOKEN_CARET_EQUAL ||
                              compiler->current.type == TOKEN_LESS_LESS_EQUAL ||
                              compiler->current.type == TOKEN_GREATER_GREATER_EQUAL)) {
        TokenType op = compiler->current.type;
        advance(compiler);
        emit_bytes(compiler, get_op, (uint8_t)arg);
        expression(compiler);
        switch (op) {
            case TOKEN_PLUS_EQUAL:      emit_byte(compiler, OP_ADD); break;
            case TOKEN_MINUS_EQUAL:     emit_byte(compiler, OP_SUBTRACT); break;
            case TOKEN_STAR_EQUAL:      emit_byte(compiler, OP_MULTIPLY); break;
            case TOKEN_SLASH_EQUAL:     emit_byte(compiler, OP_DIVIDE); break;
            case TOKEN_PERCENT_EQUAL:   emit_byte(compiler, OP_MODULO); break;
            case TOKEN_STAR_STAR_EQUAL: emit_byte(compiler, OP_POWER); break;
            case TOKEN_SLASH_SLASH_EQUAL: emit_byte(compiler, OP_FLOOR_DIV); break;
            case TOKEN_AMPERSAND_EQUAL: emit_byte(compiler, OP_BIT_AND); break;
            case TOKEN_BAR_EQUAL:       emit_byte(compiler, OP_BIT_OR); break;
            case TOKEN_CARET_EQUAL:     emit_byte(compiler, OP_BIT_XOR); break;
            case TOKEN_LESS_LESS_EQUAL: emit_byte(compiler, OP_SHIFT_LEFT); break;
            case TOKEN_GREATER_GREATER_EQUAL: emit_byte(compiler, OP_SHIFT_RIGHT); break;
            default: break;
        }
        emit_bytes(compiler, set_op, (uint8_t)arg);
    } else {
        emit_bytes(compiler, get_op, (uint8_t)arg);
    }
}

static bool lookahead_is_multi_assign(Compiler* compiler) {
    if (compiler->current.type != TOKEN_COMMA) return false;
    Scanner saved = *compiler->scanner;
    Token t = scan_token(compiler->scanner);
    if (t.type != TOKEN_IDENTIFIER && t.type != TOKEN_SELF) {
        *compiler->scanner = saved;
        return false;
    }
    t = scan_token(compiler->scanner);
    while (t.type == TOKEN_COMMA) {
        t = scan_token(compiler->scanner);
        if (t.type != TOKEN_IDENTIFIER && t.type != TOKEN_SELF) {
            *compiler->scanner = saved;
            return false;
        }
        t = scan_token(compiler->scanner);
    }
    bool is_assign = (t.type == TOKEN_EQUAL);
    *compiler->scanner = saved;
    return is_assign;
}

static void variable(Compiler* compiler, bool can_assign) {    if (can_assign && lookahead_is_multi_assign(compiler)) {
        Token targets[256];
        int n = 0;
        targets[n++] = compiler->previous;
        while (match(compiler, TOKEN_COMMA)) {
            if (compiler->current.type == TOKEN_IDENTIFIER ||
                compiler->current.type == TOKEN_SELF) {
                advance(compiler);
            } else {
                consume(compiler, TOKEN_IDENTIFIER, "Expect variable name");
            }
            targets[n++] = compiler->previous;
        }
        consume(compiler, TOKEN_EQUAL, "Expect '=' after assignment targets");
        expression(compiler);
        int rcount = 1;
        while (match(compiler, TOKEN_COMMA)) {
            expression(compiler);
            rcount++;
        }
        if (rcount > 1) {
            emit_bytes(compiler, OP_LIST, (uint8_t)rcount);
        }
        emit_bytes(compiler, OP_UNPACK, (uint8_t)n);
        for (int i = n - 1; i >= 0; i--) {
            uint8_t set_op;
            int arg = resolve_local(compiler, &targets[i]);
            if (arg != -1) {
                set_op = OP_SET_LOCAL;
            } else {
                arg = identifier_constant(compiler, &targets[i]);
                set_op = OP_SET_GLOBAL;
            }
            emit_bytes(compiler, set_op, (uint8_t)arg);
            emit_byte(compiler, OP_POP);
        }
        emit_byte(compiler, OP_VOID);
        return;
    }
    named_variable(compiler, compiler->previous, can_assign);
}

static void keyword_variable(Compiler* compiler, bool can_assign) {
    Token name = compiler->previous;
    name.type = TOKEN_IDENTIFIER;
    named_variable(compiler, name, can_assign);
}

static void len_expr(Compiler* compiler, bool can_assign) {
    emit_constant(compiler, OBJ_VAL(copy_string("len", 3)));
    consume(compiler, TOKEN_LEFT_PAREN, "Expect '(' after 'len'");
    expression(compiler);
    consume(compiler, TOKEN_RIGHT_PAREN, "Expect ')' after len argument");
    emit_bytes(compiler, OP_CALL, 1);
}

static void append_expr(Compiler* compiler, bool can_assign) {
    emit_constant(compiler, OBJ_VAL(copy_string("append", 6)));
    consume(compiler, TOKEN_LEFT_PAREN, "Expect '(' after 'append'");
    int arg_count = 0;
    if (compiler->current.type != TOKEN_RIGHT_PAREN) {
        do {
            expression(compiler);
            arg_count++;
        } while (match(compiler, TOKEN_COMMA));
    }
    consume(compiler, TOKEN_RIGHT_PAREN, "Expect ')' after append arguments");
    emit_bytes(compiler, OP_CALL, (uint8_t)arg_count);
}

static void number_expr(Compiler* compiler, bool can_assign) {
    emit_constant(compiler, OBJ_VAL(copy_string("number", 6)));
    consume(compiler, TOKEN_LEFT_PAREN, "Expect '(' after 'number'");
    expression(compiler);
    consume(compiler, TOKEN_RIGHT_PAREN, "Expect ')' after number argument");
    emit_bytes(compiler, OP_CALL, 1);
}

static void type_expr(Compiler* compiler, bool can_assign) {
    emit_constant(compiler, OBJ_VAL(copy_string("type", 4)));
    consume(compiler, TOKEN_LEFT_PAREN, "Expect '(' after 'type'");
    expression(compiler);
    consume(compiler, TOKEN_RIGHT_PAREN, "Expect ')' after type argument");
    emit_bytes(compiler, OP_CALL, 1);
}

static void set_expr(Compiler* compiler, bool can_assign) {
    emit_constant(compiler, OBJ_VAL(copy_string("set", 3)));
    consume(compiler, TOKEN_LEFT_PAREN, "Expect '(' after 'set'");
    expression(compiler);
    consume(compiler, TOKEN_RIGHT_PAREN, "Expect ')' after set argument");
    emit_bytes(compiler, OP_CALL, 1);
}

typedef struct {
    Token var;
    Scanner iter_start;
    Scanner cond_start;
    Scanner end_pos;
    bool has_if;
} CompInfo;

static bool scan_comprehension(Compiler* compiler, CompInfo* info) {
    Scanner scan = *compiler->scanner;
    scan.current = compiler->current.start;
    scan.start = compiler->current.start;
    scan.has_pending = false;
    int depth = 0;

    for (;;) {
        Token t = scan_token(&scan);
        if (t.type == TOKEN_EOF) return false;
        if (t.type == TOKEN_LEFT_PAREN || t.type == TOKEN_LEFT_BRACKET ||
            t.type == TOKEN_LEFT_BRACE) { depth++; continue; }
        if (t.type == TOKEN_RIGHT_PAREN || t.type == TOKEN_RIGHT_BRACE) {
            if (depth > 0) depth--;
            continue;
        }
        if (t.type == TOKEN_RIGHT_BRACKET) {
            if (depth == 0) return false;
            depth--;
            continue;
        }
        if (t.type == TOKEN_FOR && depth == 0) break;
    }

    Token var = scan_token(&scan);
    if (var.type != TOKEN_IDENTIFIER) return false;
    Token in = scan_token(&scan);
    if (in.type != TOKEN_OF) return false;
    info->var = var;
    info->iter_start = scan;

    for (;;) {
        Scanner before = scan;
        Token u = scan_token(&scan);
        if (u.type == TOKEN_EOF) return false;
        if (u.type == TOKEN_LEFT_PAREN || u.type == TOKEN_LEFT_BRACKET ||
            u.type == TOKEN_LEFT_BRACE) { depth++; continue; }
        if (u.type == TOKEN_RIGHT_PAREN || u.type == TOKEN_RIGHT_BRACE) {
            if (depth > 0) depth--;
            continue;
        }
        if (u.type == TOKEN_RIGHT_BRACKET) {
            if (depth == 0) {
                info->has_if = false;
                info->end_pos = before;
                return true;
            }
            depth--;
            continue;
        }
        if (u.type == TOKEN_IF && depth == 0) {
            info->has_if = true;
            info->cond_start = scan;
            break;
        }
    }

    for (;;) {
        Scanner before = scan;
        Token u = scan_token(&scan);
        if (u.type == TOKEN_EOF) return false;
        if (u.type == TOKEN_LEFT_PAREN || u.type == TOKEN_LEFT_BRACKET ||
            u.type == TOKEN_LEFT_BRACE) { depth++; continue; }
        if (u.type == TOKEN_RIGHT_PAREN || u.type == TOKEN_RIGHT_BRACE) {
            if (depth > 0) depth--;
            continue;
        }
        if (u.type == TOKEN_RIGHT_BRACKET) {
            if (depth == 0) {
                info->end_pos = before;
                return true;
            }
            depth--;
            continue;
        }
    }
}

static bool scan_comprehension_probe(Compiler* compiler) {
    CompInfo info;
    return scan_comprehension(compiler, &info);
}

static void list_comprehension(Compiler* compiler) {
    CompInfo info;

    if (!scan_comprehension(compiler, &info)) {
        error_at_current(compiler, "Invalid list comprehension");
        return;
    }

    Scanner expr_scanner = *compiler->scanner;
    expr_scanner.current = compiler->current.start;
    expr_scanner.start = compiler->current.start;
    expr_scanner.has_pending = false;

    static int comp_count = 0;
    int my_comp = comp_count++;
    char comp_buf[32], iter_buf[32], idx_buf[32];
    snprintf(comp_buf, sizeof(comp_buf), "_jts_comp%d", my_comp);
    snprintf(iter_buf, sizeof(iter_buf), "_jts_iter%d", my_comp);
    snprintf(idx_buf, sizeof(idx_buf), "_jts_idx%d", my_comp);

    uint8_t c_comp = make_constant(compiler, OBJ_VAL(copy_string(comp_buf, (int)strlen(comp_buf))));
    uint8_t c_iter = make_constant(compiler, OBJ_VAL(copy_string(iter_buf, (int)strlen(iter_buf))));
    uint8_t c_idx = make_constant(compiler, OBJ_VAL(copy_string(idx_buf, (int)strlen(idx_buf))));
    uint8_t c_var = make_constant(compiler, OBJ_VAL(copy_string(info.var.start, info.var.length)));

    emit_bytes(compiler, OP_LIST, 0);
    emit_bytes(compiler, OP_DEFINE_GLOBAL, c_comp);

    *compiler->scanner = info.iter_start;
    compiler->current = scan_token(compiler->scanner);
    expression(compiler);
    emit_bytes(compiler, OP_DEFINE_GLOBAL, c_iter);

    emit_constant(compiler, NUMBER_VAL(0));
    emit_bytes(compiler, OP_DEFINE_GLOBAL, c_idx);

    int loop_start = current_chunk(compiler)->count;

    emit_bytes(compiler, OP_GET_GLOBAL, c_idx);
    emit_bytes(compiler, OP_GET_GLOBAL, c_iter);
    emit_byte(compiler, OP_LEN);
    emit_byte(compiler, OP_LESS);
    int exit_jump = emit_jump(compiler, OP_JUMP_IF_FALSE);
    emit_byte(compiler, OP_POP);

    emit_bytes(compiler, OP_GET_GLOBAL, c_iter);
    emit_bytes(compiler, OP_GET_GLOBAL, c_idx);
    emit_byte(compiler, OP_ITER_VALUE);
    emit_bytes(compiler, OP_DEFINE_GLOBAL, c_var);

    int skip_append = -1;
    int skip_incr = -1;
    if (info.has_if) {
        *compiler->scanner = info.cond_start;
        compiler->current = scan_token(compiler->scanner);
        expression(compiler);
        skip_append = emit_jump(compiler, OP_JUMP_IF_FALSE);
        emit_byte(compiler, OP_POP);
    }

    *compiler->scanner = expr_scanner;
    compiler->current = scan_token(compiler->scanner);
    expression(compiler);

    emit_bytes(compiler, OP_GET_GLOBAL, c_comp);
    emit_byte(compiler, OP_SWAP);
    emit_byte(compiler, OP_APPEND_LIST);
    emit_byte(compiler, OP_POP);

    if (skip_append != -1) {
        skip_incr = emit_jump(compiler, OP_JUMP);
        patch_jump(compiler, skip_append);
        emit_byte(compiler, OP_POP);
        patch_jump(compiler, skip_incr);
    }

    emit_bytes(compiler, OP_GET_GLOBAL, c_idx);
    emit_constant(compiler, NUMBER_VAL(1));
    emit_byte(compiler, OP_ADD);
    emit_bytes(compiler, OP_DEFINE_GLOBAL, c_idx);

    emit_loop(compiler, loop_start);

    patch_jump(compiler, exit_jump);
    emit_byte(compiler, OP_POP);

    emit_bytes(compiler, OP_GET_GLOBAL, c_comp);

    *compiler->scanner = info.end_pos;
    compiler->current = scan_token(compiler->scanner);
    consume(compiler, TOKEN_RIGHT_BRACKET, "Expect ']' after list comprehension");
}

static void list_literal(Compiler* compiler, bool can_assign) {
    if (compiler->current.type != TOKEN_RIGHT_BRACKET &&
        scan_comprehension_probe(compiler)) {
        list_comprehension(compiler);
        return;
    }
    int count = 0;
    if (compiler->current.type != TOKEN_RIGHT_BRACKET) {
        do {
            expression(compiler);
            count++;
            if (count > 255) {
                error_at_current(compiler, "Too many list elements");
            }
        } while (match(compiler, TOKEN_COMMA));
    }
    consume(compiler, TOKEN_RIGHT_BRACKET, "Expect ']' after list elements");
    emit_bytes(compiler, OP_LIST, (uint8_t)count);
}

static void dict_literal(Compiler* compiler, bool can_assign) {
    int count = 0;
    if (compiler->current.type != TOKEN_RIGHT_BRACE) {
        do {
            if (compiler->current.type == TOKEN_STRING) {
                advance(compiler);
                ObjString* key = unescape_string(compiler->previous.start, compiler->previous.length);
                emit_constant(compiler, OBJ_VAL(key));
            } else if (compiler->current.type == TOKEN_IDENTIFIER) {
                advance(compiler);
                ObjString* key = copy_string(compiler->previous.start, compiler->previous.length);
                emit_constant(compiler, OBJ_VAL(key));
            } else {
                error_at_current(compiler, "Expect string or identifier as dict key");
                return;
            }
            consume(compiler, TOKEN_COLON, "Expect ':' after dict key");
            expression(compiler);
            count++;
        } while (match(compiler, TOKEN_COMMA));
    }
    consume(compiler, TOKEN_RIGHT_BRACE, "Expect '}' after dict elements");
    emit_constant(compiler, NUMBER_VAL(count));
    emit_byte(compiler, OP_DICT);
}

static void set_literal(Compiler* compiler, bool can_assign) {
    int count = 0;
    if (compiler->current.type != TOKEN_RIGHT_BRACE) {
        do {
            expression(compiler);
            count++;
            if (count > 255) {
                error_at_current(compiler, "Too many set elements");
            }
        } while (match(compiler, TOKEN_COMMA));
    }
    consume(compiler, TOKEN_RIGHT_BRACE, "Expect '}' after set elements");
    emit_bytes(compiler, OP_SET_LITERAL, (uint8_t)count);
}

static bool brace_is_set_literal(Compiler* compiler) {
    // {a: b, c: d} is a dict; {1, 2, 3} is a set. Empty {} is a dict.
    Scanner saved = *compiler->scanner;
    Token t = scan_token(compiler->scanner);
    *compiler->scanner = saved;
    if (t.type == TOKEN_RIGHT_BRACE) return false;
    int depth = 0;
    for (;;) {
        Scanner before = *compiler->scanner;
        Token u = scan_token(compiler->scanner);
        if (u.type == TOKEN_EOF || u.type == TOKEN_ERROR) {
            *compiler->scanner = saved;
            return false;
        }
        if (u.type == TOKEN_LEFT_PAREN || u.type == TOKEN_LEFT_BRACKET ||
            u.type == TOKEN_LEFT_BRACE) { depth++; continue; }
        if (u.type == TOKEN_RIGHT_PAREN || u.type == TOKEN_RIGHT_BRACKET ||
            u.type == TOKEN_RIGHT_BRACE) {
            if (depth == 0) break;
            depth--;
            continue;
        }
        if (depth == 0 && u.type == TOKEN_COLON) {
            *compiler->scanner = saved;
            return false;
        }
        if (depth == 0 && u.type == TOKEN_NEWLINE) {
            *compiler->scanner = saved;
            return false;
        }
    }
    *compiler->scanner = saved;
    return true;
}

static void brace_literal(Compiler* compiler, bool can_assign) {
    if (brace_is_set_literal(compiler)) {
        set_literal(compiler, can_assign);
    } else {
        dict_literal(compiler, can_assign);
    }
}

static void index_expr(Compiler* compiler, bool can_assign) {
    if (compiler->current.type == TOKEN_COLON) {
        advance(compiler);
        emit_byte(compiler, OP_VOID);
        if (compiler->current.type == TOKEN_COLON || compiler->current.type == TOKEN_RIGHT_BRACKET) {
            emit_byte(compiler, OP_VOID);
        } else {
            expression(compiler);
        }
        if (match(compiler, TOKEN_COLON)) {
            if (compiler->current.type == TOKEN_RIGHT_BRACKET) {
                emit_byte(compiler, OP_VOID);
            } else {
                expression(compiler);
            }
        } else {
            emit_byte(compiler, OP_VOID);
        }
        consume(compiler, TOKEN_RIGHT_BRACKET, "Expect ']' after slice");
        emit_byte(compiler, OP_SLICE);
        return;
    }

    expression(compiler);

    if (match(compiler, TOKEN_COLON)) {
        if (compiler->current.type == TOKEN_COLON || compiler->current.type == TOKEN_RIGHT_BRACKET) {
            emit_byte(compiler, OP_VOID);
        } else {
            expression(compiler);
        }
        if (match(compiler, TOKEN_COLON)) {
            if (compiler->current.type == TOKEN_RIGHT_BRACKET) {
                emit_byte(compiler, OP_VOID);
            } else {
                expression(compiler);
            }
        } else {
            emit_byte(compiler, OP_VOID);
        }
        consume(compiler, TOKEN_RIGHT_BRACKET, "Expect ']' after slice");
        emit_byte(compiler, OP_SLICE);
        return;
    }

    consume(compiler, TOKEN_RIGHT_BRACKET, "Expect ']' after index");

    if (can_assign && match(compiler, TOKEN_EQUAL)) {
        expression(compiler);
        emit_byte(compiler, OP_INDEX_SET);
    } else {
        emit_byte(compiler, OP_INDEX);
    }
}

static void dot_expr(Compiler* compiler, bool can_assign) {
    if (compiler->current.type != TOKEN_IDENTIFIER &&
        compiler->current.type != TOKEN_APPEND) {
        error(compiler, "Expect property name after '.'");
        return;
    }
    advance(compiler);
    Token prop_name = compiler->previous;

    // Built-in method dispatch (strings, lists, dicts): s.method(args) -> method(s, args)
    static const char* method_names[] = {
        "upper", "lower", "trim", "split", "contains", "replace", "substring", "starts_with", "ends_with",
        "find", "count", "capitalize", "title", "swapcase",
        "is_digit", "is_alpha", "is_alnum", "is_space", "is_upper", "is_lower",
        "zfill", "ljust", "rjust", "center", "join", "lstrip", "rstrip", "splitlines", "format",
        "sort", "remove", "pop", "append", "insert", "extend", "clear", "copy", "reverse", "index",
        "keys", "values", "items", "get", "has", "update",
        NULL
    };
    bool is_builtin_method = false;
    for (int i = 0; method_names[i] != NULL; i++) {
        if (prop_name.length == (int)strlen(method_names[i]) &&
            memcmp(prop_name.start, method_names[i], prop_name.length) == 0) {
            is_builtin_method = true;
            break;
        }
    }

    if (is_builtin_method) {
        emit_constant(compiler, OBJ_VAL(copy_string(prop_name.start, prop_name.length)));
        emit_byte(compiler, OP_SWAP);

        if (match(compiler, TOKEN_LEFT_PAREN)) {
            int arg_count = 1;
            if (compiler->current.type != TOKEN_RIGHT_PAREN) {
                do {
                    expression(compiler);
                    arg_count++;
                } while (match(compiler, TOKEN_COMMA));
            }
            consume(compiler, TOKEN_RIGHT_PAREN, "Expect ')' after method arguments");
            emit_bytes(compiler, OP_CALL, (uint8_t)arg_count);
        } else {
            emit_bytes(compiler, OP_CALL, 1);
        }
        return;
    }

    uint8_t name = identifier_constant(compiler, &prop_name);

    if (can_assign && match(compiler, TOKEN_EQUAL)) {
        expression(compiler);
        emit_bytes(compiler, OP_SET_PROPERTY, name);
    } else if (match(compiler, TOKEN_LEFT_PAREN)) {
        int arg_count = argument_list(compiler);
        emit_bytes(compiler, OP_INVOKE_WITH, name);
        emit_byte(compiler, (uint8_t)arg_count);
    } else {
        emit_bytes(compiler, OP_GET_PROPERTY, name);
    }
}

static void new_expr(Compiler* compiler, bool can_assign) {
    consume(compiler, TOKEN_IDENTIFIER, "Expect class name after 'new'");
    uint8_t name = identifier_constant(compiler, &compiler->previous);
    emit_bytes(compiler, OP_NEW_INSTANCE, name);

    if (match(compiler, TOKEN_LEFT_PAREN)) {
        int arg_count = argument_list(compiler);
        emit_bytes(compiler, OP_INVOKE_WITH, 
            identifier_constant(compiler, &(Token){TOKEN_IDENTIFIER, "init", 4, compiler->previous.line}));
        emit_byte(compiler, (uint8_t)arg_count);
    }
}

static void super_expr(Compiler* compiler, bool can_assign) {
    consume(compiler, TOKEN_DOT, "Expect '.' after 'super'");
    consume(compiler, TOKEN_IDENTIFIER, "Expect superclass method name");
    uint8_t name = identifier_constant(compiler, &compiler->previous);

    if (match(compiler, TOKEN_LEFT_PAREN)) {
        int arg_count = argument_list(compiler);
        emit_bytes(compiler, OP_SUPER_INVOKE, name);
        emit_byte(compiler, (uint8_t)arg_count);
    } else {
        emit_bytes(compiler, OP_SUPER, name);
    }
}

static void self_expr(Compiler* compiler, bool can_assign) {
    named_variable(compiler, (Token){TOKEN_SELF, "self", 4, compiler->previous.line}, can_assign);
}

static void tensor_expr(Compiler* compiler, bool can_assign) {
    emit_constant(compiler, OBJ_VAL(copy_string("tensor", 6)));
    consume(compiler, TOKEN_LEFT_PAREN, "Expect '(' after 'tensor'");
    expression(compiler);
    consume(compiler, TOKEN_RIGHT_PAREN, "Expect ')' after tensor argument");
    emit_bytes(compiler, OP_CALL, 1);
}

static void matrix_expr(Compiler* compiler, bool can_assign) {
    emit_constant(compiler, OBJ_VAL(copy_string("matrix", 6)));
    consume(compiler, TOKEN_LEFT_PAREN, "Expect '(' after 'matrix'");
    int arg_count = 0;
    if (compiler->current.type != TOKEN_RIGHT_PAREN) {
        do {
            expression(compiler);
            arg_count++;
        } while (match(compiler, TOKEN_COMMA));
    }
    consume(compiler, TOKEN_RIGHT_PAREN, "Expect ')' after matrix arguments");
    emit_bytes(compiler, OP_CALL, (uint8_t)arg_count);
}

static void train_expr(Compiler* compiler, bool can_assign) {
    emit_constant(compiler, OBJ_VAL(copy_string("train", 5)));
    consume(compiler, TOKEN_LEFT_PAREN, "Expect '(' after 'train'");
    int arg_count = 0;
    if (compiler->current.type != TOKEN_RIGHT_PAREN) {
        do {
            expression(compiler);
            arg_count++;
        } while (match(compiler, TOKEN_COMMA));
    }
    consume(compiler, TOKEN_RIGHT_PAREN, "Expect ')' after train arguments");
    emit_bytes(compiler, OP_CALL, (uint8_t)arg_count);
}

static void predict_expr(Compiler* compiler, bool can_assign) {
    emit_constant(compiler, OBJ_VAL(copy_string("predict", 7)));
    consume(compiler, TOKEN_LEFT_PAREN, "Expect '(' after 'predict'");
    int arg_count = 0;
    if (compiler->current.type != TOKEN_RIGHT_PAREN) {
        do {
            expression(compiler);
            arg_count++;
        } while (match(compiler, TOKEN_COMMA));
    }
    consume(compiler, TOKEN_RIGHT_PAREN, "Expect ')' after predict arguments");
    emit_bytes(compiler, OP_CALL, (uint8_t)arg_count);
}

static void http_server_expr(Compiler* compiler, bool can_assign) {
    emit_constant(compiler, OBJ_VAL(copy_string("http_server", 11)));
    consume(compiler, TOKEN_LEFT_PAREN, "Expect '(' after 'http_server'");
    int arg_count = 0;
    if (compiler->current.type != TOKEN_RIGHT_PAREN) {
        do {
            expression(compiler);
            arg_count++;
        } while (match(compiler, TOKEN_COMMA));
    }
    consume(compiler, TOKEN_RIGHT_PAREN, "Expect ')' after http_server arguments");
    emit_bytes(compiler, OP_CALL, (uint8_t)arg_count);
}

ParseRule rules[] = {
    [TOKEN_LEFT_PAREN]    = {grouping,  call_expr,  PREC_CALL},
    [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,        PREC_NONE},
    [TOKEN_LEFT_BRACKET]  = {list_literal, index_expr, PREC_CALL},
    [TOKEN_RIGHT_BRACKET] = {NULL,     NULL,        PREC_NONE},
    [TOKEN_LEFT_BRACE]    = {brace_literal, NULL,    PREC_NONE},
    [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,        PREC_NONE},
    [TOKEN_PLUS]          = {NULL,     binary,      PREC_TERM},
    [TOKEN_MINUS]         = {unary,    binary,      PREC_TERM},
    [TOKEN_STAR]          = {NULL,     binary,      PREC_FACTOR},
    [TOKEN_SLASH]         = {NULL,     binary,      PREC_FACTOR},
    [TOKEN_PERCENT]       = {NULL,     binary,      PREC_FACTOR},
    [TOKEN_STAR_STAR]     = {NULL,     binary,      PREC_FACTOR},
    [TOKEN_SLASH_SLASH]   = {NULL,     binary,      PREC_FACTOR},
    [TOKEN_AMPERSAND]     = {NULL,     binary,      PREC_BIT_AND},
    [TOKEN_BAR]           = {NULL,     binary,      PREC_BIT_OR},
    [TOKEN_CARET]         = {NULL,     binary,      PREC_BIT_XOR},
    [TOKEN_TILDE]         = {unary,    NULL,        PREC_NONE},
    [TOKEN_LESS_LESS]     = {NULL,     binary,      PREC_SHIFT},
    [TOKEN_GREATER_GREATER] = {NULL,   binary,      PREC_SHIFT},
    [TOKEN_QUESTION]      = {NULL,     ternary_expr, PREC_OR},
    [TOKEN_COMMA]         = {NULL,     NULL,        PREC_NONE},
    [TOKEN_EQUAL]         = {NULL,     NULL,        PREC_NONE},
    [TOKEN_PLUS_EQUAL]    = {NULL,     NULL,        PREC_NONE},
    [TOKEN_MINUS_EQUAL]   = {NULL,     NULL,        PREC_NONE},
    [TOKEN_STAR_EQUAL]    = {NULL,     NULL,        PREC_NONE},
    [TOKEN_SLASH_EQUAL]   = {NULL,     NULL,        PREC_NONE},
    [TOKEN_PERCENT_EQUAL] = {NULL,     NULL,        PREC_NONE},
    [TOKEN_STAR_STAR_EQUAL] = {NULL,   NULL,        PREC_NONE},
    [TOKEN_SLASH_SLASH_EQUAL] = {NULL, NULL,        PREC_NONE},
    [TOKEN_EQUAL_EQUAL]   = {NULL,     binary,      PREC_EQUALITY},
    [TOKEN_BANG]          = {unary,    NULL,        PREC_NONE},
    [TOKEN_BANG_EQUAL]    = {NULL,     binary,      PREC_EQUALITY},
    [TOKEN_IS]            = {NULL,     binary,      PREC_EQUALITY},
    [TOKEN_LESS]          = {NULL,     binary,      PREC_COMPARISON},
    [TOKEN_LESS_EQUAL]    = {NULL,     binary,      PREC_COMPARISON},
    [TOKEN_GREATER]       = {NULL,     binary,      PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL,     binary,      PREC_COMPARISON},
    [TOKEN_OF]            = {NULL,     binary,      PREC_COMPARISON},
    [TOKEN_DOT]           = {NULL,     dot_expr,    PREC_CALL},
    [TOKEN_IDENTIFIER]    = {variable, NULL,        PREC_NONE},
    [TOKEN_STRING]        = {string_literal, NULL,  PREC_NONE},
    [TOKEN_NUMBER]        = {number_literal, NULL,  PREC_NONE},
    [TOKEN_INT]           = {keyword_variable, NULL, PREC_NONE},
    [TOKEN_FLOAT]         = {keyword_variable, NULL, PREC_NONE},
    [TOKEN_STRING_KW]     = {keyword_variable, NULL, PREC_NONE},
    [TOKEN_BOOL_KW]       = {keyword_variable, NULL, PREC_NONE},
    [TOKEN_LIST_KW]       = {keyword_variable, NULL, PREC_NONE},
    [TOKEN_AND]           = {NULL,     binary,      PREC_AND},
    [TOKEN_OR]            = {NULL,     binary,      PREC_OR},
    [TOKEN_FALSE]         = {literal,  NULL,        PREC_NONE},
    [TOKEN_TRUE]          = {literal,  NULL,        PREC_NONE},
    [TOKEN_VOID]          = {literal,  NULL,        PREC_NONE},
    [TOKEN_NOT]           = {unary,    NULL,        PREC_NONE},
    [TOKEN_LEN]           = {len_expr, NULL,        PREC_NONE},
    [TOKEN_TYPE]          = {type_expr, NULL,       PREC_NONE},
    [TOKEN_SET]           = {set_expr, NULL,        PREC_NONE},
    [TOKEN_ASK]         = {input_expression, NULL, PREC_NONE},
    [TOKEN_APPEND]        = {append_expr, NULL,     PREC_NONE},
    [TOKEN_TO_NUM]        = {number_expr, NULL,     PREC_NONE},
    [TOKEN_NEW]           = {new_expr, NULL,        PREC_NONE},
    [TOKEN_SUPER]         = {super_expr, NULL,      PREC_NONE},
    [TOKEN_SELF]          = {self_expr, NULL,       PREC_NONE},
    [TOKEN_TENSOR]        = {tensor_expr, NULL,     PREC_NONE},
    [TOKEN_MATRIX]        = {matrix_expr, NULL,     PREC_NONE},
    [TOKEN_TRAIN]         = {train_expr, NULL,      PREC_NONE},
    [TOKEN_PREDICT]       = {predict_expr, NULL,    PREC_NONE},
    [TOKEN_HTTP]          = {http_server_expr, NULL, PREC_NONE},
    [TOKEN_SERVER]        = {NULL,     NULL,        PREC_NONE},
    [TOKEN_REQUEST]       = {NULL,     NULL,        PREC_NONE},
    [TOKEN_RESPONSE]      = {NULL,     NULL,        PREC_NONE},
    [TOKEN_MODEL]         = {NULL,     NULL,        PREC_NONE},
    [TOKEN_FUNC]          = {lambda_expression, NULL, PREC_NONE},
};

#define RULE_COUNT (sizeof(rules) / sizeof(rules[0]))

static ParseRule* get_rule(TokenType type) {
    if ((int)type >= 0 && (int)type < (int)RULE_COUNT) {
        return &rules[type];
    }
    static ParseRule empty = {NULL, NULL, PREC_NONE};
    return &empty;
}

static void parse_precedence(Compiler* compiler, Precedence precedence) {
    advance(compiler);
    ParseFn prefix_rule = get_rule(compiler->previous.type)->prefix;
    if (prefix_rule == NULL) {
        error(compiler, "Expect expression");
        return;
    }

    bool can_assign = precedence <= PREC_ASSIGNMENT;
    prefix_rule(compiler, can_assign);

    while (precedence <= get_rule(compiler->current.type)->precedence) {
        advance(compiler);
        ParseFn infix_rule = get_rule(compiler->previous.type)->infix;
        infix_rule(compiler, can_assign);
    }
}

bool compile(const char* source, Chunk* chunk) {
    Scanner scanner;
    init_scanner(&scanner, source);

    Compiler compiler;
    compiler.parent = NULL;
    compiler.scanner = &scanner;
    compiler.had_error = false;
    compiler.panic_mode = false;
    compiler.local_count = 0;
    compiler.scope_depth = 0;

    compiler.function = new_function();
    compiler.function->name = NULL;
    compiler.debug_func = chunk_add_debug_func(&compiler.function->chunk, "<script>", 8, 0);

    advance(&compiler);

    while (compiler.current.type == TOKEN_NEWLINE) {
        advance(&compiler);
    }

    while (!match(&compiler, TOKEN_EOF)) {
        if (compiler.current.type == TOKEN_NEWLINE) {
            advance(&compiler);
            continue;
        }
        declaration(&compiler);
    }

    consume(&compiler, TOKEN_EOF, "Expect end of expression");

    emit_return(&compiler);

    bool success = !compiler.had_error;

    if (success) {
        ObjFunction* func = compiler.function;
        *chunk = func->chunk;
    }

#ifdef DEBUG_PRINT_CODE
    if (!compiler.had_error) {
        disassemble_chunk(chunk, "code");
    }
#endif

    return success;
}
