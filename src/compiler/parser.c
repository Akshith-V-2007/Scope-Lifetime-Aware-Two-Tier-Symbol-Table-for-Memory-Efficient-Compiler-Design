#include "parser.h"
#include "../common/mem_tracker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void advance(parser_t* parser) {
    parser->current = parser->peek;
    parser->peek = lexer_next_token(&parser->lexer);
}

static bool check(parser_t* parser, token_kind_t kind) {
    return parser->current.kind == kind;
}

static bool match(parser_t* parser, token_kind_t kind) {
    if (check(parser, kind)) {
        advance(parser);
        return true;
    }
    return false;
}

static void error_at_current(parser_t* parser, const char* message) {
    if (parser->had_error) return;
    parser->had_error = true;
    snprintf(parser->error_msg, sizeof(parser->error_msg),
             "Line %zu:%zu Error: %s at '%.*s'",
             parser->current.line, parser->current.col,
             message, (int)parser->current.length, parser->current.start);
}

static bool consume(parser_t* parser, token_kind_t kind, const char* message) {
    if (check(parser, kind)) {
        advance(parser);
        return true;
    }
    error_at_current(parser, message);
    return false;
}

static void safe_copy_str(char* dest, const char* src, size_t dest_size) {
    if (!dest || dest_size == 0) return;
    if (!src) {
        dest[0] = '\0';
        return;
    }
    size_t len = strlen(src);
    if (len >= dest_size) {
        len = dest_size - 1;
    }
    memcpy(dest, src, len);
    dest[len] = '\0';
}

static bool consume_identifier(parser_t* parser, char* out_name, size_t max_len, const char* message) {
    if (check(parser, TOK_IDENTIFIER)) {
        safe_copy_str(out_name, parser->current.as.str_val, max_len);
        advance(parser);
        return true;
    }
    error_at_current(parser, message);
    return false;
}

/* Forward declarations */
static ast_node_t* parse_statement(parser_t* parser);
static ast_node_t* parse_block(parser_t* parser);
static ast_node_t* parse_expression(parser_t* parser);

static type_id_t parse_type_specifier(parser_t* parser) {
    type_id_t tid = TYPE_VOID;
    if (match(parser, TOK_KW_INT)) {
        tid = type_get_primitive(parser->type_table, TYPE_INT);
    } else if (match(parser, TOK_KW_FLOAT)) {
        tid = type_get_primitive(parser->type_table, TYPE_FLOAT);
    } else if (match(parser, TOK_KW_CHAR)) {
        tid = type_get_primitive(parser->type_table, TYPE_CHAR);
    } else if (match(parser, TOK_KW_VOID)) {
        tid = type_get_primitive(parser->type_table, TYPE_VOID);
    } else if (match(parser, TOK_KW_STRUCT)) {
        if (check(parser, TOK_IDENTIFIER)) {
            /* struct name reference */
            advance(parser);
            /* Return int or default struct type */
            tid = type_get_primitive(parser->type_table, TYPE_INT);
        }
    } else {
        return TYPE_ID_INVALID;
    }

    /* Pointers: int*, float**, etc. */
    while (match(parser, TOK_STAR)) {
        tid = type_get_pointer(parser->type_table, tid);
    }

    return tid;
}

