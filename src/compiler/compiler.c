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
    emit_byte(compiler, OP_NIL);
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
        emit_byte(compiler, OP_POP);
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

static void add_local(Compiler* compiler, Token name) {
    if (compiler->local_count == MAX_LOCALS) {
        error(compiler, "Too many local variables");
        return;
    }
    Local* local = &compiler->locals[compiler->local_count++];
    local->name = name;
    local->depth = -1;
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
    emit_byte(compiler, OP_PRINT);
}

static void input_expression(Compiler* compiler, bool can_assign) {
    emit_constant(compiler, OBJ_VAL(copy_string("input", 5)));
    consume(compiler, TOKEN_LEFT_PAREN, "Expect '(' after 'input'");
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

    // Patch break jumps to after loop
    for (int i = saved_break_count; i < break_count; i++) {
        patch_jump(compiler, break_jumps[i]);
    }
    break_count = saved_break_count;
    loop_depth--;

    patch_jump(compiler, exit_jump);
    emit_byte(compiler, OP_POP);
}

static void for_statement(Compiler* compiler) {
    begin_scope(compiler);

    consume(compiler, TOKEN_IDENTIFIER, "Expect variable name after 'for'");
    Token var_name = compiler->previous;
    uint8_t var_constant = identifier_constant(compiler, &var_name);

    consume(compiler, TOKEN_IN, "Expect 'in' after variable name");
    expression(compiler);

    emit_bytes(compiler, OP_DEFINE_GLOBAL, var_constant);

    consume(compiler, TOKEN_TO, "Expect 'to' after start value");
    expression(compiler);

    uint8_t end_name = make_constant(compiler,
        OBJ_VAL(copy_string("_jts_for_end", 11)));
    emit_bytes(compiler, OP_DEFINE_GLOBAL, end_name);

    int loop_start = current_chunk(compiler)->count;

    emit_bytes(compiler, OP_GET_GLOBAL, var_constant);
    emit_bytes(compiler, OP_GET_GLOBAL, end_name);
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

    // Patch continue jumps to increment
    int increment_start = current_chunk(compiler)->count;

    emit_bytes(compiler, OP_GET_GLOBAL, var_constant);
    emit_constant(compiler, NUMBER_VAL(1));
    emit_byte(compiler, OP_ADD);
    emit_bytes(compiler, OP_SET_GLOBAL, var_constant);
    emit_byte(compiler, OP_POP);

    for (int i = saved_continue_count; i < continue_count; i++) {
        int offset = current_chunk(compiler)->count - continue_jumps[i] - 2;
        current_chunk(compiler)->code[continue_jumps[i]] = (offset >> 8) & 0xff;
        current_chunk(compiler)->code[continue_jumps[i] + 1] = offset & 0xff;
    }
    continue_count = saved_continue_count;

    emit_loop(compiler, loop_start);

    // Patch break jumps
    for (int i = saved_break_count; i < break_count; i++) {
        patch_jump(compiler, break_jumps[i]);
    }
    break_count = saved_break_count;
    loop_depth--;

    patch_jump(compiler, exit_jump);
    emit_byte(compiler, OP_POP);

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

static void func_definition(Compiler* compiler) {
    consume(compiler, TOKEN_IDENTIFIER, "Expect function name");
    Token name = compiler->previous;

    Compiler fn_compiler;
    fn_compiler.parent = compiler;
    fn_compiler.scanner = compiler->scanner;
    fn_compiler.had_error = false;
    fn_compiler.panic_mode = false;
    fn_compiler.local_count = 0;
    fn_compiler.scope_depth = 0;
    fn_compiler.current = compiler->current;
    fn_compiler.previous = compiler->previous;

    fn_compiler.function = new_function();
    fn_compiler.function->name = copy_string(name.start, name.length);

    Local* local = &fn_compiler.locals[fn_compiler.local_count++];
    local->depth = 0;
    local->name.start = "";
    local->name.length = 0;

    begin_scope(&fn_compiler);

    consume(&fn_compiler, TOKEN_LEFT_PAREN, "Expect '(' after function name");

        int param_count = 0;
        if (!match(&fn_compiler, TOKEN_RIGHT_PAREN)) {
            do {
                if (fn_compiler.current.type == TOKEN_IDENTIFIER ||
                    fn_compiler.current.type == TOKEN_SELF) {
                    advance(&fn_compiler);
                } else {
                    consume(&fn_compiler, TOKEN_IDENTIFIER, "Expect parameter name");
                }
                declare_variable(&fn_compiler);
                mark_initialized(&fn_compiler);
                fn_compiler.locals[fn_compiler.local_count - 1].depth = 1;
                param_count++;
            } while (match(&fn_compiler, TOKEN_COMMA));
            consume(&fn_compiler, TOKEN_RIGHT_PAREN, "Expect ')' after parameters");
        }
        fn_compiler.function->arity = param_count;

    if (fn_compiler.current.type == TOKEN_NEWLINE) {
        advance(&fn_compiler);
    }

    block(&fn_compiler);
    match(&fn_compiler, TOKEN_END);

    emit_return(&fn_compiler);

    end_scope(&fn_compiler);

    ObjFunction* function = fn_compiler.function;

    compiler->had_error = fn_compiler.had_error;
    compiler->current = fn_compiler.current;
    compiler->previous = fn_compiler.previous;

    if (fn_compiler.had_error) return;

    uint8_t constant = make_constant(compiler, OBJ_VAL(function));
    emit_bytes(compiler, OP_CONSTANT, constant);

    uint8_t name_constant = identifier_constant(compiler, &name);
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
            fn_compiler.parent = compiler;
            fn_compiler.scanner = compiler->scanner;
            fn_compiler.had_error = false;
            fn_compiler.panic_mode = false;
            fn_compiler.local_count = 0;
            fn_compiler.scope_depth = 0;
            fn_compiler.current = compiler->current;
            fn_compiler.previous = compiler->previous;

            fn_compiler.function = new_function();
            fn_compiler.function->name = copy_string(method_name.start, method_name.length);

            Local* local = &fn_compiler.locals[fn_compiler.local_count++];
            local->depth = 0;
            local->name.start = "";
            local->name.length = 0;

            begin_scope(&fn_compiler);

            consume(&fn_compiler, TOKEN_LEFT_PAREN, "Expect '(' after method name");

            int param_count = 0;
            if (!match(&fn_compiler, TOKEN_RIGHT_PAREN)) {
                do {
                    if (fn_compiler.current.type == TOKEN_IDENTIFIER ||
                        fn_compiler.current.type == TOKEN_SELF) {
                        advance(&fn_compiler);
                    } else {
                        consume(&fn_compiler, TOKEN_IDENTIFIER, "Expect parameter name");
                    }
                    declare_variable(&fn_compiler);
                    mark_initialized(&fn_compiler);
                    fn_compiler.locals[fn_compiler.local_count - 1].depth = 1;
                    param_count++;
                } while (match(&fn_compiler, TOKEN_COMMA));
                consume(&fn_compiler, TOKEN_RIGHT_PAREN, "Expect ')' after parameters");
            }
            fn_compiler.function->arity = param_count;

            if (fn_compiler.current.type == TOKEN_NEWLINE) {
                advance(&fn_compiler);
            }

            block(&fn_compiler);
            match(&fn_compiler, TOKEN_END);

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
    } else {
        emit_byte(compiler, OP_NIL);
    }
    emit_byte(compiler, OP_RETURN);
}

