#include "ast.h"
#include "../common/mem_tracker.h"
#include <stdlib.h>
#include <string.h>

ast_node_t* ast_create_node(ast_node_kind_t kind, size_t line, size_t col) {
    ast_node_t* node = (ast_node_t*)tracked_malloc(sizeof(ast_node_t));
    if (!node) return NULL;
    memset(node, 0, sizeof(ast_node_t));
    node->kind = kind;
    node->line = line;
    node->col = col;
    return node;
}

void ast_destroy_node(ast_node_t* node) {
    if (!node) return;

    switch (node->kind) {
        case AST_PROGRAM:
            if (node->as.program.items) {
                for (size_t i = 0; i < node->as.program.count; i++) {
                    ast_destroy_node(node->as.program.items[i]);
                }
                tracked_free(node->as.program.items);
            }
            break;
        case AST_VAR_DECL:
            if (node->as.var_decl.init_expr) {
                ast_destroy_node(node->as.var_decl.init_expr);
            }
            break;
        case AST_STRUCT_DECL:
            if (node->as.struct_decl.fields) {
                for (size_t i = 0; i < node->as.struct_decl.field_count; i++) {
                    ast_destroy_node(node->as.struct_decl.fields[i]);
                }
                tracked_free(node->as.struct_decl.fields);
            }
            break;
        case AST_FUNC_DECL:
            if (node->as.func_decl.params) {
                for (size_t i = 0; i < node->as.func_decl.param_count; i++) {
                    ast_destroy_node(node->as.func_decl.params[i]);
                }
                tracked_free(node->as.func_decl.params);
            }
            if (node->as.func_decl.body) {
                ast_destroy_node(node->as.func_decl.body);
            }
            break;
        case AST_BLOCK:
            if (node->as.block.statements) {
                for (size_t i = 0; i < node->as.block.count; i++) {
                    ast_destroy_node(node->as.block.statements[i]);
                }
                tracked_free(node->as.block.statements);
            }
            break;
        case AST_ASSIGN:
            ast_destroy_node(node->as.assign.target);
            ast_destroy_node(node->as.assign.expr);
            break;
        case AST_RETURN:
            if (node->as.return_stmt.expr) {
                ast_destroy_node(node->as.return_stmt.expr);
            }
            break;
        case AST_IF:
            ast_destroy_node(node->as.if_stmt.cond);
            ast_destroy_node(node->as.if_stmt.then_block);
            if (node->as.if_stmt.else_block) {
                ast_destroy_node(node->as.if_stmt.else_block);
            }
            break;
        case AST_WHILE:
            ast_destroy_node(node->as.while_stmt.cond);
            ast_destroy_node(node->as.while_stmt.body);
            break;
        case AST_UNARY_EXPR:
            ast_destroy_node(node->as.unary.operand);
            break;
        case AST_BINARY_EXPR:
            ast_destroy_node(node->as.binary.left);
            ast_destroy_node(node->as.binary.right);
            break;
        case AST_CALL_EXPR:
            if (node->as.call.args) {
                for (size_t i = 0; i < node->as.call.arg_count; i++) {
                    ast_destroy_node(node->as.call.args[i]);
                }
                tracked_free(node->as.call.args);
            }
            break;
        case AST_MEMBER_EXPR:
            ast_destroy_node(node->as.member.target);
            break;
        case AST_INDEX_EXPR:
            ast_destroy_node(node->as.index_expr.target);
            ast_destroy_node(node->as.index_expr.index);
            break;
        default:
            break;
    }

    tracked_free(node);
}
