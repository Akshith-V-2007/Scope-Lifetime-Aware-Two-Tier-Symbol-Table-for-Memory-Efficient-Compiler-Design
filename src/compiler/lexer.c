#include "lexer.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

const char* token_kind_name(token_kind_t kind) {
    switch (kind) {
        case TOK_EOF:            return "EOF";
        case TOK_INT_LITERAL:    return "INT_LITERAL";
        case TOK_FLOAT_LITERAL:  return "FLOAT_LITERAL";
        case TOK_CHAR_LITERAL:   return "CHAR_LITERAL";
        case TOK_STRING_LITERAL: return "STRING_LITERAL";
        case TOK_IDENTIFIER:     return "IDENTIFIER";
        case TOK_KW_INT:         return "int";
        case TOK_KW_FLOAT:       return "float";
        case TOK_KW_CHAR:        return "char";
        case TOK_KW_VOID:        return "void";
        case TOK_KW_STRUCT:      return "struct";
        case TOK_KW_RETURN:      return "return";
        case TOK_KW_IF:          return "if";
        case TOK_KW_ELSE:        return "else";
        case TOK_KW_WHILE:       return "while";
        case TOK_KW_FOR:         return "for";
        case TOK_LPAREN:         return "(";
        case TOK_RPAREN:         return ")";
        case TOK_LBRACE:         return "{";
        case TOK_RBRACE:         return "}";
        case TOK_LBRACKET:       return "[";
        case TOK_RBRACKET:       return "]";
        case TOK_SEMICOLON:      return ";";
        case TOK_COMMA:          return ",";
        case TOK_DOT:            return ".";
        case TOK_ARROW:          return "->";
        case TOK_ASSIGN:         return "=";
        case TOK_PLUS:           return "+";
        case TOK_MINUS:          return "-";
        case TOK_STAR:           return "*";
        case TOK_SLASH:          return "/";
        case TOK_EQ:             return "==";
        case TOK_NEQ:            return "!=";
        case TOK_LT:             return "<";
        case TOK_GT:             return ">";
        case TOK_LE:             return "<=";
        case TOK_GE:             return ">=";
        default:                 return "ERROR";
    }
}

void lexer_init(lexer_t* lexer, const char* source) {
    lexer->source = source ? source : "";
    lexer->cursor = lexer->source;
    lexer->line = 1;
    lexer->col = 1;
}

static void skip_whitespace_and_comments(lexer_t* lexer) {
    while (*lexer->cursor) {
        char c = *lexer->cursor;
        if (c == ' ' || c == '\t' || c == '\r') {
            lexer->cursor++;
            lexer->col++;
        } else if (c == '\n') {
            lexer->cursor++;
            lexer->line++;
            lexer->col = 1;
        } else if (c == '/' && *(lexer->cursor + 1) == '/') {
            /* Line comment */
            lexer->cursor += 2;
            lexer->col += 2;
            while (*lexer->cursor && *lexer->cursor != '\n') {
                lexer->cursor++;
                lexer->col++;
            }
        } else if (c == '/' && *(lexer->cursor + 1) == '*') {
            /* Block comment */
            lexer->cursor += 2;
            lexer->col += 2;
            while (*lexer->cursor && !(*lexer->cursor == '*' && *(lexer->cursor + 1) == '/')) {
                if (*lexer->cursor == '\n') {
                    lexer->line++;
                    lexer->col = 1;
                } else {
                    lexer->col++;
                }
                lexer->cursor++;
            }
            if (*lexer->cursor) {
                lexer->cursor += 2;
                lexer->col += 2;
            }
        } else {
            break;
        }
    }
}

static token_t make_token(lexer_t* lexer, token_kind_t kind, const char* start, size_t length, size_t line, size_t col) {
    (void)lexer;
    token_t tok;
    tok.kind = kind;
    tok.start = start;
    tok.length = length;
    tok.line = line;
    tok.col = col;
    tok.as.int_val = 0;
    return tok;
}