static ast_node_t* parse_primary(parser_t* parser) {
    size_t line = parser->current.line;
    size_t col = parser->current.col;

    if (match(parser, TOK_INT_LITERAL)) {
        ast_node_t* node = ast_create_node(AST_LITERAL_EXPR, line, col);
        node->as.literal.type_id = type_get_primitive(parser->type_table, TYPE_INT);
        node->as.literal.int_val = parser->current.as.int_val;
        return node;
    }

    if (match(parser, TOK_FLOAT_LITERAL)) {
        ast_node_t* node = ast_create_node(AST_LITERAL_EXPR, line, col);
        node->as.literal.type_id = type_get_primitive(parser->type_table, TYPE_FLOAT);
        node->as.literal.float_val = parser->current.as.float_val;
        return node;
    }

    if (check(parser, TOK_IDENTIFIER)) {
        char name[64] = {0};
        safe_copy_str(name, parser->current.as.str_val, sizeof(name));
        advance(parser);

        /* Function Call */
        if (match(parser, TOK_LPAREN)) {
            ast_node_t* call = ast_create_node(AST_CALL_EXPR, line, col);
            safe_copy_str(call->as.call.callee, name, sizeof(call->as.call.callee));
            call->as.call.args = (ast_node_t**)tracked_malloc(sizeof(ast_node_t*) * 16);
            call->as.call.arg_count = 0;

            if (!check(parser, TOK_RPAREN)) {
                do {
                    ast_node_t* arg = parse_expression(parser);
                    if (arg && call->as.call.arg_count < 16) {
                        call->as.call.args[call->as.call.arg_count++] = arg;
                    }
                } while (match(parser, TOK_COMMA));
            }
            consume(parser, TOK_RPAREN, "Expected ')' after arguments");
            return call;
        }

        ast_node_t* ident = ast_create_node(AST_IDENT_EXPR, line, col);
        safe_copy_str(ident->as.ident.name, name, sizeof(ident->as.ident.name));
        return ident;
    }

    if (match(parser, TOK_LPAREN)) {
        ast_node_t* expr = parse_expression(parser);
        consume(parser, TOK_RPAREN, "Expected ')' after grouping expression");
        return expr;
    }

    error_at_current(parser, "Expected expression");
    advance(parser);
    return NULL;
}

static ast_node_t* parse_postfix(parser_t* parser) {
    ast_node_t* expr = parse_primary(parser);
    if (!expr) return NULL;

    while (true) {
        if (match(parser, TOK_LBRACKET)) {
            ast_node_t* index = parse_expression(parser);
            consume(parser, TOK_RBRACKET, "Expected ']' after array index");
            ast_node_t* idx_node = ast_create_node(AST_INDEX_EXPR, expr->line, expr->col);
            idx_node->as.index_expr.target = expr;
            idx_node->as.index_expr.index = index;
            expr = idx_node;
        } else if (match(parser, TOK_DOT)) {
            char mname[64] = {0};
            if (consume_identifier(parser, mname, sizeof(mname), "Expected member name after '.'")) {
                ast_node_t* member = ast_create_node(AST_MEMBER_EXPR, expr->line, expr->col);
                member->as.member.target = expr;
                safe_copy_str(member->as.member.field_name, mname, sizeof(member->as.member.field_name));
                expr = member;
            }
        } else {
            break;
        }
    }

    return expr;
}

static ast_node_t* parse_unary(parser_t* parser) {
    if (check(parser, TOK_MINUS) || check(parser, TOK_PLUS)) {
        token_kind_t op = parser->current.kind;
        size_t line = parser->current.line;
        size_t col = parser->current.col;
        advance(parser);
        ast_node_t* operand = parse_unary(parser);
        if (!operand) return NULL;
        if (operand->kind == AST_LITERAL_EXPR && op == TOK_MINUS) {
            operand->as.literal.int_val = -operand->as.literal.int_val;
            operand->as.literal.float_val = -operand->as.literal.float_val;
            return operand;
        } else if (operand->kind == AST_LITERAL_EXPR && op == TOK_PLUS) {
            return operand;
        }
        ast_node_t* u = ast_create_node(AST_UNARY_EXPR, line, col);
        u->as.unary.op = (int)op;
        u->as.unary.operand = operand;
        return u;
    }
    return parse_postfix(parser);
}

static ast_node_t* parse_multiplicative(parser_t* parser) {
    ast_node_t* expr = parse_unary(parser);
    while (check(parser, TOK_STAR) || check(parser, TOK_SLASH)) {
        token_kind_t op = parser->current.kind;
        advance(parser);
        ast_node_t* right = parse_unary(parser);
        ast_node_t* bin = ast_create_node(AST_BINARY_EXPR, expr->line, expr->col);
        bin->as.binary.op = (int)op;
        bin->as.binary.left = expr;
        bin->as.binary.right = right;
        expr = bin;
    }
    return expr;
}

