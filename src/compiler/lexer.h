#ifndef LEXER_H
#define LEXER_H

#include "token.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char* source;
    const char* cursor;
    size_t      line;
    size_t      col;
} lexer_t;

void    lexer_init(lexer_t* lexer, const char* source);
token_t lexer_next_token(lexer_t* lexer);
token_t lexer_peek_token(lexer_t* lexer);

#ifdef __cplusplus
}
#endif

#endif /* LEXER_H */
