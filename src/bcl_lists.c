/**
 * @file bcl_lists.c
 * @brief Implementación de comandos de listas de BCL
 *
 * En BCL, las listas son strings con elementos separados por espacios.
 * Elementos con espacios van entre comillas.
 */

#include "bcl.h"
#include "bcl_commands.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

/* ========================================================================== */
/* UTILIDADES INTERNAS PARA LISTAS                                          */
/* ========================================================================== */

/**
 * @brief Cuenta el número de elementos en una lista
 */
static int list_count_elements(const char *list) {
    if (!list || !*list) return 0;

    int count = 0;
    const char *p = list;

    while (*p) {
        /* Saltar espacios iniciales */
        while (*p && isspace(*p)) p++;
        if (!*p) break;

        count++;

        /* Saltar el elemento */
        if (*p == '"') {
            /* Elemento entre comillas */
            p++;
            while (*p && *p != '"') {
                if (*p == '\\' && *(p+1)) p++; /* Escape */
                p++;
            }
            if (*p == '"') p++;
        } else {
            /* Elemento sin comillas */
            while (*p && !isspace(*p)) p++;
        }
    }

    return count;
}

/**
 * @brief Obtiene el n-ésimo elemento de una lista (índice basado en 0)
 * @return NULL si índice fuera de rango, o nuevo string asignado con malloc
 */
static char *list_get_element(const char *list, int index) {
    if (!list || index < 0) return NULL;

    const char *p = list;
    int current = 0;

    while (*p) {
        /* Saltar espacios */
        while (*p && isspace(*p)) p++;
        if (!*p) break;

        const char *start = p;

        /* Determinar final del elemento */
        if (*p == '"') {
            /* Elemento entre comillas */
            start++;  /* Saltar comilla inicial */
            p++;
            while (*p && *p != '"') {
                if (*p == '\\' && *(p+1)) p++;
                p++;
            }

            if (current == index) {
                /* Este es el elemento que buscamos */
                size_t len = p - start;
                char *elem = malloc(len + 1);
                if (!elem) return NULL;
                strncpy(elem, start, len);
                elem[len] = '\0';
                return elem;
            }

            if (*p == '"') p++;
        } else {
            /* Elemento sin comillas */
            while (*p && !isspace(*p)) p++;

            if (current == index) {
                size_t len = p - start;
                char *elem = malloc(len + 1);
                if (!elem) return NULL;
                strncpy(elem, start, len);
                elem[len] = '\0';
                return elem;
            }
        }

        current++;
    }

    return NULL;  /* Índice fuera de rango */
}

/**
 * @brief Construye una lista a partir de elementos individuales
 */
static char *list_build(int count, char **elements) {
    if (count <= 0) return bcl_strdup("");

    /* Calcular tamaño necesario */
    size_t total_len = 0;
    for (int i = 0; i < count; i++) {
        const char *elem = elements[i];
        bool needs_quotes = false;

        /* Ver si necesita comillas (contiene espacios) */
        for (const char *p = elem; *p; p++) {
            if (isspace(*p)) {
                needs_quotes = true;
                break;
            }
        }

        if (needs_quotes) {
            total_len += strlen(elem) + 2;  /* elemento + "" */
        } else {
            total_len += strlen(elem);
        }

        if (i < count - 1) total_len++;  /* espacio separador */
    }

    /* Construir lista */
    char *result = malloc(total_len + 1);
    if (!result) return NULL;

    char *p = result;
    for (int i = 0; i < count; i++) {
        const char *elem = elements[i];
        bool needs_quotes = false;

        for (const char *q = elem; *q; q++) {
            if (isspace(*q)) {
                needs_quotes = true;
                break;
            }
        }

        if (needs_quotes) {
            *p++ = '"';
            strcpy(p, elem);
            p += strlen(elem);
            *p++ = '"';
        } else {
            strcpy(p, elem);
            p += strlen(elem);
        }

        if (i < count - 1) *p++ = ' ';
    }
    *p = '\0';

    return result;
}

/* ========================================================================== */
/* LIST - Crear lista                                                        */
/* ========================================================================== */

bcl_result_t bcl_cmd_list(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result) {
    /* LIST puede recibir 0 o más argumentos */
    if (argc == 0) {
        *result = bcl_value_create("");
        return BCL_OK;
    }

    /* Construir lista con los argumentos */
    char *list_str = list_build(argc, argv);
    if (!list_str) {
        bcl_set_error(interp, "LIST: out of memory");
        return BCL_ERROR;
    }

    *result = bcl_value_create(list_str);
    free(list_str);
    return BCL_OK;
}