static ast_node_t* parse_additive(parser_t* parser) {
    ast_node_t* expr = parse_multiplicative(parser);
    while (check(parser, TOK_PLUS) || check(parser, TOK_MINUS)) {
        token_kind_t op = parser->current.kind;
        advance(parser);
        ast_node_t* right = parse_multiplicative(parser);
        ast_node_t* bin = ast_create_node(AST_BINARY_EXPR, expr->line, expr->col);
        bin->as.binary.op = (int)op;
        bin->as.binary.left = expr;
        bin->as.binary.right = right;
        expr = bin;
    }
    return expr;
}

static ast_node_t* parse_relational(parser_t* parser) {
    ast_node_t* expr = parse_additive(parser);
    while (check(parser, TOK_LT) || check(parser, TOK_GT) ||
           check(parser, TOK_LE) || check(parser, TOK_GE) ||
           check(parser, TOK_EQ) || check(parser, TOK_NEQ)) {
        token_kind_t op = parser->current.kind;
        advance(parser);
        ast_node_t* right = parse_additive(parser);
        ast_node_t* bin = ast_create_node(AST_BINARY_EXPR, expr->line, expr->col);
        bin->as.binary.op = (int)op;
        bin->as.binary.left = expr;
        bin->as.binary.right = right;
        expr = bin;
    }
    return expr;
}

static ast_node_t* parse_expression(parser_t* parser) {
    ast_node_t* left = parse_relational(parser);
    if (!left) return NULL;

    if (match(parser, TOK_ASSIGN)) {
        ast_node_t* right = parse_expression(parser);
        ast_node_t* assign = ast_create_node(AST_ASSIGN, left->line, left->col);
        assign->as.assign.target = left;
        assign->as.assign.expr = right;
        return assign;
    }

    return left;
}

static ast_node_t* parse_block(parser_t* parser) {
    size_t line = parser->current.line;
    size_t col = parser->current.col;

    if (!consume(parser, TOK_LBRACE, "Expected '{' to start block")) {
        return NULL;
    }

    ast_node_t* block = ast_create_node(AST_BLOCK, line, col);
    block->as.block.capacity = 32;
    block->as.block.count = 0;
    block->as.block.statements = (ast_node_t**)tracked_malloc(sizeof(ast_node_t*) * block->as.block.capacity);

    while (!check(parser, TOK_RBRACE) && !check(parser, TOK_EOF)) {
        ast_node_t* stmt = parse_statement(parser);
        if (stmt) {
            if (block->as.block.count >= block->as.block.capacity) {
                block->as.block.capacity *= 2;
                block->as.block.statements = (ast_node_t**)tracked_realloc(
                    block->as.block.statements,
                    sizeof(ast_node_t*) * block->as.block.capacity
                );
            }
            block->as.block.statements[block->as.block.count++] = stmt;
        }
    }

    consume(parser, TOK_RBRACE, "Expected '}' to close block");
    return block;
}

