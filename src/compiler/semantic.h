#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include "../symtab/symtab_api.h"
#include "../common/type_table.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*semantic_step_fn)(void* user_data, size_t step, size_t current_bytes);

typedef struct {
    symtab_t*     symtab;
    type_table_t* type_table;
    size_t        error_count;
    char          last_error[256];
    int           current_offset;
    bool          quiet;
    bool          dump_resolutions;
    semantic_step_fn step_callback;
    void*         step_user_data;
    size_t        current_step;
} semantic_context_t;

void semantic_init(semantic_context_t* ctx, symtab_t* symtab, type_table_t* type_table);
bool semantic_analyze(semantic_context_t* ctx, ast_node_t* program);

#ifdef __cplusplus
}
#endif

#endif /* SEMANTIC_H */
