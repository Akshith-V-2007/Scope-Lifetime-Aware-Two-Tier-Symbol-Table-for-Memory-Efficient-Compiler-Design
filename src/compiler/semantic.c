#include "semantic.h"
#include <stdio.h>
#include <string.h>

void semantic_init(semantic_context_t* ctx, symtab_t* symtab, type_table_t* type_table) {
    memset(ctx, 0, sizeof(semantic_context_t));
    ctx->symtab = symtab;
    ctx->type_table = type_table;
    ctx->current_offset = 0;
}

static void report_error(semantic_context_t* ctx, size_t line, size_t col, const char* msg) {
    ctx->error_count++;
    snprintf(ctx->last_error, sizeof(ctx->last_error), "Line %zu:%zu Semantic Error: %s", line, col, msg);
    if (!ctx->quiet) {
        fprintf(stderr, "%s\n", ctx->last_error);
    }
}

/* Forward declarations */
static type_id_t analyze_expression(semantic_context_t* ctx, ast_node_t* expr);
static void analyze_statement(semantic_context_t* ctx, ast_node_t* stmt);

static type_id_t analyze_expression(semantic_context_t* ctx, ast_node_t* expr) {
    if (!expr) return TYPE_ID_INVALID;

    switch (expr->kind) {
        case AST_LITERAL_EXPR:
            return expr->as.literal.type_id;

        case AST_IDENT_EXPR: {
            const char* name = expr->as.ident.name;
            const symbol_record_t* sym = ctx->symtab->lookup(ctx->symtab, name);
            if (!sym) {
                char err[128];
                snprintf(err, sizeof(err), "Undeclared identifier '%s'", name);
                report_error(ctx, expr->line, expr->col, err);
                return TYPE_ID_INVALID;
            }
            if (ctx->dump_resolutions) {
                printf("[RESOLVE] %s -> type=%u, offset=%d, depth=%u, scope=%u\n",
                       name, sym->type_id, sym->offset, sym->scope_depth, sym->scope_id);
            }
            return sym->type_id;
        }

        case AST_ASSIGN: {
            type_id_t target_t = analyze_expression(ctx, expr->as.assign.target);
            type_id_t val_t = analyze_expression(ctx, expr->as.assign.expr);
            if (target_t != TYPE_ID_INVALID && val_t != TYPE_ID_INVALID) {
                if (target_t != val_t) {
                    /* Type mismatch warning/error */
                    /* Note: Mini-C allows int/float promotion or strict check */
                }
            }
            return target_t;
        }

        case AST_UNARY_EXPR:
            return analyze_expression(ctx, expr->as.unary.operand);

        case AST_BINARY_EXPR: {
            type_id_t lt = analyze_expression(ctx, expr->as.binary.left);
            type_id_t rt = analyze_expression(ctx, expr->as.binary.right);
            if (lt == TYPE_ID_INVALID || rt == TYPE_ID_INVALID) return TYPE_ID_INVALID;
            return lt;
        }

        case AST_CALL_EXPR: {
            const char* callee = expr->as.call.callee;
            const symbol_record_t* sym = ctx->symtab->lookup(ctx->symtab, callee);
            if (!sym) {
                char err[128];
                snprintf(err, sizeof(err), "Call to undeclared function '%s'", callee);
                report_error(ctx, expr->line, expr->col, err);
            }
            if (ctx->dump_resolutions && sym) {
                printf("[RESOLVE-CALL] %s -> type=%u, offset=%d, depth=%u, scope=%u\n",
                       callee, sym->type_id, sym->offset, sym->scope_depth, sym->scope_id);
            }
            for (size_t i = 0; i < expr->as.call.arg_count; i++) {
                analyze_expression(ctx, expr->as.call.args[i]);
            }
            return sym ? sym->type_id : TYPE_ID_INVALID;
        }

        case AST_INDEX_EXPR: {
            type_id_t target_t = analyze_expression(ctx, expr->as.index_expr.target);
            type_id_t idx_t = analyze_expression(ctx, expr->as.index_expr.index);
            (void)idx_t;
            const type_info_t* ti = type_table_lookup(ctx->type_table, target_t);
            if (ti && ti->kind == TYPE_ARRAY_T) {
                return ti->as.array_info.elem_type;
            }
            return target_t;
        }

        case AST_MEMBER_EXPR: {
            type_id_t target_t = analyze_expression(ctx, expr->as.member.target);
            (void)target_t;
            return type_get_primitive(ctx->type_table, TYPE_INT);
        }

        default:
            return TYPE_ID_INVALID;
    }
}