/* ========================================================================== */
/* LLENGTH - Longitud de lista                                               */
/* ========================================================================== */

bcl_result_t bcl_cmd_llength(bcl_interp_t *interp, int argc, char **argv,
                             bcl_value_t **result) {
    if (argc != 1) {
        bcl_set_error(interp, "LLENGTH: wrong # args: should be \"LLENGTH list\"");
        return BCL_ERROR;
    }

    int count = list_count_elements(argv[0]);

    char buf[32];
    snprintf(buf, sizeof(buf), "%d", count);
    *result = bcl_value_create(buf);
    return BCL_OK;
}

/* ========================================================================== */
/* LINDEX - Obtener elemento de lista                                        */
/* ========================================================================== */

bcl_result_t bcl_cmd_lindex(bcl_interp_t *interp, int argc, char **argv,
                            bcl_value_t **result) {
    if (argc != 2) {
        bcl_set_error(interp, "LINDEX: wrong # args: should be \"LINDEX list index\"");
        return BCL_ERROR;
    }

    const char *list = argv[0];
    const char *index_str = argv[1];

    /* Convertir índice a número */
    bool ok;
    double idx_d = bcl_str_to_number(index_str, &ok);
    if (!ok) {
        bcl_set_error(interp, "LINDEX: bad index \"%s\": must be integer", index_str);
        return BCL_ERROR;
    }

    int index = (int)idx_d;

    /* Obtener elemento */
    char *elem = list_get_element(list, index);
    if (!elem) {
        /* Índice fuera de rango -> retornar string vacío */
        *result = bcl_value_create("");
    } else {
        *result = bcl_value_create(elem);
        free(elem);
    }

    return BCL_OK;
}

/* ========================================================================== */
/* LAPPEND - Añadir elementos a lista                                        */
/* ========================================================================== */

bcl_result_t bcl_cmd_lappend(bcl_interp_t *interp, int argc, char **argv,
                             bcl_value_t **result) {
    if (argc < 2) {
        bcl_set_error(interp, "LAPPEND: wrong # args: should be \"LAPPEND varName element ?element...?\"");
        return BCL_ERROR;
    }

    const char *varname = argv[0];

    /* Obtener valor actual de la variable */
    bcl_value_t *var_val = bcl_var_get(interp, varname);
    const char *list = var_val ? bcl_value_get(var_val) : "";

    /* Contar elementos actuales de la lista */
    int old_count = list_count_elements(list);
    int new_count = old_count + (argc - 1);

    /* Crear array con todos los elementos */
    char **all_elements = malloc(sizeof(char*) * new_count);
    if (!all_elements) {
        bcl_set_error(interp, "LAPPEND: out of memory");
        return BCL_ERROR;
    }

    /* Extraer elementos existentes */
    for (int i = 0; i < old_count; i++) {
        all_elements[i] = list_get_element(list, i);
        if (!all_elements[i]) {
            /* Liberar lo ya asignado */
            for (int j = 0; j < i; j++) free(all_elements[j]);
            free(all_elements);
            bcl_set_error(interp, "LAPPEND: internal error");
            return BCL_ERROR;
        }
    }

    /* Añadir nuevos elementos */
    for (int i = 0; i < argc - 1; i++) {
        all_elements[old_count + i] = bcl_strdup(argv[i + 1]);
    }

    /* Construir nueva lista */
    char *new_list = list_build(new_count, all_elements);

    /* Liberar elementos temporales */
    for (int i = 0; i < new_count; i++) {
        free(all_elements[i]);
    }
    free(all_elements);

    if (!new_list) {
        bcl_set_error(interp, "LAPPEND: out of memory");
        return BCL_ERROR;
    }

    /* Actualizar la variable con la nueva lista */
    bcl_var_set(interp, varname, new_list);

    *result = bcl_value_create(new_list);
    free(new_list);
    return BCL_OK;
}

/* ========================================================================== */
/* LRANGE - Sublista                                                         */
/* ========================================================================== */