token_t lexer_next_token(lexer_t* lexer) {
    skip_whitespace_and_comments(lexer);

    size_t line = lexer->line;
    size_t col = lexer->col;
    const char* start = lexer->cursor;

    if (*start == '\0') {
        return make_token(lexer, TOK_EOF, start, 0, line, col);
    }

    /* Identifier or Keyword */
    if (isalpha((unsigned char)*start) || *start == '_') {
        while (isalnum((unsigned char)*lexer->cursor) || *lexer->cursor == '_') {
            lexer->cursor++;
            lexer->col++;
        }
        size_t len = (size_t)(lexer->cursor - start);
        token_t tok = make_token(lexer, TOK_IDENTIFIER, start, len, line, col);

        if (len < sizeof(tok.as.str_val)) {
            memcpy(tok.as.str_val, start, len);
            tok.as.str_val[len] = '\0';
        }

        /* Check keywords */
        if (len == 3 && strncmp(start, "int", 3) == 0) tok.kind = TOK_KW_INT;
        else if (len == 5 && strncmp(start, "float", 5) == 0) tok.kind = TOK_KW_FLOAT;
        else if (len == 4 && strncmp(start, "char", 4) == 0) tok.kind = TOK_KW_CHAR;
        else if (len == 4 && strncmp(start, "void", 4) == 0) tok.kind = TOK_KW_VOID;
        else if (len == 6 && strncmp(start, "struct", 6) == 0) tok.kind = TOK_KW_STRUCT;
        else if (len == 6 && strncmp(start, "return", 6) == 0) tok.kind = TOK_KW_RETURN;
        else if (len == 2 && strncmp(start, "if", 2) == 0) tok.kind = TOK_KW_IF;
        else if (len == 4 && strncmp(start, "else", 4) == 0) tok.kind = TOK_KW_ELSE;
        else if (len == 5 && strncmp(start, "while", 5) == 0) tok.kind = TOK_KW_WHILE;
        else if (len == 3 && strncmp(start, "for", 3) == 0) tok.kind = TOK_KW_FOR;

        return tok;
    }

    /* Number literal (int or float) */
    if (isdigit((unsigned char)*start)) {
        bool is_float = false;
        while (isdigit((unsigned char)*lexer->cursor)) {
            lexer->cursor++;
            lexer->col++;
        }
        if (*lexer->cursor == '.' && isdigit((unsigned char)*(lexer->cursor + 1))) {
            is_float = true;
            lexer->cursor++;
            lexer->col++;
            while (isdigit((unsigned char)*lexer->cursor)) {
                lexer->cursor++;
                lexer->col++;
            }
        }
        size_t len = (size_t)(lexer->cursor - start);
        token_t tok = make_token(lexer, is_float ? TOK_FLOAT_LITERAL : TOK_INT_LITERAL, start, len, line, col);
        if (is_float) {
            tok.as.float_val = strtod(start, NULL);
        } else {
            tok.as.int_val = strtol(start, NULL, 0);
        }
        return tok;
    }

    /* Single / Double character symbols */
    char c = *lexer->cursor++;
    lexer->col++;

    switch (c) {
        case '(': return make_token(lexer, TOK_LPAREN, start, 1, line, col);
        case ')': return make_token(lexer, TOK_RPAREN, start, 1, line, col);
        case '{': return make_token(lexer, TOK_LBRACE, start, 1, line, col);
        case '}': return make_token(lexer, TOK_RBRACE, start, 1, line, col);
        case '[': return make_token(lexer, TOK_LBRACKET, start, 1, line, col);
        case ']': return make_token(lexer, TOK_RBRACKET, start, 1, line, col);
        case ';': return make_token(lexer, TOK_SEMICOLON, start, 1, line, col);
        case ',': return make_token(lexer, TOK_COMMA, start, 1, line, col);
        case '.': return make_token(lexer, TOK_DOT, start, 1, line, col);
        case '+': return make_token(lexer, TOK_PLUS, start, 1, line, col);
        case '*': return make_token(lexer, TOK_STAR, start, 1, line, col);
        case '/': return make_token(lexer, TOK_SLASH, start, 1, line, col);
        case '-':
            if (*lexer->cursor == '>') {
                lexer->cursor++;
                lexer->col++;
                return make_token(lexer, TOK_ARROW, start, 2, line, col);
            }
            return make_token(lexer, TOK_MINUS, start, 1, line, col);
        case '=':
            if (*lexer->cursor == '=') {
                lexer->cursor++;
                lexer->col++;
                return make_token(lexer, TOK_EQ, start, 2, line, col);
            }
            return make_token(lexer, TOK_ASSIGN, start, 1, line, col);
        case '!':
            if (*lexer->cursor == '=') {
                lexer->cursor++;
                lexer->col++;
                return make_token(lexer, TOK_NEQ, start, 2, line, col);
            }
            break;
        case '<':
            if (*lexer->cursor == '=') {
                lexer->cursor++;
                lexer->col++;
                return make_token(lexer, TOK_LE, start, 2, line, col);
            }
            return make_token(lexer, TOK_LT, start, 1, line, col);
        case '>':
            if (*lexer->cursor == '=') {
                lexer->cursor++;
                lexer->col++;
                return make_token(lexer, TOK_GE, start, 2, line, col);
            }
            return make_token(lexer, TOK_GT, start, 1, line, col);
    }

    return make_token(lexer, TOK_ERROR, start, 1, line, col);
}

token_t lexer_peek_token(lexer_t* lexer) {
    lexer_t copy = *lexer;
    return lexer_next_token(&copy);
}