static ast_node_t* parse_statement(parser_t* parser) {
    size_t line = parser->current.line;
    size_t col = parser->current.col;

    /* Nested Compound Block */
    if (check(parser, TOK_LBRACE)) {
        return parse_block(parser);
    }

    /* Variable Declaration */
    type_id_t tid = parse_type_specifier(parser);
    if (tid != TYPE_ID_INVALID) {
        char vname[64] = {0};
        if (!consume_identifier(parser, vname, sizeof(vname), "Expected variable name")) return NULL;

        ast_node_t* decl = ast_create_node(AST_VAR_DECL, line, col);
        safe_copy_str(decl->as.var_decl.name, vname, sizeof(decl->as.var_decl.name));
        decl->as.var_decl.type_id = tid;

        /* Array declaration: int arr[10]; */
        if (match(parser, TOK_LBRACKET)) {
            if (match(parser, TOK_INT_LITERAL)) {
                decl->as.var_decl.is_array = true;
                decl->as.var_decl.array_len = (size_t)parser->current.as.int_val;
            }
            consume(parser, TOK_RBRACKET, "Expected ']' after array size");
        }

        /* Optional initializer: int x = 42; */
        if (match(parser, TOK_ASSIGN)) {
            decl->as.var_decl.init_expr = parse_expression(parser);
        }

        consume(parser, TOK_SEMICOLON, "Expected ';' after declaration");
        return decl;
    }

    /* Return statement */
    if (match(parser, TOK_KW_RETURN)) {
        ast_node_t* ret = ast_create_node(AST_RETURN, line, col);
        if (!check(parser, TOK_SEMICOLON)) {
            ret->as.return_stmt.expr = parse_expression(parser);
        }
        consume(parser, TOK_SEMICOLON, "Expected ';' after return");
        return ret;
    }

    /* If statement */
    if (match(parser, TOK_KW_IF)) {
        consume(parser, TOK_LPAREN, "Expected '(' after 'if'");
        ast_node_t* cond = parse_expression(parser);
        consume(parser, TOK_RPAREN, "Expected ')' after condition");
        ast_node_t* then_b = parse_statement(parser);
        ast_node_t* else_b = NULL;
        if (match(parser, TOK_KW_ELSE)) {
            else_b = parse_statement(parser);
        }

        ast_node_t* if_node = ast_create_node(AST_IF, line, col);
        if_node->as.if_stmt.cond = cond;
        if_node->as.if_stmt.then_block = then_b;
        if_node->as.if_stmt.else_block = else_b;
        return if_node;
    }

    /* While statement */
    if (match(parser, TOK_KW_WHILE)) {
        consume(parser, TOK_LPAREN, "Expected '(' after 'while'");
        ast_node_t* cond = parse_expression(parser);
        consume(parser, TOK_RPAREN, "Expected ')' after condition");
        ast_node_t* body = parse_statement(parser);

        ast_node_t* while_node = ast_create_node(AST_WHILE, line, col);
        while_node->as.while_stmt.cond = cond;
        while_node->as.while_stmt.body = body;
        return while_node;
    }

    /* Expression statement */
    ast_node_t* expr = parse_expression(parser);
    consume(parser, TOK_SEMICOLON, "Expected ';' after expression");
    return expr;
}

