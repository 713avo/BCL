/**
 * @file bcl_hash.c
 * @brief Implementación de hash tables (case-insensitive)
 */

#include "bcl.h"
#include <string.h>
#include <stdlib.h>

/* ========================================================================== */
/* HASH FUNCTION                                                             */
/* ========================================================================== */

static unsigned int bcl_hash_string(const char *str) {
    unsigned int hash = 5381;
    int c;

    while ((c = *str++)) {
        /* Convertir a minúscula para case-insensitive */
        c = tolower((unsigned char)c);
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }

    return hash % BCL_HASH_TABLE_SIZE;
}

/* ========================================================================== */
/* HASH TABLE                                                                */
/* ========================================================================== */

bcl_hash_table_t *bcl_hash_create(void) {
    bcl_hash_table_t *table = malloc(sizeof(bcl_hash_table_t));
    if (!table) return NULL;

    memset(table->buckets, 0, sizeof(table->buckets));
    table->count = 0;

    return table;
}

void bcl_hash_destroy(bcl_hash_table_t *table) {
    if (!table) return;

    for (size_t i = 0; i < BCL_HASH_TABLE_SIZE; i++) {
        bcl_hash_entry_t *entry = table->buckets[i];
        while (entry) {
            bcl_hash_entry_t *next = entry->next;
            free(entry->key);
            bcl_value_destroy(entry->value);
            free(entry);
            entry = next;
        }
    }

    free(table);
}

void bcl_hash_set(bcl_hash_table_t *table, const char *key, bcl_value_t *value) {
    if (!table || !key) return;

    unsigned int index = bcl_hash_string(key);
    bcl_hash_entry_t *entry = table->buckets[index];

    /* Buscar si ya existe */
    while (entry) {
        if (bcl_strcasecmp(entry->key, key) == 0) {
            /* Actualizar valor existente */
            bcl_value_destroy(entry->value);
            entry->value = value;
            return;
        }
        entry = entry->next;
    }

    /* Crear nueva entrada */
    bcl_hash_entry_t *new_entry = malloc(sizeof(bcl_hash_entry_t));
    if (!new_entry) return;

    new_entry->key = bcl_strdup(key);
    new_entry->value = value;
    new_entry->next = table->buckets[index];
    table->buckets[index] = new_entry;
    table->count++;
}

bcl_value_t *bcl_hash_get(bcl_hash_table_t *table, const char *key) {
    if (!table || !key) return NULL;

    unsigned int index = bcl_hash_string(key);
    bcl_hash_entry_t *entry = table->buckets[index];

    while (entry) {
        if (bcl_strcasecmp(entry->key, key) == 0) {
            return entry->value;
        }
        entry = entry->next;
    }

    return NULL;
}

bool bcl_hash_exists(bcl_hash_table_t *table, const char *key) {
    return bcl_hash_get(table, key) != NULL;
}

void bcl_hash_remove(bcl_hash_table_t *table, const char *key) {
    if (!table || !key) return;

    unsigned int index = bcl_hash_string(key);
    bcl_hash_entry_t *entry = table->buckets[index];
    bcl_hash_entry_t *prev = NULL;

    while (entry) {
        if (bcl_strcasecmp(entry->key, key) == 0) {
            if (prev) {
                prev->next = entry->next;
            } else {
                table->buckets[index] = entry->next;
            }

            free(entry->key);
            bcl_value_destroy(entry->value);
            free(entry);
            table->count--;
            return;
        }
        prev = entry;
        entry = entry->next;
    }
}

char **bcl_hash_keys(bcl_hash_table_t *table, size_t *count) {
    if (!table || !count) return NULL;

    *count = table->count;
    if (*count == 0) return NULL;

    char **keys = malloc(sizeof(char *) * (*count));
    if (!keys) return NULL;

    size_t idx = 0;
    for (size_t i = 0; i < BCL_HASH_TABLE_SIZE; i++) {
        bcl_hash_entry_t *entry = table->buckets[i];
        while (entry) {
            keys[idx++] = bcl_strdup(entry->key);
            entry = entry->next;
        }
    }

    return keys;
}
