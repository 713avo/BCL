/**
 * @file bcl_array.c
 * @brief Comando ARRAY - Manipulación de arrays asociativos
 * @note Inspirado en el comando array de Tcl
 */

#include "bcl.h"
#include "bcl_commands.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

/* ========================================================================== */
/* PATRÓN MATCHING (GLOB)                                                    */
/* ========================================================================== */

/**
 * @brief Compara un string con un patrón glob
 * Soporta: * (cero o más caracteres), ? (un carácter), [abc] (conjunto)
 */
static bool match_pattern(const char *pattern, const char *str, bool nocase) {
    while (*pattern && *str) {
        if (*pattern == '*') {
            /* Asterisco: cero o más caracteres */
            pattern++;
            if (!*pattern) return true; /* * al final coincide con todo */

            /* Probar todas las posiciones */
            while (*str) {
                if (match_pattern(pattern, str, nocase)) return true;
                str++;
            }
            return false;
        } else if (*pattern == '?') {
            /* Interrogación: exactamente un carácter */
            pattern++;
            str++;
        } else if (*pattern == '[') {
            /* Conjunto de caracteres [abc] o rango [a-z] */
            pattern++;
            bool match = false;
            bool negate = false;

            if (*pattern == '!') {
                negate = true;
                pattern++;
            }

            while (*pattern && *pattern != ']') {
                if (*(pattern + 1) == '-' && *(pattern + 2) != ']') {
                    /* Rango */
                    char start = nocase ? tolower(*pattern) : *pattern;
                    char end = nocase ? tolower(*(pattern + 2)) : *(pattern + 2);
                    char c = nocase ? tolower(*str) : *str;
                    if (c >= start && c <= end) match = true;
                    pattern += 3;
                } else {
                    /* Carácter individual */
                    char pc = nocase ? tolower(*pattern) : *pattern;
                    char sc = nocase ? tolower(*str) : *str;
                    if (pc == sc) match = true;
                    pattern++;
                }
            }
            if (*pattern == ']') pattern++;

            if (negate) match = !match;
            if (!match) return false;
            str++;
        } else if (*pattern == '\\' && *(pattern + 1)) {
            /* Escape */
            pattern++;
            char pc = nocase ? tolower(*pattern) : *pattern;
            char sc = nocase ? tolower(*str) : *str;

            if (pc != sc) return false;
            pattern++;
            str++;
        } else {
            /* Carácter literal */
            char pc = nocase ? tolower(*pattern) : *pattern;
            char sc = nocase ? tolower(*str) : *str;

            if (pc != sc) return false;
            pattern++;
            str++;
        }
    }

    /* Consumir asteriscos finales */
    while (*pattern == '*') pattern++;

    return !*pattern && !*str;
}

/* ========================================================================== */
/* ARRAY EXISTS - Verificar si una variable es un array                     */
/* ========================================================================== */

static bcl_result_t array_exists(bcl_interp_t *interp, const char *array_name,
                                  bcl_value_t **result) {
    if (!array_name) {
        bcl_set_error(interp, "ARRAY EXISTS: missing array name");
        return BCL_ERROR;
    }

    /* Buscar si existe al menos un elemento con el patrón array_name(*)  */
    bcl_hash_table_t *vars = NULL;

    /* Obtener tabla de variables del scope actual */
    if (interp->scope_depth > 0) {
        bcl_scope_t *scope = interp->scope_stack[interp->scope_depth - 1];
        vars = scope ? scope->vars : interp->global_vars;
    } else {
        vars = interp->global_vars;
    }

    if (!vars) {
        *result = bcl_value_create("0");
        return BCL_OK;
    }

    /* Construir patrón: "arrayname(" */
    char pattern[256];
    snprintf(pattern, sizeof(pattern), "%s(", array_name);
    size_t pattern_len = strlen(pattern);

    /* Iterar sobre todas las variables */
    bool found = false;
    for (size_t i = 0; i < BCL_HASH_TABLE_SIZE && !found; i++) {
        bcl_hash_entry_t *entry = vars->buckets[i];
        while (entry && !found) {
            if (bcl_strncasecmp(entry->key, pattern, pattern_len) == 0) {
                found = true;
            }
            entry = entry->next;
        }
    }

    *result = bcl_value_create(found ? "1" : "0");
    return BCL_OK;
}

/* ========================================================================== */
/* ARRAY SIZE - Obtener número de elementos en array                        */
/* ========================================================================== */