bcl_result_t bcl_cmd_lrange(bcl_interp_t *interp, int argc, char **argv,
                            bcl_value_t **result) {
    if (argc != 3) {
        bcl_set_error(interp, "LRANGE: wrong # args: should be \"LRANGE list first last\"");
        return BCL_ERROR;
    }

    const char *list = argv[0];
    bool ok;

    double first_d = bcl_str_to_number(argv[1], &ok);
    if (!ok) {
        bcl_set_error(interp, "LRANGE: bad index \"%s\"", argv[1]);
        return BCL_ERROR;
    }

    double last_d = bcl_str_to_number(argv[2], &ok);
    if (!ok) {
        bcl_set_error(interp, "LRANGE: bad index \"%s\"", argv[2]);
        return BCL_ERROR;
    }

    int first = (int)first_d;
    int last = (int)last_d;
    int count = list_count_elements(list);

    /* Ajustar índices negativos */
    if (first < 0) first = 0;
    if (last >= count) last = count - 1;
    if (first > last) {
        *result = bcl_value_create("");
        return BCL_OK;
    }

    /* Extraer elementos del rango */
    int range_count = last - first + 1;
    char **elements = malloc(sizeof(char*) * range_count);
    if (!elements) {
        bcl_set_error(interp, "LRANGE: out of memory");
        return BCL_ERROR;
    }

    for (int i = 0; i < range_count; i++) {
        elements[i] = list_get_element(list, first + i);
        if (!elements[i]) {
            for (int j = 0; j < i; j++) free(elements[j]);
            free(elements);
            bcl_set_error(interp, "LRANGE: internal error");
            return BCL_ERROR;
        }
    }

    /* Construir sublista */
    char *sublist = list_build(range_count, elements);

    for (int i = 0; i < range_count; i++) {
        free(elements[i]);
    }
    free(elements);

    if (!sublist) {
        bcl_set_error(interp, "LRANGE: out of memory");
        return BCL_ERROR;
    }

    *result = bcl_value_create(sublist);
    free(sublist);
    return BCL_OK;
}

/* ========================================================================== */
/* SPLIT - Dividir string en lista                                           */
/* ========================================================================== */

bcl_result_t bcl_cmd_split(bcl_interp_t *interp, int argc, char **argv,
                           bcl_value_t **result) {
    if (argc != 2) {
        bcl_set_error(interp, "SPLIT: wrong # args: should be \"SPLIT string separator\"");
        return BCL_ERROR;
    }

    const char *text = argv[0];
    const char *sep = argv[1];

    if (strlen(sep) != 1) {
        bcl_set_error(interp, "SPLIT: separator must be a single character");
        return BCL_ERROR;
    }

    char sep_char = sep[0];

    /* Contar cuántos elementos habrá */
    int count = 1;
    for (const char *p = text; *p; p++) {
        if (*p == sep_char) count++;
    }

    /* Extraer elementos */
    char **elements = malloc(sizeof(char*) * count);
    if (!elements) {
        bcl_set_error(interp, "SPLIT: out of memory");
        return BCL_ERROR;
    }

    int idx = 0;
    const char *start = text;
    for (const char *p = text; ; p++) {
        if (*p == sep_char || *p == '\0') {
            size_t len = p - start;
            elements[idx] = malloc(len + 1);
            if (!elements[idx]) {
                for (int i = 0; i < idx; i++) free(elements[i]);
                free(elements);
                bcl_set_error(interp, "SPLIT: out of memory");
                return BCL_ERROR;
            }
            strncpy(elements[idx], start, len);
            elements[idx][len] = '\0';
            idx++;

            if (*p == '\0') break;
            start = p + 1;
        }
    }

    /* Construir lista */
    char *list = list_build(count, elements);

    for (int i = 0; i < count; i++) {
        free(elements[i]);
    }
    free(elements);

    if (!list) {
        bcl_set_error(interp, "SPLIT: out of memory");
        return BCL_ERROR;
    }

    *result = bcl_value_create(list);
    free(list);
    return BCL_OK;
}

/* ========================================================================== */
/* JOIN - Unir lista en string                                               */
/* ========================================================================== */

bcl_result_t bcl_cmd_join(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result) {
    if (argc != 2) {
        bcl_set_error(interp, "JOIN: wrong # args: should be \"JOIN list separator\"");
        return BCL_ERROR;
    }

    const char *list = argv[0];
    const char *sep = argv[1];

    int count = list_count_elements(list);
    if (count == 0) {
        *result = bcl_value_create("");
        return BCL_OK;
    }

    /* Calcular tamaño necesario */
    size_t total_len = 0;
    for (int i = 0; i < count; i++) {
        char *elem = list_get_element(list, i);
        if (elem) {
            total_len += strlen(elem);
            free(elem);
        }
    }
    total_len += strlen(sep) * (count - 1);

    /* Construir string */
    char *str = malloc(total_len + 1);
    if (!str) {
        bcl_set_error(interp, "JOIN: out of memory");
        return BCL_ERROR;
    }

    char *p = str;
    for (int i = 0; i < count; i++) {
        char *elem = list_get_element(list, i);
        if (elem) {
            strcpy(p, elem);
            p += strlen(elem);
            free(elem);
        }

        if (i < count - 1) {
            strcpy(p, sep);
            p += strlen(sep);
        }
    }
    *p = '\0';

    *result = bcl_value_create(str);
    free(str);
    return BCL_OK;
}

