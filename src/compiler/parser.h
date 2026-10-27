#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "lexer.h"
#include "../common/type_table.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lexer_t       lexer;
    token_t       current;
    token_t       peek;
    type_table_t* type_table;
    bool          had_error;
    char          error_msg[256];
} parser_t;

void        parser_init(parser_t* parser, const char* source, type_table_t* type_table);
ast_node_t* parser_parse_program(parser_t* parser);

#ifdef __cplusplus
}
#endif

#endif /* PARSER_H */