static bcl_result_t array_size(bcl_interp_t *interp, const char *array_name,
                                bcl_value_t **result) {
    if (!array_name) {
        bcl_set_error(interp, "ARRAY SIZE: missing array name");
        return BCL_ERROR;
    }

    bcl_hash_table_t *vars = NULL;

    /* Obtener tabla de variables del scope actual */
    if (interp->scope_depth > 0) {
        bcl_scope_t *scope = interp->scope_stack[interp->scope_depth - 1];
        vars = scope ? scope->vars : interp->global_vars;
    } else {
        vars = interp->global_vars;
    }

    if (!vars) {
        *result = bcl_value_create("0");
        return BCL_OK;
    }

    /* Construir patrón */
    char pattern[256];
    snprintf(pattern, sizeof(pattern), "%s(", array_name);
    size_t pattern_len = strlen(pattern);

    /* Contar elementos */
    size_t count = 0;
    for (size_t i = 0; i < BCL_HASH_TABLE_SIZE; i++) {
        bcl_hash_entry_t *entry = vars->buckets[i];
        while (entry) {
            if (bcl_strncasecmp(entry->key, pattern, pattern_len) == 0) {
                count++;
            }
            entry = entry->next;
        }
    }

    char size_str[32];
    snprintf(size_str, sizeof(size_str), "%zu", count);
    *result = bcl_value_create(size_str);
    return BCL_OK;
}

/* ========================================================================== */
/* ARRAY NAMES - Obtener nombres de índices del array                       */
/* ========================================================================== */

static bcl_result_t array_names(bcl_interp_t *interp, const char *array_name,
                                 const char *pattern, bcl_value_t **result) {
    if (!array_name) {
        bcl_set_error(interp, "ARRAY NAMES: missing array name");
        return BCL_ERROR;
    }

    bcl_hash_table_t *vars = NULL;

    /* Obtener tabla de variables del scope actual */
    if (interp->scope_depth > 0) {
        bcl_scope_t *scope = interp->scope_stack[interp->scope_depth - 1];
        vars = scope ? scope->vars : interp->global_vars;
    } else {
        vars = interp->global_vars;
    }

    if (!vars) {
        *result = bcl_value_create("");
        return BCL_OK;
    }

    /* Construir patrón de búsqueda */
    char search_pattern[256];
    snprintf(search_pattern, sizeof(search_pattern), "%s(", array_name);
    size_t pattern_len = strlen(search_pattern);

    /* Recolectar nombres */
    bcl_string_t *names = bcl_string_create_empty(256);
    if (!names) {
        bcl_set_error(interp, "Out of memory");
        return BCL_ERROR;
    }

    for (size_t i = 0; i < BCL_HASH_TABLE_SIZE; i++) {
        bcl_hash_entry_t *entry = vars->buckets[i];
        while (entry) {
            if (bcl_strncasecmp(entry->key, search_pattern, pattern_len) == 0) {
                /* Extraer el índice entre paréntesis */
                const char *start = entry->key + pattern_len;
                const char *end = strchr(start, ')');

                if (end) {
                    size_t index_len = end - start;
                    char *index = malloc(index_len + 1);
                    if (index) {
                        memcpy(index, start, index_len);
                        index[index_len] = '\0';

                        /* Verificar patrón si se especificó */
                        bool matches = true;
                        if (pattern && *pattern) {
                            matches = match_pattern(pattern, index, false);
                        }

                        if (matches) {
                            if (bcl_string_cstr(names)[0] != '\0') {
                                bcl_string_append(names, " ");
                            }
                            bcl_string_append(names, index);
                        }

                        free(index);
                    }
                }
            }
            entry = entry->next;
        }
    }

    char *final = bcl_strdup(bcl_string_cstr(names));
    bcl_string_destroy(names);
    *result = bcl_value_create(final);
    free(final);
    return BCL_OK;
}

/* ========================================================================== */
/* ARRAY GET - Obtener pares clave-valor del array                          */
/* ========================================================================== */

