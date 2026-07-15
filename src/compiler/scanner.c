#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "compiler/scanner.h"
#include "core/memory.h"

void init_scanner(Scanner* scanner, const char* source) {
    scanner->source = source;
    scanner->start = source;
    scanner->current = source;
    scanner->line = 1;
    scanner->indent_level = 0;
    scanner->indent_top = 0;
    scanner->indent_stack[0] = 0;
    scanner->at_line_start = true;
    scanner->need_newline = false;
    scanner->need_dedent = false;
    scanner->pending_dedents = 0;
    scanner->has_pending = false;
}

static bool is_at_end(Scanner* scanner) {
    return *scanner->current == '\0';
}

static char advance(Scanner* scanner) {
    scanner->current++;
    return scanner->current[-1];
}

static char peek_char(Scanner* scanner) {
    return *scanner->current;
}

static char peek_next(Scanner* scanner) {
    if (is_at_end(scanner)) return '\0';
    return scanner->current[1];
}

static bool match_char(Scanner* scanner, char expected) {
    if (is_at_end(scanner)) return false;
    if (*scanner->current != expected) return false;
    scanner->current++;
    return true;
}

static Token make_token(Scanner* scanner, TokenType type) {
    Token token;
    token.type = type;
    token.start = scanner->start;
    token.length = (int)(scanner->current - scanner->start);
    token.line = scanner->line;
    return token;
}

static Token make_token_at(Scanner* scanner, TokenType type, const char* start, int length) {
    Token token;
    token.type = type;
    token.start = start;
    token.length = length;
    token.line = scanner->line;
    return token;
}

static Token error_token(Scanner* scanner, const char* message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = scanner->line;
    return token;
}

static void skip_whitespace_and_comments(Scanner* scanner) {
    for (;;) {
        char c = peek_char(scanner);
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance(scanner);
                break;
            case '#':
                while (peek_char(scanner) != '\n' && !is_at_end(scanner)) {
                    advance(scanner);
                }
                break;
            default:
                return;
        }
    }
}

static TokenType check_keyword(Scanner* scanner, int start_off, int length,
                                const char* rest, TokenType type) {
    if (scanner->current - scanner->start == start_off + length &&
        memcmp(scanner->start + start_off, rest, length) == 0) {
        return type;
    }
    return TOKEN_IDENTIFIER;
}

