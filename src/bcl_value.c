/**
 * @file bcl_value.c
 * @brief Implementación de valores BCL (siempre strings)
 */

#include "bcl.h"
#include <string.h>
#include <stdlib.h>

/* ========================================================================== */
/* VALORES                                                                   */
/* ========================================================================== */

bcl_value_t *bcl_value_create(const char *str) {
    bcl_value_t *val = malloc(sizeof(bcl_value_t));
    if (!val) return NULL;

    val->str.data = bcl_strdup(str ? str : "");
    if (!val->str.data) {
        free(val);
        return NULL;
    }

    val->str.len = strlen(val->str.data);
    val->str.capacity = val->str.len + 1;
    val->is_cached_number = false;
    val->cached_number = 0.0;

    return val;
}

bcl_value_t *bcl_value_create_empty(void) {
    return bcl_value_create("");
}

void bcl_value_destroy(bcl_value_t *val) {
    if (!val) return;
    free(val->str.data);
    free(val);
}

void bcl_value_set(bcl_value_t *val, const char *str) {
    if (!val) return;

    free(val->str.data);
    val->str.data = bcl_strdup(str ? str : "");
    val->str.len = strlen(val->str.data);
    val->str.capacity = val->str.len + 1;
    val->is_cached_number = false;
}

const char *bcl_value_get(bcl_value_t *val) {
    return val ? val->str.data : "";
}

double bcl_value_to_number(bcl_value_t *val, bool *ok) {
    if (!val) {
        if (ok) *ok = false;
        return 0.0;
    }

    /* Usar cache si está disponible */
    if (val->is_cached_number) {
        if (ok) *ok = true;
        return val->cached_number;
    }

    /* Convertir y cachear */
    bool conversion_ok;
    double result = bcl_str_to_number(val->str.data, &conversion_ok);

    if (conversion_ok) {
        val->is_cached_number = true;
        val->cached_number = result;
    }

    if (ok) *ok = conversion_ok;
    return result;
}

bool bcl_value_to_bool(bcl_value_t *val) {
    if (!val) return false;

    const char *str = val->str.data;

    /* Cadena vacía es false */
    if (!str || !*str) return false;

    /* "0" es false */
    if (strcmp(str, "0") == 0) return false;

    /* Todo lo demás es true */
    return true;
}

bcl_value_t *bcl_value_clone(bcl_value_t *val) {
    if (!val) return NULL;
    return bcl_value_create(val->str.data);
}
