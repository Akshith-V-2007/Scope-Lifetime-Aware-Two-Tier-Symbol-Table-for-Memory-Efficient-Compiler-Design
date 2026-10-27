#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "../src/symtab/symtab_baseline.h"
#include "../src/common/mem_tracker.h"

static void test_baseline_basic_and_shadowing(void) {
    printf("[TEST] Baseline: Basic insertion, shadowing, and scope exit...\n");
    mem_tracker_reset();

    symtab_t* tab = symtab_baseline_create();
    assert(tab != NULL);

    /* Scope 0 (Global) */
    tab->enter_scope(tab);
    assert(tab->get_scope_depth(tab) == 0);
    assert(tab->insert(tab, "g_counter", 2, KIND_VAR, 0) == true);
    assert(tab->insert(tab, "main_func", 1, KIND_FUNC, 0) == true);

    /* Lookup global */
    const symbol_record_t* sym = tab->lookup(tab, "g_counter");
    assert(sym != NULL);
    assert(strcmp(sym->name, "g_counter") == 0);
    assert(sym->offset == 0);
    assert(sym->scope_depth == 0);

    /* Scope 1 (Function body) */
    tab->enter_scope(tab);
    assert(tab->get_scope_depth(tab) == 1);
    assert(tab->insert(tab, "x", 2, KIND_VAR, 4) == true);
    assert(tab->insert(tab, "y", 3, KIND_VAR, 8) == true);

    /* Scope 2 (Inner block - shadowing x) */
    tab->enter_scope(tab);
    assert(tab->get_scope_depth(tab) == 2);
    assert(tab->insert(tab, "x", 2, KIND_VAR, 100) == true); /* Shadow x! */

    /* Verify shadowed x returns the innermost one (offset 100, depth 2) */
    sym = tab->lookup(tab, "x");
    assert(sym != NULL);
    assert(sym->offset == 100);
    assert(sym->scope_depth == 2);

    /* Redeclaration in current scope must fail */
    assert(tab->insert(tab, "x", 2, KIND_VAR, 999) == false);

    /* Exit Scope 2 */
    tab->exit_scope(tab);
    assert(tab->get_scope_depth(tab) == 1);

    /* Now x should resolve to Scope 1 (offset 4, depth 1) */
    sym = tab->lookup(tab, "x");
    assert(sym != NULL);
    assert(sym->offset == 4);
    assert(sym->scope_depth == 1);

    /* Exit Scope 1 */
    tab->exit_scope(tab);
    assert(tab->get_scope_depth(tab) == 0);

    /* x and y should no longer be visible */
    assert(tab->lookup(tab, "x") == NULL);
    assert(tab->lookup(tab, "y") == NULL);
    /* globals still visible */
    assert(tab->lookup(tab, "g_counter") != NULL);

    /* Exit Scope 0 */
    tab->exit_scope(tab);
    assert(tab->get_scope_depth(tab) == -1);

    symtab_stats_t stats;
    tab->get_stats(tab, &stats);
    assert(stats.total_inserts == 5);
    assert(stats.total_scopes_created == 3);
    assert(stats.total_scopes_destroyed == 3);

    tab->destroy(tab);
    printf("       Peak bytes tracked: %zu bytes\n", mem_tracker_get_peak());
    printf("       Current leaked bytes: %zu bytes\n", mem_tracker_get_current());
    assert(mem_tracker_get_current() == 0); /* 0 memory leaks! */
    printf("[PASS] Baseline tests passed successfully!\n\n");
}

int main(void) {
    mem_tracker_init();
    test_baseline_basic_and_shadowing();
    return 0;
}