static TokenType identifier_type(Scanner* scanner) {
    switch (scanner->start[0]) {
        case 'a':
            if (scanner->current - scanner->start > 1) {
                switch (scanner->start[1]) {
                    case 'n': return check_keyword(scanner, 1, 2, "nd", TOKEN_AND);
                    case 'p': return check_keyword(scanner, 1, 5, "ppend", TOKEN_APPEND);
                }
            }
            break;
        case 'b':
            if (scanner->current - scanner->start > 1) {
                switch (scanner->start[1]) {
                    case 'o': return check_keyword(scanner, 2, 2, "ol", TOKEN_BOOL_KW);
                    case 'r': return check_keyword(scanner, 2, 3, "eak", TOKEN_BREAK);
                }
            }
            break;
        case 'c':
            if (scanner->current - scanner->start > 1) {
                switch (scanner->start[1]) {
                    case 'l': return check_keyword(scanner, 2, 3, "ass", TOKEN_CLASS);
                    case 'a': return check_keyword(scanner, 2, 3, "tch", TOKEN_CATCH);
                    case 'o': return check_keyword(scanner, 2, 6, "ntinue", TOKEN_CONTINUE);
                }
            }
            break;
        case 'e':
            if (scanner->current - scanner->start > 1) {
                switch (scanner->start[1]) {
                    case 'l': 
                        if (scanner->current - scanner->start > 2 && scanner->start[2] == 'i')
                            return check_keyword(scanner, 3, 1, "f", TOKEN_ELIF);
                        return check_keyword(scanner, 2, 2, "se", TOKEN_ELSE);
                    case 'n': return check_keyword(scanner, 2, 1, "d", TOKEN_END);
                    case 'x': return check_keyword(scanner, 2, 5, "tends", TOKEN_EXTENDS);
                }
            }
            break;
        case 'f':
            if (scanner->current - scanner->start > 1) {
                switch (scanner->start[1]) {
                    case 'a': return check_keyword(scanner, 2, 3, "lse", TOKEN_FALSE);
                    case 'l': return check_keyword(scanner, 2, 3, "oat", TOKEN_FLOAT);
                    case 'o': return check_keyword(scanner, 2, 1, "r", TOKEN_FOR);
                    case 'u': return check_keyword(scanner, 2, 2, "nc", TOKEN_FUNC);
                }
            }
            break;
        case 'h':
            if (scanner->current - scanner->start > 1) {
                switch (scanner->start[1]) {
                    case 't': return check_keyword(scanner, 2, 2, "tp", TOKEN_HTTP);
                }
            }
            break;
        case 'i':
            if (scanner->current - scanner->start > 1) {
                switch (scanner->start[1]) {
                    case 'f': return check_keyword(scanner, 2, 0, "", TOKEN_IF);
                    case 'n':
                        if (scanner->current - scanner->start > 2) {
                            if (scanner->start[2] == 'p')
                                return check_keyword(scanner, 3, 2, "ut", TOKEN_INPUT);
                            if (scanner->start[2] == 't')
                                return check_keyword(scanner, 1, 2, "nt", TOKEN_INT);
                        }
                        return check_keyword(scanner, 1, 1, "n", TOKEN_IN);
                    case 'm': return check_keyword(scanner, 2, 4, "port", TOKEN_IMPORT);
                }
            }
            break;
        case 'l':
            if (scanner->current - scanner->start > 1) {
                switch (scanner->start[1]) {
                    case 'e': return check_keyword(scanner, 1, 2, "en", TOKEN_LEN);
                    case 'i': return check_keyword(scanner, 2, 2, "st", TOKEN_LIST_KW);
                }
            }
            return check_keyword(scanner, 1, 2, "en", TOKEN_LEN);
        case 'm':
            if (scanner->current - scanner->start > 1) {
                switch (scanner->start[1]) {
                    case 'a': return check_keyword(scanner, 2, 5, "trix", TOKEN_MATRIX);
                    case 'o': return check_keyword(scanner, 2, 3, "del", TOKEN_MODEL);
                }
            }
            break;
        case 'n':
            if (scanner->current - scanner->start > 1) {
                switch (scanner->start[1]) {
                    case 'e': return check_keyword(scanner, 2, 1, "w", TOKEN_NEW);
                    case 'i': return check_keyword(scanner, 2, 1, "l", TOKEN_NIL);
                    case 'o': return check_keyword(scanner, 2, 1, "t", TOKEN_NOT);
                    case 'u': return check_keyword(scanner, 2, 4, "mber", TOKEN_TO_NUM);
                }
            }
            break;
        case 'o': return check_keyword(scanner, 1, 1, "r", TOKEN_OR);
        case 'p':
            if (scanner->current - scanner->start > 1) {
                switch (scanner->start[1]) {
                    case 'r':
                        if (scanner->current - scanner->start > 3 && scanner->start[2] == 'e')
                            return check_keyword(scanner, 2, 5, "edict", TOKEN_PREDICT);
                        return check_keyword(scanner, 1, 4, "rint", TOKEN_PRINT);
                }
            }
            return check_keyword(scanner, 1, 4, "rint", TOKEN_PRINT);
        case 'r':
            if (scanner->current - scanner->start > 1) {
                switch (scanner->start[1]) {
                    case 'e':
                        if (scanner->current - scanner->start > 2) {
                            if (scanner->start[2] == 'q')
                                return check_keyword(scanner, 3, 7, "uest", TOKEN_REQUEST);
                            if (scanner->start[2] == 's')
                                return check_keyword(scanner, 3, 6, "ponse", TOKEN_RESPONSE);
                        }
                        return check_keyword(scanner, 1, 5, "eturn", TOKEN_RETURN);
                }
            }
            break;
        case 's':
            if (scanner->current - scanner->start > 1) {
                switch (scanner->start[1]) {
                    case 'e':
                        if (scanner->current - scanner->start > 2) {
                            if (scanner->start[2] == 'l')
                                return check_keyword(scanner, 3, 1, "f", TOKEN_SELF);
                            if (scanner->start[2] == 'r')
                                return check_keyword(scanner, 3, 3, "ver", TOKEN_SERVER);
                        }
                        break;
                    case 'u': return check_keyword(scanner, 2, 3, "per", TOKEN_SUPER);
                    case 't': return check_keyword(scanner, 2, 4, "ring", TOKEN_STRING_KW);
                }
            }
            break;
        case 't':
            if (scanner->current - scanner->start > 1) {
                switch (scanner->start[1]) {
                    case 'e': return check_keyword(scanner, 2, 4, "nsor", TOKEN_TENSOR);
                    case 'o': return check_keyword(scanner, 2, 0, "", TOKEN_TO);
                    case 'r':
                        if (scanner->current - scanner->start > 2 && scanner->start[2] == 'y')
                            return check_keyword(scanner, 2, 1, "y", TOKEN_TRY);
                        return check_keyword(scanner, 2, 2, "ue", TOKEN_TRUE);
                    case 'y':
                        if (scanner->current - scanner->start > 2 && scanner->start[2] == 'p')
                            return check_keyword(scanner, 3, 1, "e", TOKEN_TYPE);
                        break;
                    case 'h': return check_keyword(scanner, 2, 3, "row", TOKEN_THROW);
                }
            }
            break;
        case 'v':
            if (scanner->current - scanner->start > 1) {
                switch (scanner->start[1]) {
                    case 'a': return check_keyword(scanner, 2, 1, "r", TOKEN_VAR);
                }
            }
            break;
        case 'w': return check_keyword(scanner, 1, 4, "hile", TOKEN_WHILE);
    }
    return TOKEN_IDENTIFIER;
}

