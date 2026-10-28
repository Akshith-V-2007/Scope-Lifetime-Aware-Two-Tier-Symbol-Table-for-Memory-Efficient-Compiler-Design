#ifndef TOKEN_H
#define TOKEN_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TOK_EOF = 0,
    TOK_INT_LITERAL,
    TOK_FLOAT_LITERAL,
    TOK_CHAR_LITERAL,
    TOK_STRING_LITERAL,
    TOK_IDENTIFIER,

    /* Keywords */
    TOK_KW_INT,
    TOK_KW_FLOAT,
    TOK_KW_CHAR,
    TOK_KW_VOID,
    TOK_KW_STRUCT,
    TOK_KW_RETURN,
    TOK_KW_IF,
    TOK_KW_ELSE,
    TOK_KW_WHILE,
    TOK_KW_FOR,

    /* Symbols & Operators */
    TOK_LPAREN,     /* ( */
    TOK_RPAREN,     /* ) */
    TOK_LBRACE,     /* { */
    TOK_RBRACE,     /* } */
    TOK_LBRACKET,   /* [ */
    TOK_RBRACKET,   /* ] */
    TOK_SEMICOLON,  /* ; */
    TOK_COMMA,      /* , */
    TOK_DOT,        /* . */
    TOK_ARROW,      /* -> */
    TOK_ASSIGN,     /* = */
    TOK_PLUS,       /* + */
    TOK_MINUS,      /* - */
    TOK_STAR,       /* * */
    TOK_SLASH,      /* / */
    TOK_EQ,         /* == */
    TOK_NEQ,        /* != */
    TOK_LT,         /* < */
    TOK_GT,         /* > */
    TOK_LE,         /* <= */
    TOK_GE,         /* >= */
    TOK_AMPERSAND,  /* & */
    TOK_PERCENT,    /* % */

    TOK_ERROR
} token_kind_t;

typedef struct {
    token_kind_t kind;
    const char*  start;
    size_t       length;
    size_t       line;
    size_t       col;
    union {
        long   int_val;
        double float_val;
        char   str_val[128];
    } as;
} token_t;

const char* token_kind_name(token_kind_t kind);

#ifdef __cplusplus
}
#endif

#endif /* TOKEN_H */