static ast_node_t* parse_top_level(parser_t* parser) {
    size_t line = parser->current.line;
    size_t col = parser->current.col;

    /* Struct definition: struct Point { int x; int y; }; */
    if (check(parser, TOK_KW_STRUCT) && parser->peek.kind == TOK_IDENTIFIER) {
        lexer_t snap = parser->lexer;
        token_t p1 = parser->current;
        token_t p2 = parser->peek;
        advance(parser); /* 'struct' */
        char sname[64] = {0};
        safe_copy_str(sname, parser->current.as.str_val, sizeof(sname));
        advance(parser); /* name */
        if (check(parser, TOK_LBRACE)) {
            advance(parser); /* '{' */

            ast_node_t* sdecl = ast_create_node(AST_STRUCT_DECL, line, col);
            safe_copy_str(sdecl->as.struct_decl.name, sname, sizeof(sdecl->as.struct_decl.name));
            sdecl->as.struct_decl.fields = (ast_node_t**)tracked_malloc(sizeof(ast_node_t*) * 16);
            sdecl->as.struct_decl.field_count = 0;

            while (!check(parser, TOK_RBRACE) && !check(parser, TOK_EOF)) {
                type_id_t ftid = parse_type_specifier(parser);
                char fname[64] = {0};
                if (consume_identifier(parser, fname, sizeof(fname), "Expected field name")) {
                    ast_node_t* f = ast_create_node(AST_VAR_DECL, line, col);
                    safe_copy_str(f->as.var_decl.name, fname, sizeof(f->as.var_decl.name));
                    f->as.var_decl.type_id = ftid;
                    consume(parser, TOK_SEMICOLON, "Expected ';' after struct field");
                    sdecl->as.struct_decl.fields[sdecl->as.struct_decl.field_count++] = f;
                }
            }
            consume(parser, TOK_RBRACE, "Expected '}' after struct fields");
            consume(parser, TOK_SEMICOLON, "Expected ';' after struct declaration");
            return sdecl;
        } else {
            /* Backtrack */
            parser->lexer = snap;
            parser->current = p1;
            parser->peek = p2;
        }
    }

    type_id_t tid = parse_type_specifier(parser);
    if (tid == TYPE_ID_INVALID) {
        error_at_current(parser, "Expected top-level declaration");
        advance(parser);
        return NULL;
    }

    char name[64] = {0};
    if (!consume_identifier(parser, name, sizeof(name), "Expected identifier after type")) {
        return NULL;
    }

    /* Function Declaration: int foo(...) { ... } */
    if (match(parser, TOK_LPAREN)) {
        ast_node_t* fdecl = ast_create_node(AST_FUNC_DECL, line, col);
        safe_copy_str(fdecl->as.func_decl.name, name, sizeof(fdecl->as.func_decl.name));
        fdecl->as.func_decl.return_type = tid;
        fdecl->as.func_decl.params = (ast_node_t**)tracked_malloc(sizeof(ast_node_t*) * 8);
        fdecl->as.func_decl.param_count = 0;

        if (!check(parser, TOK_RPAREN)) {
            do {
                type_id_t ptid = parse_type_specifier(parser);
                char pname[64] = {0};
                if (consume_identifier(parser, pname, sizeof(pname), "Expected parameter name")) {
                    ast_node_t* param = ast_create_node(AST_PARAM_DECL, line, col);
                    safe_copy_str(param->as.param_decl.name, pname, sizeof(param->as.param_decl.name));
                    param->as.param_decl.type_id = ptid;
                    fdecl->as.func_decl.params[fdecl->as.func_decl.param_count++] = param;
                }
            } while (match(parser, TOK_COMMA));
        }
        consume(parser, TOK_RPAREN, "Expected ')' after parameters");

        /* Function Body */
        fdecl->as.func_decl.body = parse_block(parser);
        return fdecl;
    }

    /* Global Variable Declaration */
    ast_node_t* gvar = ast_create_node(AST_VAR_DECL, line, col);
    safe_copy_str(gvar->as.var_decl.name, name, sizeof(gvar->as.var_decl.name));
    gvar->as.var_decl.type_id = tid;

    if (match(parser, TOK_LBRACKET)) {
        if (match(parser, TOK_INT_LITERAL)) {
            gvar->as.var_decl.is_array = true;
            gvar->as.var_decl.array_len = (size_t)parser->current.as.int_val;
        }
        consume(parser, TOK_RBRACKET, "Expected ']' after array size");
    }

    if (match(parser, TOK_ASSIGN)) {
        gvar->as.var_decl.init_expr = parse_expression(parser);
    }
    consume(parser, TOK_SEMICOLON, "Expected ';' after global declaration");
    return gvar;
}

void parser_init(parser_t* parser, const char* source, type_table_t* type_table) {
    memset(parser, 0, sizeof(parser_t));
    lexer_init(&parser->lexer, source);
    parser->type_table = type_table;
    advance(parser);
    advance(parser);
}

ast_node_t* parser_parse_program(parser_t* parser) {
    ast_node_t* prog = ast_create_node(AST_PROGRAM, 1, 1);
    prog->as.program.capacity = 32;
    prog->as.program.count = 0;
    prog->as.program.items = (ast_node_t**)tracked_malloc(sizeof(ast_node_t*) * prog->as.program.capacity);

    while (!check(parser, TOK_EOF) && !parser->had_error) {
        ast_node_t* item = parse_top_level(parser);
        if (item) {
            if (prog->as.program.count >= prog->as.program.capacity) {
                prog->as.program.capacity *= 2;
                prog->as.program.items = (ast_node_t**)tracked_realloc(
                    prog->as.program.items,
                    sizeof(ast_node_t*) * prog->as.program.capacity
                );
            }
            prog->as.program.items[prog->as.program.count++] = item;
        }
    }

    return prog;
}