static Token identifier(Scanner* scanner) {
    while (isalpha(peek_char(scanner)) || isdigit(peek_char(scanner)) || peek_char(scanner) == '_') {
        advance(scanner);
    }
    return make_token(scanner, identifier_type(scanner));
}

static Token number(Scanner* scanner) {
    while (isdigit(peek_char(scanner))) {
        advance(scanner);
    }
    if (peek_char(scanner) == '.' && isdigit(peek_next(scanner))) {
        advance(scanner);
        while (isdigit(peek_char(scanner))) {
            advance(scanner);
        }
    }
    return make_token(scanner, TOKEN_NUMBER);
}

static Token string_token(Scanner* scanner) {
    char quote = scanner->current[-1];
    scanner->start = scanner->current;
    while (peek_char(scanner) != quote && !is_at_end(scanner)) {
        if (peek_char(scanner) == '\n') scanner->line++;
        advance(scanner);
    }
    if (is_at_end(scanner)) {
        return error_token(scanner, "Unterminated string");
    }
    Token token = make_token(scanner, TOKEN_STRING);
    advance(scanner); // closing quote
    return token;
}

static void skip_line_rest(Scanner* scanner) {
    while (peek_char(scanner) != '\n' && !is_at_end(scanner)) {
        advance(scanner);
    }
}

static Token handle_newline(Scanner* scanner) {
    scanner->line++;
    scanner->at_line_start = true;
    Token token;
    token.type = TOKEN_NEWLINE;
    token.start = "newline";
    token.length = 0;
    token.line = scanner->line - 1;
    return token;
}

static Token handle_eof(Scanner* scanner) {
    if (scanner->indent_level > 0) {
        scanner->pending_dedents = scanner->indent_level;
        scanner->indent_level = 0;
        scanner->pending_dedents--;
        Token token;
        token.type = TOKEN_DEDENT;
        token.start = "dedent";
        token.length = 0;
        token.line = scanner->line;
        return token;
    }
    Token token;
    token.type = TOKEN_EOF;
    token.start = scanner->current;
    token.length = 0;
    token.line = scanner->line;
    return token;
}