/* ========================================================================== */
/* LINSERT - Insertar elementos en lista                                     */
/* ========================================================================== */

bcl_result_t bcl_cmd_linsert(bcl_interp_t *interp, int argc, char **argv,
                             bcl_value_t **result) {
    if (argc < 3) {
        bcl_set_error(interp, "LINSERT: wrong # args: should be \"LINSERT list index element ?element...?\"");
        return BCL_ERROR;
    }

    const char *list = argv[0];
    const char *index_str = argv[1];

    int old_count = list_count_elements(list);
    int insert_count = argc - 2;
    int new_count = old_count + insert_count;

    /* Parsear índice (soportar "end") */
    int index;
    if (strcmp(index_str, "end") == 0) {
        index = old_count;
    } else {
        bool ok;
        double idx_d = bcl_str_to_number(index_str, &ok);
        if (!ok) {
            bcl_set_error(interp, "LINSERT: bad index \"%s\"", index_str);
            return BCL_ERROR;
        }
        index = (int)idx_d;
    }

    if (index < 0) index = 0;
    if (index > old_count) index = old_count;

    /* Construir nueva lista */
    char **all_elements = malloc(sizeof(char*) * new_count);
    if (!all_elements) {
        bcl_set_error(interp, "LINSERT: out of memory");
        return BCL_ERROR;
    }

    /* Copiar elementos antes del índice */
    for (int i = 0; i < index; i++) {
        all_elements[i] = list_get_element(list, i);
    }

    /* Insertar nuevos elementos */
    for (int i = 0; i < insert_count; i++) {
        all_elements[index + i] = bcl_strdup(argv[2 + i]);
    }

    /* Copiar elementos después del índice */
    for (int i = index; i < old_count; i++) {
        all_elements[i + insert_count] = list_get_element(list, i);
    }

    /* Construir lista */
    char *new_list = list_build(new_count, all_elements);

    for (int i = 0; i < new_count; i++) {
        free(all_elements[i]);
    }
    free(all_elements);

    if (!new_list) {
        bcl_set_error(interp, "LINSERT: out of memory");
        return BCL_ERROR;
    }

    *result = bcl_value_create(new_list);
    free(new_list);
    return BCL_OK;
}

/* ========================================================================== */
/* LREPLACE - Reemplazar rango en lista                                      */
/* ========================================================================== */

bcl_result_t bcl_cmd_lreplace(bcl_interp_t *interp, int argc, char **argv,
                              bcl_value_t **result) {
    if (argc < 3) {
        bcl_set_error(interp, "LREPLACE: wrong # args: should be \"LREPLACE list first last ?element...?\"");
        return BCL_ERROR;
    }

    const char *list = argv[0];
    bool ok;

    double first_d = bcl_str_to_number(argv[1], &ok);
    if (!ok) {
        bcl_set_error(interp, "LREPLACE: bad index \"%s\"", argv[1]);
        return BCL_ERROR;
    }

    double last_d = bcl_str_to_number(argv[2], &ok);
    if (!ok) {
        bcl_set_error(interp, "LREPLACE: bad index \"%s\"", argv[2]);
        return BCL_ERROR;
    }

    int first = (int)first_d;
    int last = (int)last_d;
    int old_count = list_count_elements(list);
    int replace_count = argc - 3;

    /* Ajustar índices */
    if (first < 0) first = 0;
    if (last >= old_count) last = old_count - 1;
    if (first > last) first = last;

    int removed = last - first + 1;
    int new_count = old_count - removed + replace_count;

    char **all_elements = malloc(sizeof(char*) * new_count);
    if (!all_elements) {
        bcl_set_error(interp, "LREPLACE: out of memory");
        return BCL_ERROR;
    }

    /* Elementos antes del rango */
    for (int i = 0; i < first; i++) {
        all_elements[i] = list_get_element(list, i);
    }

    /* Nuevos elementos de reemplazo */
    for (int i = 0; i < replace_count; i++) {
        all_elements[first + i] = bcl_strdup(argv[3 + i]);
    }

    /* Elementos después del rango */
    for (int i = last + 1; i < old_count; i++) {
        all_elements[first + replace_count + (i - last - 1)] = list_get_element(list, i);
    }

    /* Construir lista */
    char *new_list = list_build(new_count, all_elements);

    for (int i = 0; i < new_count; i++) {
        free(all_elements[i]);
    }
    free(all_elements);

    if (!new_list) {
        bcl_set_error(interp, "LREPLACE: out of memory");
        return BCL_ERROR;
    }

    *result = bcl_value_create(new_list);
    free(new_list);
    return BCL_OK;
}

