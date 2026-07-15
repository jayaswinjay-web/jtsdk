#ifndef jts_scanner_h
#define jts_scanner_h

#include "common.h"

typedef enum {
    // Single-character tokens
    TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACKET, TOKEN_RIGHT_BRACKET,
    TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
    TOKEN_PLUS, TOKEN_MINUS, TOKEN_STAR, TOKEN_SLASH, TOKEN_PERCENT,
    TOKEN_COMMA, TOKEN_COLON,
    TOKEN_DOT,

    // One or two character tokens
    TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
    TOKEN_BANG, TOKEN_BANG_EQUAL,
    TOKEN_LESS, TOKEN_LESS_EQUAL,
    TOKEN_GREATER, TOKEN_GREATER_EQUAL,

    // Literals
    TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,

    // Keywords
    TOKEN_AND, TOKEN_ELSE, TOKEN_ELIF, TOKEN_FALSE, TOKEN_FOR, TOKEN_FUNC,
    TOKEN_IF, TOKEN_IN, TOKEN_NIL, TOKEN_NOT, TOKEN_OR,
    TOKEN_PRINT, TOKEN_RETURN, TOKEN_TO, TOKEN_TRUE, TOKEN_WHILE,
    TOKEN_INPUT, TOKEN_END, TOKEN_LEN, TOKEN_TYPE, TOKEN_APPEND, TOKEN_TO_NUM,
    TOKEN_BREAK, TOKEN_CONTINUE,

    // Compound assignment
    TOKEN_PLUS_EQUAL, TOKEN_MINUS_EQUAL,

    // Type keywords
    TOKEN_INT, TOKEN_FLOAT, TOKEN_STRING_KW, TOKEN_BOOL_KW, TOKEN_LIST_KW, TOKEN_VAR,

    // OOP keywords
    TOKEN_CLASS, TOKEN_SELF, TOKEN_EXTENDS, TOKEN_SUPER, TOKEN_NEW,

    // Error handling keywords
    TOKEN_TRY, TOKEN_CATCH, TOKEN_THROW,

    // Module keywords
    TOKEN_IMPORT,

    // Web keywords
    TOKEN_HTTP, TOKEN_SERVER, TOKEN_REQUEST, TOKEN_RESPONSE,

    // ML keywords
    TOKEN_TENSOR, TOKEN_MATRIX, TOKEN_TRAIN, TOKEN_PREDICT, TOKEN_MODEL,

    // Special
    TOKEN_INDENT, TOKEN_DEDENT, TOKEN_NEWLINE,
    TOKEN_ERROR, TOKEN_EOF,
} TokenType;

typedef struct {
    TokenType type;
    const char* start;
    int length;
    int line;
} Token;

typedef struct {
    const char* source;
    const char* start;
    const char* current;
    int line;
    int indent_level;
    int indent_stack[256];
    int indent_top;
    bool at_line_start;
    bool need_newline;
    bool need_dedent;
    int pending_dedents;
    bool has_pending;
    Token pending_token;
} Scanner;

void init_scanner(Scanner* scanner, const char* source);
Token scan_token(Scanner* scanner);

#endif