static Token scan_line_start(Scanner* scanner) {
    scanner->at_line_start = false;

    skip_whitespace_and_comments(scanner);

    if (is_at_end(scanner)) {
        return handle_eof(scanner);
    }

    if (peek_char(scanner) == '\n') {
        advance(scanner);
        return handle_newline(scanner);
    }

    int indent = 0;
    while (peek_char(scanner) == ' ' || peek_char(scanner) == '\t') {
        if (peek_char(scanner) == '\t') {
            indent += 4;
        } else {
            indent++;
        }
        advance(scanner);
    }

    skip_whitespace_and_comments(scanner);

    if (is_at_end(scanner) || peek_char(scanner) == '\n' || peek_char(scanner) == '#') {
        if (peek_char(scanner) == '#') {
            skip_line_rest(scanner);
        }
        if (is_at_end(scanner)) {
            return handle_eof(scanner);
        }
        if (peek_char(scanner) == '\n') {
            advance(scanner);
        }
        scanner->at_line_start = true;
        Token token;
        token.type = TOKEN_NEWLINE;
        token.start = "newline";
        token.length = 0;
        token.line = scanner->line;
        return token;
    }

    if (indent > scanner->indent_level) {
        scanner->indent_stack[scanner->indent_top] = scanner->indent_level;
        scanner->indent_top++;
        scanner->indent_level = indent;
        Token token;
        token.type = TOKEN_INDENT;
        token.start = "indent";
        token.length = 0;
        token.line = scanner->line;
        return token;
    }

    if (indent < scanner->indent_level) {
        scanner->indent_level = indent;
        scanner->pending_dedents = 0;
        for (int i = scanner->indent_top - 1; i >= 0; i--) {
            if (scanner->indent_stack[i] == indent) {
                scanner->pending_dedents = scanner->indent_top - i - 1;
                scanner->indent_top = i;
                break;
            }
        }
        if (scanner->pending_dedents > 0) {
            scanner->pending_dedents--;
            Token token;
            token.type = TOKEN_DEDENT;
            token.start = "dedent";
            token.length = 0;
            token.line = scanner->line;
            return token;
        }
    }

    return (Token){TOKEN_ERROR, "", 0, scanner->line};
}