static bcl_result_t array_get(bcl_interp_t *interp, const char *array_name,
                               const char *pattern, bcl_value_t **result) {
    if (!array_name) {
        bcl_set_error(interp, "ARRAY GET: missing array name");
        return BCL_ERROR;
    }

    bcl_hash_table_t *vars = NULL;

    /* Obtener tabla de variables del scope actual */
    if (interp->scope_depth > 0) {
        bcl_scope_t *scope = interp->scope_stack[interp->scope_depth - 1];
        vars = scope ? scope->vars : interp->global_vars;
    } else {
        vars = interp->global_vars;
    }

    if (!vars) {
        *result = bcl_value_create("");
        return BCL_OK;
    }

    /* Construir patrón de búsqueda */
    char search_pattern[256];
    snprintf(search_pattern, sizeof(search_pattern), "%s(", array_name);
    size_t pattern_len = strlen(search_pattern);

    /* Recolectar pares índice valor */
    bcl_string_t *pairs = bcl_string_create_empty(512);
    if (!pairs) {
        bcl_set_error(interp, "Out of memory");
        return BCL_ERROR;
    }

    for (size_t i = 0; i < BCL_HASH_TABLE_SIZE; i++) {
        bcl_hash_entry_t *entry = vars->buckets[i];
        while (entry) {
            if (bcl_strncasecmp(entry->key, search_pattern, pattern_len) == 0) {
                /* Extraer el índice */
                const char *start = entry->key + pattern_len;
                const char *end = strchr(start, ')');

                if (end) {
                    size_t index_len = end - start;
                    char *index = malloc(index_len + 1);
                    if (index) {
                        memcpy(index, start, index_len);
                        index[index_len] = '\0';

                        /* Verificar patrón si se especificó */
                        bool matches = true;
                        if (pattern && *pattern) {
                            matches = match_pattern(pattern, index, false);
                        }

                        if (matches) {
                            if (bcl_string_cstr(pairs)[0] != '\0') {
                                bcl_string_append(pairs, " ");
                            }
                            bcl_string_append(pairs, index);
                            bcl_string_append(pairs, " ");
                            bcl_string_append(pairs, bcl_value_get(entry->value));
                        }

                        free(index);
                    }
                }
            }
            entry = entry->next;
        }
    }

    char *final = bcl_strdup(bcl_string_cstr(pairs));
    bcl_string_destroy(pairs);
    *result = bcl_value_create(final);
    free(final);
    return BCL_OK;
}

/* ========================================================================== */
/* ARRAY SET - Establecer array desde lista de pares                        */
/* ========================================================================== */

static bcl_result_t array_set(bcl_interp_t *interp, const char *array_name,
                               const char *list_str, bcl_value_t **result) {
    if (!array_name) {
        bcl_set_error(interp, "ARRAY SET: missing array name");
        return BCL_ERROR;
    }

    if (!list_str) {
        bcl_set_error(interp, "ARRAY SET: missing list");
        return BCL_ERROR;
    }

    /* Convertir lista a tokens */
    char *list_copy = bcl_strdup(list_str);
    if (!list_copy) {
        bcl_set_error(interp, "Out of memory");
        return BCL_ERROR;
    }

    /* Dividir en elementos */
    char **elements = NULL;
    size_t count = 0;
    size_t capacity = 16;
    elements = malloc(sizeof(char*) * capacity);
    if (!elements) {
        free(list_copy);
        bcl_set_error(interp, "Out of memory");
        return BCL_ERROR;
    }

    char *saveptr = NULL;
    char *token = strtok_r(list_copy, " \t\n", &saveptr);
    while (token) {
        if (count >= capacity) {
            capacity *= 2;
            elements = realloc(elements, sizeof(char*) * capacity);
        }
        elements[count++] = token;
        token = strtok_r(NULL, " \t\n", &saveptr);
    }

    /* Verificar que haya un número par de elementos */
    if (count % 2 != 0) {
        free(elements);
        free(list_copy);
        bcl_set_error(interp, "ARRAY SET: list must have an even number of elements");
        return BCL_ERROR;
    }

    /* Establecer elementos del array */
    for (size_t i = 0; i < count; i += 2) {
        char varname[512];
        snprintf(varname, sizeof(varname), "%s(%s)", array_name, elements[i]);

        if (bcl_var_set(interp, varname, elements[i + 1]) != BCL_OK) {
            free(elements);
            free(list_copy);
            return BCL_ERROR;
        }
    }

    free(elements);
    free(list_copy);

    *result = bcl_value_create("");
    return BCL_OK;
}

/* ========================================================================== */
/* ARRAY UNSET - Eliminar array o elementos que coincidan con patrón        */
/* ========================================================================== */

