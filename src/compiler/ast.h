#ifndef AST_H
#define AST_H

#include "../common/types.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    AST_PROGRAM = 0,
    AST_VAR_DECL,
    AST_PARAM_DECL,
    AST_STRUCT_DECL,
    AST_FUNC_DECL,
    AST_BLOCK,
    AST_ASSIGN,
    AST_RETURN,
    AST_IF,
    AST_WHILE,
    AST_UNARY_EXPR,
    AST_BINARY_EXPR,
    AST_IDENT_EXPR,
    AST_LITERAL_EXPR,
    AST_CALL_EXPR,
    AST_MEMBER_EXPR,
    AST_INDEX_EXPR
} ast_node_kind_t;

typedef struct ast_node ast_node_t;

struct ast_node {
    ast_node_kind_t kind;
    size_t line;
    size_t col;

    union {
        struct {
            ast_node_t** items;
            size_t count;
            size_t capacity;
        } program;

        struct {
            char name[64];
            type_id_t type_id;
            bool is_array;
            size_t array_len;
            ast_node_t* init_expr;
        } var_decl;

        struct {
            char name[64];
            type_id_t type_id;
        } param_decl;

        struct {
            char name[64];
            ast_node_t** fields;
            size_t field_count;
        } struct_decl;

        struct {
            char name[64];
            type_id_t return_type;
            ast_node_t** params;
            size_t param_count;
            ast_node_t* body;
        } func_decl;

        struct {
            ast_node_t** statements;
            size_t count;
            size_t capacity;
        } block;

        struct {
            ast_node_t* target;
            ast_node_t* expr;
        } assign;

        struct {
            ast_node_t* expr;
        } return_stmt;

        struct {
            ast_node_t* cond;
            ast_node_t* then_block;
            ast_node_t* else_block;
        } if_stmt;

        struct {
            ast_node_t* cond;
            ast_node_t* body;
        } while_stmt;

        struct {
            int op;
            ast_node_t* operand;
        } unary;

        struct {
            int op;
            ast_node_t* left;
            ast_node_t* right;
        } binary;

        struct {
            char name[64];
        } ident;

        struct {
            type_id_t type_id;
            long int_val;
            double float_val;
        } literal;

        struct {
            char callee[64];
            ast_node_t** args;
            size_t arg_count;
        } call;

        struct {
            ast_node_t* target;
            char field_name[64];
        } member;

        struct {
            ast_node_t* target;
            ast_node_t* index;
        } index_expr;
    } as;
};

ast_node_t* ast_create_node(ast_node_kind_t kind, size_t line, size_t col);
void        ast_destroy_node(ast_node_t* node);

#ifdef __cplusplus
}
#endif

#endif /* AST_H */