Token scan_token(Scanner* scanner) {
    if (scanner->has_pending) {
        scanner->has_pending = false;
        return scanner->pending_token;
    }

    if (scanner->pending_dedents > 0) {
        scanner->pending_dedents--;
        Token token;
        token.type = TOKEN_DEDENT;
        token.start = "dedent";
        token.length = 0;
        token.line = scanner->line;
#ifdef DEBUG_SCANNER
        fprintf(stderr, "[SCAN] DEDENT (pending=%d)\n", scanner->pending_dedents);
#endif
        return token;
    }

    if (scanner->at_line_start) {
        scanner->at_line_start = false;

        int indent = 0;
        while (peek_char(scanner) == ' ' || peek_char(scanner) == '\t') {
            if (peek_char(scanner) == '\t') {
                indent += 4;
            } else {
                indent++;
            }
            advance(scanner);
        }

        skip_whitespace_and_comments(scanner);

        if (is_at_end(scanner)) {
            return handle_eof(scanner);
        }

        if (peek_char(scanner) == '\n') {
            advance(scanner);
            return handle_newline(scanner);
        }

        if (indent > scanner->indent_level) {
            scanner->indent_stack[scanner->indent_top] = scanner->indent_level;
            scanner->indent_top++;
            scanner->indent_level = indent;
            Token token;
            token.type = TOKEN_INDENT;
            token.start = "indent";
            token.length = 0;
            token.line = scanner->line;
            return token;
        }

        if (indent < scanner->indent_level) {
            scanner->indent_level = indent;
            scanner->pending_dedents = 0;
            for (int i = scanner->indent_top - 1; i >= 0; i--) {
                if (scanner->indent_stack[i] == indent) {
                    scanner->pending_dedents = scanner->indent_top - i;
                    scanner->indent_top = i;
                    break;
                }
            }
            if (scanner->pending_dedents > 0) {
                scanner->pending_dedents--;
                Token token;
                token.type = TOKEN_DEDENT;
                token.start = "dedent";
                token.length = 0;
                token.line = scanner->line;
                return token;
            }
        }

        // Same indent level, fall through to normal scanning
    }

    skip_whitespace_and_comments(scanner);

    scanner->start = scanner->current;

    if (is_at_end(scanner)) {
        Token token;
        token.type = TOKEN_EOF;
        token.start = scanner->current;
        token.length = 0;
        token.line = scanner->line;
        return token;
    }

    char c = advance(scanner);

    if (isalpha(c) || c == '_') {
        Token t = identifier(scanner);
#ifdef DEBUG_SCANNER
        fprintf(stderr, "[SCAN] IDENT/KEYW '%.*s' line=%d\n", t.length, t.start, t.line);
#endif
        return t;
    }
    if (isdigit(c)) {
        Token t = number(scanner);
#ifdef DEBUG_SCANNER
        fprintf(stderr, "[SCAN] NUMBER '%.*s' line=%d\n", t.length, t.start, t.line);
#endif
        return t;
    }

    switch (c) {
        case '(': { Token t = {TOKEN_LEFT_PAREN, scanner->start, 1, scanner->line}; return t; }
        case ')': { Token t = {TOKEN_RIGHT_PAREN, scanner->start, 1, scanner->line}; return t; }
        case '[': { Token t = {TOKEN_LEFT_BRACKET, scanner->start, 1, scanner->line}; return t; }
        case ']': { Token t = {TOKEN_RIGHT_BRACKET, scanner->start, 1, scanner->line}; return t; }
        case '{': { Token t = {TOKEN_LEFT_BRACE, scanner->start, 1, scanner->line}; return t; }
        case '}': { Token t = {TOKEN_RIGHT_BRACE, scanner->start, 1, scanner->line}; return t; }
        case '+':
            if (match_char(scanner, '=')) {
                return make_token(scanner, TOKEN_PLUS_EQUAL);
            }
            return make_token(scanner, TOKEN_PLUS);
        case '-':
            if (match_char(scanner, '=')) {
                return make_token(scanner, TOKEN_MINUS_EQUAL);
            }
            return make_token(scanner, TOKEN_MINUS);
        case ':': { Token t = {TOKEN_COLON, scanner->start, 1, scanner->line}; return t; }
        case '*': { Token t = {TOKEN_STAR, scanner->start, 1, scanner->line}; return t; }
        case '/': { Token t = {TOKEN_SLASH, scanner->start, 1, scanner->line}; return t; }
        case '%': { Token t = {TOKEN_PERCENT, scanner->start, 1, scanner->line}; return t; }
        case ',': { Token t = {TOKEN_COMMA, scanner->start, 1, scanner->line}; return t; }
        case '.': { Token t = {TOKEN_DOT, scanner->start, 1, scanner->line}; return t; }
        case '=':
            if (match_char(scanner, '=')) {
                return make_token(scanner, TOKEN_EQUAL_EQUAL);
            }
            return make_token(scanner, TOKEN_EQUAL);
        case '!':
            if (match_char(scanner, '=')) {
                return make_token(scanner, TOKEN_BANG_EQUAL);
            }
            return make_token(scanner, TOKEN_BANG);
        case '<':
            if (match_char(scanner, '=')) {
                return make_token(scanner, TOKEN_LESS_EQUAL);
            }
            return make_token(scanner, TOKEN_LESS);
        case '>':
            if (match_char(scanner, '=')) {
                return make_token(scanner, TOKEN_GREATER_EQUAL);
            }
            return make_token(scanner, TOKEN_GREATER);
        case '"':
        case '\'':
            return string_token(scanner);
        case '\n':
            return handle_newline(scanner);
    }

    return error_token(scanner, "Unexpected character");
}