static bcl_result_t array_unset(bcl_interp_t *interp, const char *array_name,
                                 const char *pattern, bcl_value_t **result) {
    if (!array_name) {
        bcl_set_error(interp, "ARRAY UNSET: missing array name");
        return BCL_ERROR;
    }

    /* Si no hay patrón, eliminar todo el array */
    if (!pattern || !*pattern) {
        pattern = "*";
    }

    /* Recolectar nombres de variables a eliminar */
    char **to_delete = NULL;
    size_t delete_count = 0;
    size_t delete_capacity = 16;
    to_delete = malloc(sizeof(char*) * delete_capacity);
    if (!to_delete) {
        bcl_set_error(interp, "Out of memory");
        return BCL_ERROR;
    }

    bcl_hash_table_t *vars = NULL;

    /* Obtener tabla de variables del scope actual */
    if (interp->scope_depth > 0) {
        bcl_scope_t *scope = interp->scope_stack[interp->scope_depth - 1];
        vars = scope ? scope->vars : interp->global_vars;
    } else {
        vars = interp->global_vars;
    }

    if (vars) {
        char search_pattern[256];
        snprintf(search_pattern, sizeof(search_pattern), "%s(", array_name);
        size_t pattern_len = strlen(search_pattern);

        for (size_t i = 0; i < BCL_HASH_TABLE_SIZE; i++) {
            bcl_hash_entry_t *entry = vars->buckets[i];
            while (entry) {
                if (bcl_strncasecmp(entry->key, search_pattern, pattern_len) == 0) {
                    /* Extraer índice */
                    const char *start = entry->key + pattern_len;
                    const char *end = strchr(start, ')');

                    if (end) {
                        size_t index_len = end - start;
                        char *index = malloc(index_len + 1);
                        if (index) {
                            memcpy(index, start, index_len);
                            index[index_len] = '\0';

                            if (match_pattern(pattern, index, false)) {
                                if (delete_count >= delete_capacity) {
                                    delete_capacity *= 2;
                                    to_delete = realloc(to_delete, sizeof(char*) * delete_capacity);
                                }
                                to_delete[delete_count++] = bcl_strdup(entry->key);
                            }

                            free(index);
                        }
                    }
                }
                entry = entry->next;
            }
        }
    }

    /* Eliminar variables */
    for (size_t i = 0; i < delete_count; i++) {
        bcl_var_unset(interp, to_delete[i]);
        free(to_delete[i]);
    }
    free(to_delete);

    *result = bcl_value_create("");
    return BCL_OK;
}

/* ========================================================================== */
/* ARRAY - Comando principal                                                */
/* ========================================================================== */

bcl_result_t bcl_cmd_array(bcl_interp_t *interp, int argc, char **argv,
                            bcl_value_t **result) {
    if (argc < 2) {
        bcl_set_error(interp, "ARRAY: wrong # args: should be \"ARRAY option arrayName ?arg ...?\"");
        return BCL_ERROR;
    }

    const char *option = argv[0];
    const char *array_name = argv[1];

    if (bcl_strcasecmp(option, "EXISTS") == 0) {
        if (argc != 2) {
            bcl_set_error(interp, "ARRAY EXISTS: wrong # args: should be \"ARRAY EXISTS arrayName\"");
            return BCL_ERROR;
        }
        return array_exists(interp, array_name, result);

    } else if (bcl_strcasecmp(option, "SIZE") == 0) {
        if (argc != 2) {
            bcl_set_error(interp, "ARRAY SIZE: wrong # args: should be \"ARRAY SIZE arrayName\"");
            return BCL_ERROR;
        }
        return array_size(interp, array_name, result);

    } else if (bcl_strcasecmp(option, "NAMES") == 0) {
        const char *pattern = (argc > 2) ? argv[2] : NULL;
        return array_names(interp, array_name, pattern, result);

    } else if (bcl_strcasecmp(option, "GET") == 0) {
        const char *pattern = (argc > 2) ? argv[2] : NULL;
        return array_get(interp, array_name, pattern, result);

    } else if (bcl_strcasecmp(option, "SET") == 0) {
        if (argc != 3) {
            bcl_set_error(interp, "ARRAY SET: wrong # args: should be \"ARRAY SET arrayName list\"");
            return BCL_ERROR;
        }
        return array_set(interp, array_name, argv[2], result);

    } else if (bcl_strcasecmp(option, "UNSET") == 0) {
        const char *pattern = (argc > 2) ? argv[2] : NULL;
        return array_unset(interp, array_name, pattern, result);

    } else {
        bcl_set_error(interp, "ARRAY: bad option \"%s\": must be EXISTS, GET, NAMES, SET, SIZE, or UNSET", option);
        return BCL_ERROR;
    }
}