static void statement(Compiler* compiler) {
    if (match(compiler, TOKEN_PRINT)) {
        print_statement(compiler);
    } else if (match(compiler, TOKEN_IF)) {
        if_statement(compiler);
    } else if (match(compiler, TOKEN_WHILE)) {
        while_statement(compiler);
    } else if (match(compiler, TOKEN_FOR)) {
        for_statement(compiler);
    } else if (match(compiler, TOKEN_RETURN)) {
        return_statement(compiler);
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
        int skip_catch = emit_jump(compiler, OP_JUMP);

        // Catch handler start — OP_TRY_SET_IP jumps here on exception
        // (error value is on TOS)
        patch_jump(compiler, handler_offset);

        if (match(compiler, TOKEN_CATCH)) {
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
        }

        match(compiler, TOKEN_END);
        patch_jump(compiler, skip_catch);
    } else if (match(compiler, TOKEN_THROW)) {
        expression(compiler);
        emit_byte(compiler, OP_THROW);
    } else if (match(compiler, TOKEN_INPUT)) {
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
    emit_byte(compiler, OP_POP);
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
        // For now, emit the filename as a constant for runtime handling
        emit_constant(compiler, OBJ_VAL(copy_string(
            compiler->previous.start, compiler->previous.length)));
        emit_constant(compiler, OBJ_VAL(copy_string("import_file", 11)));
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
        advance(compiler); // consume the type keyword
        Token name = compiler->current;
        consume(compiler, TOKEN_IDENTIFIER, "Expect variable name after type");
        
        if (match(compiler, TOKEN_EQUAL)) {
            // Type-annotated with value: int x = 10
            expression(compiler);
        } else {
            // Unassigned: int x (default to nil)
            emit_byte(compiler, OP_NIL);
        }
        
        uint8_t arg = identifier_constant(compiler, &name);
        emit_bytes(compiler, OP_DEFINE_GLOBAL, arg);
        return;
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
            case TOKEN_PRINT:
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
    parse_precedence(compiler, (Precedence)(rule->precedence + 1));

    switch (operator_type) {
        case TOKEN_PLUS:          emit_byte(compiler, OP_ADD); break;
        case TOKEN_MINUS:         emit_byte(compiler, OP_SUBTRACT); break;
        case TOKEN_STAR:          emit_byte(compiler, OP_MULTIPLY); break;
        case TOKEN_SLASH:         emit_byte(compiler, OP_DIVIDE); break;
        case TOKEN_PERCENT:       emit_byte(compiler, OP_MODULO); break;
        case TOKEN_EQUAL_EQUAL:   emit_byte(compiler, OP_EQUAL); break;
        case TOKEN_BANG_EQUAL:    emit_byte(compiler, OP_NOT_EQUAL); break;
        case TOKEN_GREATER:       emit_byte(compiler, OP_GREATER); break;
        case TOKEN_LESS:          emit_byte(compiler, OP_LESS); break;
        case TOKEN_GREATER_EQUAL: emit_byte(compiler, OP_GREATER_EQUAL); break;
        case TOKEN_LESS_EQUAL:    emit_byte(compiler, OP_LESS_EQUAL); break;
        case TOKEN_AND: {
            int right_jump = emit_jump(compiler, OP_JUMP_IF_FALSE);
            emit_byte(compiler, OP_POP);
            parse_precedence(compiler, PREC_AND);
            patch_jump(compiler, right_jump);
            break;
        }
        case TOKEN_OR: {
            int right_jump = emit_jump(compiler, OP_JUMP);
            emit_byte(compiler, OP_POP);
            parse_precedence(compiler, PREC_OR);
            patch_jump(compiler, right_jump);
            break;
        }
        default: return;
    }
}

static void literal(Compiler* compiler, bool can_assign) {
    switch (compiler->previous.type) {
        case TOKEN_FALSE: emit_byte(compiler, OP_FALSE); break;
        case TOKEN_NIL:   emit_byte(compiler, OP_NIL); break;
        case TOKEN_TRUE:  emit_byte(compiler, OP_TRUE); break;
        default: return;
    }
}

static void grouping(Compiler* compiler, bool can_assign) {
    expression(compiler);
    consume(compiler, TOKEN_RIGHT_PAREN, "Expect ')' after expression");
}

static void number_literal(Compiler* compiler, bool can_assign) {
    double value = strtod(compiler->previous.start, NULL);
    emit_constant(compiler, NUMBER_VAL(value));
}

static void string_literal(Compiler* compiler, bool can_assign) {
    ObjString* string = copy_string(compiler->previous.start,
                                     compiler->previous.length);
    emit_constant(compiler, OBJ_VAL(string));
}

static void unary(Compiler* compiler, bool can_assign) {
    TokenType operator_type = compiler->previous.type;
    parse_precedence(compiler, PREC_UNARY);

    switch (operator_type) {
        case TOKEN_MINUS: emit_byte(compiler, OP_NEGATE); break;
        case TOKEN_NOT:   emit_byte(compiler, OP_NOT); break;
        default: return;
    }
}

static void named_variable(Compiler* compiler, Token name, bool can_assign) {
    uint8_t get_op, set_op;
    int arg = resolve_local(compiler, &name);
    if (arg != -1) {
        get_op = OP_GET_LOCAL;
        set_op = OP_SET_LOCAL;
    } else {
        arg = identifier_constant(compiler, &name);
        get_op = OP_GET_GLOBAL;
        set_op = OP_SET_GLOBAL;
    }

    if (can_assign && match(compiler, TOKEN_EQUAL)) {
        expression(compiler);
        emit_bytes(compiler, set_op, (uint8_t)arg);
    } else if (can_assign && match(compiler, TOKEN_PLUS_EQUAL)) {
        emit_bytes(compiler, get_op, (uint8_t)arg);
        expression(compiler);
        emit_byte(compiler, OP_ADD);
        emit_bytes(compiler, set_op, (uint8_t)arg);
    } else if (can_assign && match(compiler, TOKEN_MINUS_EQUAL)) {
        emit_bytes(compiler, get_op, (uint8_t)arg);
        expression(compiler);
        emit_byte(compiler, OP_SUBTRACT);
        emit_bytes(compiler, set_op, (uint8_t)arg);
    } else {
        emit_bytes(compiler, get_op, (uint8_t)arg);
    }
}

static void variable(Compiler* compiler, bool can_assign) {
    named_variable(compiler, compiler->previous, can_assign);
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

static void list_literal(Compiler* compiler, bool can_assign) {
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
                ObjString* key = copy_string(compiler->previous.start, compiler->previous.length);
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

static void index_expr(Compiler* compiler, bool can_assign) {
    expression(compiler);
    consume(compiler, TOKEN_RIGHT_BRACKET, "Expect ']' after index");

    if (can_assign && match(compiler, TOKEN_EQUAL)) {
        expression(compiler);
        emit_byte(compiler, OP_INDEX_SET);
    } else {
        emit_byte(compiler, OP_INDEX);
    }
}

static void dot_expr(Compiler* compiler, bool can_assign) {
    consume(compiler, TOKEN_IDENTIFIER, "Expect property name after '.'");
    Token prop_name = compiler->previous;

    // String method dispatch: transform s.method(args) into method_native(s, args)
    static const char* string_methods[] = {
        "upper", "lower", "trim", "split", "contains", "replace", "substring", "starts_with", "ends_with", NULL
    };
    bool is_string_method = false;
    for (int i = 0; string_methods[i] != NULL; i++) {
        if (prop_name.length == (int)strlen(string_methods[i]) &&
            memcmp(prop_name.start, string_methods[i], prop_name.length) == 0) {
            is_string_method = true;
            break;
        }
    }

    if (is_string_method) {
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

    static const char* list_methods[] = {
        "sort", "remove", "pop", "append", NULL
    };
    bool is_list_method = false;
    for (int i = 0; list_methods[i] != NULL; i++) {
        if (prop_name.length == (int)strlen(list_methods[i]) &&
            memcmp(prop_name.start, list_methods[i], prop_name.length) == 0) {
            is_list_method = true;
            break;
        }
    }

    if (is_list_method) {
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
    [TOKEN_LEFT_BRACE]    = {dict_literal, NULL,    PREC_NONE},
    [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,        PREC_NONE},
    [TOKEN_PLUS]          = {NULL,     binary,      PREC_TERM},
    [TOKEN_MINUS]         = {unary,    binary,      PREC_TERM},
    [TOKEN_STAR]          = {NULL,     binary,      PREC_FACTOR},
    [TOKEN_SLASH]         = {NULL,     binary,      PREC_FACTOR},
    [TOKEN_PERCENT]       = {NULL,     binary,      PREC_FACTOR},
    [TOKEN_COMMA]         = {NULL,     NULL,        PREC_NONE},
    [TOKEN_EQUAL]         = {NULL,     NULL,        PREC_NONE},
    [TOKEN_EQUAL_EQUAL]   = {NULL,     binary,      PREC_EQUALITY},
    [TOKEN_BANG]          = {unary,    NULL,        PREC_NONE},
    [TOKEN_BANG_EQUAL]    = {NULL,     binary,      PREC_EQUALITY},
    [TOKEN_LESS]          = {NULL,     binary,      PREC_COMPARISON},
    [TOKEN_LESS_EQUAL]    = {NULL,     binary,      PREC_COMPARISON},
    [TOKEN_GREATER]       = {NULL,     binary,      PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL,     binary,      PREC_COMPARISON},
    [TOKEN_DOT]           = {NULL,     dot_expr,    PREC_CALL},
    [TOKEN_IDENTIFIER]    = {variable, NULL,        PREC_NONE},
    [TOKEN_STRING]        = {string_literal, NULL,  PREC_NONE},
    [TOKEN_NUMBER]        = {number_literal, NULL,  PREC_NONE},
    [TOKEN_AND]           = {NULL,     binary,      PREC_AND},
    [TOKEN_OR]            = {NULL,     binary,      PREC_OR},
    [TOKEN_FALSE]         = {literal,  NULL,        PREC_NONE},
    [TOKEN_TRUE]          = {literal,  NULL,        PREC_NONE},
    [TOKEN_NIL]           = {literal,  NULL,        PREC_NONE},
    [TOKEN_NOT]           = {unary,    NULL,        PREC_NONE},
    [TOKEN_LEN]           = {len_expr, NULL,        PREC_NONE},
    [TOKEN_TYPE]          = {type_expr, NULL,       PREC_NONE},
    [TOKEN_INPUT]         = {input_expression, NULL, PREC_NONE},
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