/* ========================================================================== */
/* CONCAT - Concatenar listas                                                */
/* ========================================================================== */

bcl_result_t bcl_cmd_concat(bcl_interp_t *interp, int argc, char **argv,
                            bcl_value_t **result) {
    if (argc == 0) {
        *result = bcl_value_create("");
        return BCL_OK;
    }

    /* Contar total de elementos */
    int total_count = 0;
    for (int i = 0; i < argc; i++) {
        total_count += list_count_elements(argv[i]);
    }

    if (total_count == 0) {
        *result = bcl_value_create("");
        return BCL_OK;
    }

    /* Extraer todos los elementos */
    char **all_elements = malloc(sizeof(char*) * total_count);
    if (!all_elements) {
        bcl_set_error(interp, "CONCAT: out of memory");
        return BCL_ERROR;
    }

    int idx = 0;
    for (int i = 0; i < argc; i++) {
        int count = list_count_elements(argv[i]);
        for (int j = 0; j < count; j++) {
            all_elements[idx++] = list_get_element(argv[i], j);
        }
    }

    /* Construir lista concatenada */
    char *concat_list = list_build(total_count, all_elements);

    for (int i = 0; i < total_count; i++) {
        free(all_elements[i]);
    }
    free(all_elements);

    if (!concat_list) {
        bcl_set_error(interp, "CONCAT: out of memory");
        return BCL_ERROR;
    }

    *result = bcl_value_create(concat_list);
    free(concat_list);
    return BCL_OK;
}

/* ========================================================================== */
/* LSORT - Ordenar lista                                                     */
/* ========================================================================== */

static int compare_strings(const void *a, const void *b) {
    return strcmp(*(const char**)a, *(const char**)b);
}

bcl_result_t bcl_cmd_lsort(bcl_interp_t *interp, int argc, char **argv,
                           bcl_value_t **result) {
    if (argc != 1) {
        bcl_set_error(interp, "LSORT: wrong # args: should be \"LSORT list\"");
        return BCL_ERROR;
    }

    const char *list = argv[0];
    int count = list_count_elements(list);

    if (count == 0) {
        *result = bcl_value_create("");
        return BCL_OK;
    }

    /* Extraer elementos */
    char **elements = malloc(sizeof(char*) * count);
    if (!elements) {
        bcl_set_error(interp, "LSORT: out of memory");
        return BCL_ERROR;
    }

    for (int i = 0; i < count; i++) {
        elements[i] = list_get_element(list, i);
    }

    /* Ordenar */
    qsort(elements, count, sizeof(char*), compare_strings);

    /* Construir lista ordenada */
    char *sorted_list = list_build(count, elements);

    for (int i = 0; i < count; i++) {
        free(elements[i]);
    }
    free(elements);

    if (!sorted_list) {
        bcl_set_error(interp, "LSORT: out of memory");
        return BCL_ERROR;
    }

    *result = bcl_value_create(sorted_list);
    free(sorted_list);
    return BCL_OK;
}

/* ========================================================================== */
/* LSEARCH - Buscar en lista                                                 */
/* ========================================================================== */

bcl_result_t bcl_cmd_lsearch(bcl_interp_t *interp, int argc, char **argv,
                             bcl_value_t **result) {
    if (argc != 2) {
        bcl_set_error(interp, "LSEARCH: wrong # args: should be \"LSEARCH list value\"");
        return BCL_ERROR;
    }

    const char *list = argv[0];
    const char *value = argv[1];
    int count = list_count_elements(list);

    /* Buscar valor */
    for (int i = 0; i < count; i++) {
        char *elem = list_get_element(list, i);
        if (elem) {
            bool found = (strcmp(elem, value) == 0);
            free(elem);

            if (found) {
                char buf[32];
                snprintf(buf, sizeof(buf), "%d", i);
                *result = bcl_value_create(buf);
                return BCL_OK;
            }
        }
    }

    /* No encontrado */
    *result = bcl_value_create("-1");
    return BCL_OK;
}