static void analyze_statement(semantic_context_t* ctx, ast_node_t* stmt) {
    if (!stmt) return;

    if (ctx->step_callback) {
        ctx->current_step++;
        ctx->step_callback(ctx->step_user_data, ctx->current_step, mem_tracker_get_current());
    }

    switch (stmt->kind) {
        case AST_VAR_DECL: {
            const char* name = stmt->as.var_decl.name;
            type_id_t tid = stmt->as.var_decl.type_id;
            if (stmt->as.var_decl.is_array) {
                tid = type_get_array(ctx->type_table, tid, stmt->as.var_decl.array_len);
            }

            int offset = ctx->current_offset;
            const type_info_t* ti = type_table_lookup(ctx->type_table, tid);
            ctx->current_offset += ti ? (int)ti->size_bytes : 4;

            bool ok = ctx->symtab->insert(ctx->symtab, name, tid, KIND_VAR, offset);
            if (!ok) {
                char err[128];
                snprintf(err, sizeof(err), "Redeclaration of symbol '%s' in the same scope", name);
                report_error(ctx, stmt->line, stmt->col, err);
            }

            if (stmt->as.var_decl.init_expr) {
                analyze_expression(ctx, stmt->as.var_decl.init_expr);
            }
            break;
        }

        case AST_BLOCK: {
            ctx->symtab->enter_scope(ctx->symtab);
            for (size_t i = 0; i < stmt->as.block.count; i++) {
                analyze_statement(ctx, stmt->as.block.statements[i]);
            }
            ctx->symtab->exit_scope(ctx->symtab);
            if (ctx->step_callback) {
                ctx->current_step++;
                ctx->step_callback(ctx->step_user_data, ctx->current_step, mem_tracker_get_current());
            }
            break;
        }

        case AST_ASSIGN:
            analyze_expression(ctx, stmt);
            break;

        case AST_RETURN:
            if (stmt->as.return_stmt.expr) {
                analyze_expression(ctx, stmt->as.return_stmt.expr);
            }
            break;

        case AST_IF:
            analyze_expression(ctx, stmt->as.if_stmt.cond);
            analyze_statement(ctx, stmt->as.if_stmt.then_block);
            if (stmt->as.if_stmt.else_block) {
                analyze_statement(ctx, stmt->as.if_stmt.else_block);
            }
            break;

        case AST_WHILE:
            analyze_expression(ctx, stmt->as.while_stmt.cond);
            analyze_statement(ctx, stmt->as.while_stmt.body);
            break;

        default:
            analyze_expression(ctx, stmt);
            break;
    }
}

bool semantic_analyze(semantic_context_t* ctx, ast_node_t* program) {
    if (!program || program->kind != AST_PROGRAM) return false;

    /* Scope 0: Global Scope */
    ctx->symtab->enter_scope(ctx->symtab);

    for (size_t i = 0; i < program->as.program.count; i++) {
        ast_node_t* item = program->as.program.items[i];
        if (!item) continue;

        if (item->kind == AST_VAR_DECL) {
            analyze_statement(ctx, item);
        } else if (item->kind == AST_STRUCT_DECL) {
            ctx->symtab->insert(ctx->symtab, item->as.struct_decl.name, TYPE_VOID, KIND_STRUCT, 0);
        } else if (item->kind == AST_FUNC_DECL) {
            /* Insert function name into global scope */
            ctx->symtab->insert(
                ctx->symtab,
                item->as.func_decl.name,
                item->as.func_decl.return_type,
                KIND_FUNC,
                0
            );

            /* Function Scope */
            ctx->symtab->enter_scope(ctx->symtab);
            int param_offset = 0;
            for (size_t p = 0; p < item->as.func_decl.param_count; p++) {
                ast_node_t* param = item->as.func_decl.params[p];
                ctx->symtab->insert(
                    ctx->symtab,
                    param->as.param_decl.name,
                    param->as.param_decl.type_id,
                    KIND_PARAM,
                    param_offset
                );
                param_offset += 4;
            }

            if (item->as.func_decl.body) {
                ast_node_t* body = item->as.func_decl.body;
                for (size_t b = 0; b < body->as.block.count; b++) {
                    analyze_statement(ctx, body->as.block.statements[b]);
                }
            }

            ctx->symtab->exit_scope(ctx->symtab);
        }
    }

    ctx->symtab->exit_scope(ctx->symtab);
    return (ctx->error_count == 0);
}
