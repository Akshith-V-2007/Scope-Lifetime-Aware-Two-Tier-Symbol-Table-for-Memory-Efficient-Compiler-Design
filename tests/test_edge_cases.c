#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../src/symtab/symtab_baseline.h"
#include "../src/symtab/symtab_twotier.h"
#include "../src/common/mem_tracker.h"

static void test_empty_scopes(void) {
    printf("[EDGE CASE] Testing empty scopes (rapid enter/exit)...\n");
    symtab_t* tab = symtab_twotier_create(2, 0);
    tab->enter_scope(tab);
    for (int i = 0; i < 50; i++) {
        tab->enter_scope(tab);
        tab->exit_scope(tab);
    }
    tab->exit_scope(tab);
    tab->destroy(tab);
    printf("            Passed empty scopes test.\n");
}

static void test_one_symbol_per_scope(void) {
    printf("[EDGE CASE] Testing 1 symbol per scope across 40 scopes...\n");
    symtab_t* tab = symtab_twotier_create(2, 0);
    tab->enter_scope(tab);

    char name[32];
    for (int i = 0; i < 40; i++) {
        tab->enter_scope(tab);
        snprintf(name, sizeof(name), "var_scope_%d", i);
        assert(tab->insert(tab, name, 2, KIND_VAR, i * 4) == true);
    }

    /* Lookup all 40 symbols from the deepest scope */
    for (int i = 0; i < 40; i++) {
        snprintf(name, sizeof(name), "var_scope_%d", i);
        const symbol_record_t* rec = tab->lookup(tab, name);
        assert(rec != NULL);
        assert(rec->offset == i * 4);
    }

    /* Exit all scopes */
    for (int i = 0; i < 40; i++) {
        tab->exit_scope(tab);
    }
    tab->exit_scope(tab);
    tab->destroy(tab);
    printf("            Passed 1 symbol per scope test.\n");
}

static void test_thousands_of_symbols_in_one_scope(void) {
    printf("[EDGE CASE] Testing 5,000 symbols in a single scope...\n");
    symtab_t* tab = symtab_twotier_create(2, 0);
    tab->enter_scope(tab);

    char name[32];
    for (int i = 0; i < 5000; i++) {
        snprintf(name, sizeof(name), "sym_%d", i);
        assert(tab->insert(tab, name, 2, KIND_VAR, i * 4) == true);
    }

    /* Demote to cold by pushing deeper scopes */
    tab->enter_scope(tab);
    tab->enter_scope(tab);

    /* Lookup from cold tier */
    for (int i = 0; i < 5000; i += 50) {
        snprintf(name, sizeof(name), "sym_%d", i);
        const symbol_record_t* rec = tab->lookup(tab, name);
        assert(rec != NULL);
        assert(rec->offset == i * 4);
    }

    tab->exit_scope(tab);
    tab->exit_scope(tab);
    tab->exit_scope(tab);
    tab->destroy(tab);
    printf("            Passed 5,000 symbols in single scope test.\n");
}

static void test_deep_nesting_and_same_name_shadowing(void) {
    printf("[EDGE CASE] Testing 64-level deep nesting with identical shadowed name at every level...\n");
    symtab_t* base = symtab_baseline_create();
    symtab_t* two = symtab_twotier_create(2, 0);

    base->enter_scope(base);
    two->enter_scope(two);

    /* Insert 'shadow_var' at 64 levels with distinct offsets */
    for (int d = 1; d <= 64; d++) {
        base->enter_scope(base);
        two->enter_scope(two);

        assert(base->insert(base, "shadow_var", 2, KIND_VAR, d * 10) == true);
        assert(two->insert(two, "shadow_var", 2, KIND_VAR, d * 10) == true);

        /* Lookup at current depth must always yield current depth's symbol */
        const symbol_record_t* b_rec = base->lookup(base, "shadow_var");
        const symbol_record_t* t_rec = two->lookup(two, "shadow_var");
        assert(b_rec != NULL && t_rec != NULL);
        assert(b_rec->offset == d * 10);
        assert(t_rec->offset == d * 10);
        assert(t_rec->scope_depth == (uint32_t)d);
    }

    /* Unwind scopes one by one, checking that the shadowed symbol unshadows correctly */
    for (int d = 64; d >= 1; d--) {
        const symbol_record_t* b_rec = base->lookup(base, "shadow_var");
        const symbol_record_t* t_rec = two->lookup(two, "shadow_var");
        assert(b_rec != NULL && t_rec != NULL);
        assert(b_rec->offset == d * 10);
        assert(t_rec->offset == d * 10);

        base->exit_scope(base);
        two->exit_scope(two);
    }

    /* In global scope, shadow_var was not declared */
    assert(base->lookup(base, "shadow_var") == NULL);
    assert(two->lookup(two, "shadow_var") == NULL);

    base->exit_scope(base);
    two->exit_scope(two);

    base->destroy(base);
    two->destroy(two);
    printf("            Passed 64-level deep nesting & shadowing test.\n");
}

static void test_very_long_identifier_names(void) {
    printf("[EDGE CASE] Testing very long identifier names (256+ characters)...\n");
    symtab_t* tab = symtab_twotier_create(2, 0);
    tab->enter_scope(tab);

    char long_name[300];
    memset(long_name, 'a', 260);
    long_name[260] = '\0';
    strcat(long_name, "_variable_suffix_2026");

    assert(tab->insert(tab, long_name, 2, KIND_VAR, 1024) == true);
    const symbol_record_t* rec = tab->lookup(tab, long_name);
    assert(rec != NULL);
    assert(strcmp(rec->name, long_name) == 0);
    assert(rec->offset == 1024);

    tab->exit_scope(tab);
    tab->destroy(tab);
    printf("            Passed very long identifier names test.\n");
}

int main(void) {
    mem_tracker_init();
    test_empty_scopes();
    printf("After empty: %zu bytes\n", mem_tracker_get_current()); fflush(stdout);
    test_one_symbol_per_scope();
    printf("After one sym: %zu bytes\n", mem_tracker_get_current()); fflush(stdout);
    test_thousands_of_symbols_in_one_scope();
    printf("After 5000: %zu bytes\n", mem_tracker_get_current()); fflush(stdout);
    test_deep_nesting_and_same_name_shadowing();
    printf("After deep: %zu bytes\n", mem_tracker_get_current()); fflush(stdout);
    test_very_long_identifier_names();
    printf("After long names: %zu bytes\n", mem_tracker_get_current()); fflush(stdout);

    assert(mem_tracker_get_current() == 0);
    printf("\n[SUCCESS] All edge case tests passed with 0 memory leaks!\n");
    return 0;
}
