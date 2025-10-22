/**
 * @file bcl_string_cmd.c
 * @brief Implementación del comando STRING y sus subcomandos
 * @version 1.1
 *
 * Alineado con Tcl 8.6.13 según especificación BCL_STRING_Referencia_v1.1_PC.txt
 */

#include "bcl.h"
#include "bcl_commands.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

/* ========================================================================== */
/* UTILIDADES INTERNAS                                                       */
/* ========================================================================== */

/**
 * @brief Parsea un índice (entero, "end", "end-N", etc.)
 * @param s String con el índice
 * @param len Longitud del string al que se refiere
 * @return Índice normalizado (0..len-1) o -1 si fuera de rango
 */
static int parse_index(const char *s, int len) {
    if (!s) return -1;

    /* Caso: "end" */
    if (bcl_strcasecmp(s, "end") == 0) {
        return len > 0 ? len - 1 : 0;
    }

    /* Caso: "end-N" o "end+N" */
    if (bcl_strncasecmp(s, "end", 3) == 0) {
        const char *rest = s + 3;
        if (*rest == '-' || *rest == '+') {
            int offset = atoi(rest);
            int idx = len - 1 + offset;
            return idx < 0 ? 0 : (idx >= len ? len - 1 : idx);
        }
    }

    /* Caso: entero simple o "M+N", "M-N" */
    int idx = atoi(s);

    /* Buscar operadores */
    const char *op = strpbrk(s, "+-");
    if (op && op != s) { /* No es el signo del número */
        int offset = atoi(op);
        idx = idx + offset;
    }

    /* Saturar */
    if (idx < 0) return 0;
    if (idx >= len) return len - 1;
    return idx;
}

/* ========================================================================== */
/* STRING LENGTH                                                             */
/* ========================================================================== */

bcl_result_t bcl_string_length(bcl_interp_t *interp, int argc, char **argv,
                               bcl_value_t **result) {
    if (argc != 1) {
        bcl_set_error(interp, "wrong # args: should be \"STRING LENGTH string\"");
        return BCL_ERROR;
    }

    size_t len = strlen(argv[0]);
    char buf[32];
    snprintf(buf, sizeof(buf), "%zu", len);

    if (result) {
        *result = bcl_value_create(buf);
    }

    return BCL_OK;
}

/* ========================================================================== */
/* STRING CAT                                                                */
/* ========================================================================== */

bcl_result_t bcl_string_cat(bcl_interp_t *interp, int argc, char **argv,
                            bcl_value_t **result) {
    BCL_UNUSED(interp);

    bcl_string_t *cat = bcl_string_create("");

    for (int i = 0; i < argc; i++) {
        bcl_string_append(cat, argv[i]);
    }

    if (result) {
        *result = bcl_value_create(bcl_string_cstr(cat));
    }

    bcl_string_destroy(cat);
    return BCL_OK;
}

/* ========================================================================== */
/* STRING REVERSE                                                            */
/* ========================================================================== */

bcl_result_t bcl_string_reverse(bcl_interp_t *interp, int argc, char **argv,
                                bcl_value_t **result) {
    if (argc != 1) {
        bcl_set_error(interp, "wrong # args: should be \"STRING REVERSE string\"");
        return BCL_ERROR;
    }

    const char *s = argv[0];
    size_t len = strlen(s);

    char *rev = malloc(len + 1);
    if (!rev) {
        bcl_set_error(interp, "Out of memory");
        return BCL_ERROR;
    }

    for (size_t i = 0; i < len; i++) {
        rev[i] = s[len - 1 - i];
    }
    rev[len] = '\0';

    if (result) {
        *result = bcl_value_create(rev);
    }

    free(rev);
    return BCL_OK;
}

/* ========================================================================== */
/* STRING REPEAT                                                             */
/* ========================================================================== */

bcl_result_t bcl_string_repeat(bcl_interp_t *interp, int argc, char **argv,
                               bcl_value_t **result) {
    if (argc != 2) {
        bcl_set_error(interp, "wrong # args: should be \"STRING REPEAT string count\"");
        return BCL_ERROR;
    }

    const char *s = argv[0];
    int count = atoi(argv[1]);

    if (count < 0) {
        bcl_set_error(interp, "count must be non-negative");
        return BCL_ERROR;
    }

    bcl_string_t *rep = bcl_string_create("");
    for (int i = 0; i < count; i++) {
        bcl_string_append(rep, s);
    }

    if (result) {
        *result = bcl_value_create(bcl_string_cstr(rep));
    }

    bcl_string_destroy(rep);
    return BCL_OK;
}

/* ========================================================================== */
/* STRING TOUPPER / TOLOWER / TOTITLE                                        */
/* ========================================================================== */

bcl_result_t bcl_string_toupper(bcl_interp_t *interp, int argc, char **argv,
                                bcl_value_t **result) {
    if (argc < 1 || argc > 3) {
        bcl_set_error(interp, "wrong # args: should be \"STRING TOUPPER string [first [last]]\"");
        return BCL_ERROR;
    }

    const char *s = argv[0];
    int len = strlen(s);
    int first = 0;
    int last = len - 1;

    if (argc >= 2) {
        first = parse_index(argv[1], len);
    }
    if (argc >= 3) {
        last = parse_index(argv[2], len);
    }

    if (first < 0 || last < 0 || first > last || first >= len) {
        /* Fuera de rango - devolver original */
        if (result) {
            *result = bcl_value_create(s);
        }
        return BCL_OK;
    }

    char *upper = bcl_strdup(s);
    if (!upper) {
        bcl_set_error(interp, "Out of memory");
        return BCL_ERROR;
    }

    for (int i = first; i <= last && i < len; i++) {
        upper[i] = toupper((unsigned char)upper[i]);
    }

    if (result) {
        *result = bcl_value_create(upper);
    }

    free(upper);
    return BCL_OK;
}

bcl_result_t bcl_string_tolower(bcl_interp_t *interp, int argc, char **argv,
                                bcl_value_t **result) {
    if (argc < 1 || argc > 3) {
        bcl_set_error(interp, "wrong # args: should be \"STRING TOLOWER string [first [last]]\"");
        return BCL_ERROR;
    }

    const char *s = argv[0];
    int len = strlen(s);
    int first = 0;
    int last = len - 1;

    if (argc >= 2) {
        first = parse_index(argv[1], len);
    }
    if (argc >= 3) {
        last = parse_index(argv[2], len);
    }

    if (first < 0 || last < 0 || first > last || first >= len) {
        if (result) {
            *result = bcl_value_create(s);
        }
        return BCL_OK;
    }

    char *lower = bcl_strdup(s);
    if (!lower) {
        bcl_set_error(interp, "Out of memory");
        return BCL_ERROR;
    }

    for (int i = first; i <= last && i < len; i++) {
        lower[i] = tolower((unsigned char)lower[i]);
    }

    if (result) {
        *result = bcl_value_create(lower);
    }

    free(lower);
    return BCL_OK;
}

bcl_result_t bcl_string_totitle(bcl_interp_t *interp, int argc, char **argv,
                                bcl_value_t **result) {
    if (argc < 1 || argc > 3) {
        bcl_set_error(interp, "wrong # args: should be \"STRING TOTITLE string [first [last]]\"");
        return BCL_ERROR;
    }

    const char *s = argv[0];
    int len = strlen(s);
    int first = 0;
    int last = len - 1;

    if (argc >= 2) {
        first = parse_index(argv[1], len);
    }
    if (argc >= 3) {
        last = parse_index(argv[2], len);
    }

    if (first < 0 || last < 0 || first > last || first >= len) {
        if (result) {
            *result = bcl_value_create(s);
        }
        return BCL_OK;
    }

    char *title = bcl_strdup(s);
    if (!title) {
        bcl_set_error(interp, "Out of memory");
        return BCL_ERROR;
    }

    /* TOTITLE: primera letra mayúscula, resto minúsculas */
    bool word_start = true;
    for (int i = first; i <= last && i < len; i++) {
        if (isalpha((unsigned char)title[i])) {
            if (word_start) {
                title[i] = toupper((unsigned char)title[i]);
                word_start = false;
            } else {
                title[i] = tolower((unsigned char)title[i]);
            }
        } else {
            word_start = true; /* Espacio u otro carácter inicia nueva palabra */
        }
    }

    if (result) {
        *result = bcl_value_create(title);
    }

    free(title);
    return BCL_OK;
}

/* ========================================================================== */
/* STRING TRIM / TRIMLEFT / TRIMRIGHT                                        */
/* ========================================================================== */

bcl_result_t bcl_string_trim(bcl_interp_t *interp, int argc, char **argv,
                             bcl_value_t **result) {
    if (argc < 1 || argc > 2) {
        bcl_set_error(interp, "wrong # args: should be \"STRING TRIM string [chars]\"");
        return BCL_ERROR;
    }

    const char *s = argv[0];
    const char *chars = (argc >= 2) ? argv[1] : " \t\n\r\0";

    /* Trim left */
    while (*s && strchr(chars, *s)) {
        s++;
    }

    /* Trim right */
    int len = strlen(s);
    while (len > 0 && strchr(chars, s[len - 1])) {
        len--;
    }

    char *trimmed = malloc(len + 1);
    if (!trimmed) {
        bcl_set_error(interp, "Out of memory");
        return BCL_ERROR;
    }

    memcpy(trimmed, s, len);
    trimmed[len] = '\0';

    if (result) {
        *result = bcl_value_create(trimmed);
    }

    free(trimmed);
    return BCL_OK;
}

bcl_result_t bcl_string_trimleft(bcl_interp_t *interp, int argc, char **argv,
                                 bcl_value_t **result) {
    if (argc < 1 || argc > 2) {
        bcl_set_error(interp, "wrong # args: should be \"STRING TRIMLEFT string [chars]\"");
        return BCL_ERROR;
    }

    const char *s = argv[0];
    const char *chars = (argc >= 2) ? argv[1] : " \t\n\r\0";

    /* Trim left */
    while (*s && strchr(chars, *s)) {
        s++;
    }

    if (result) {
        *result = bcl_value_create(s);
    }

    return BCL_OK;
}

bcl_result_t bcl_string_trimright(bcl_interp_t *interp, int argc, char **argv,
                                  bcl_value_t **result) {
    if (argc < 1 || argc > 2) {
        bcl_set_error(interp, "wrong # args: should be \"STRING TRIMRIGHT string [chars]\"");
        return BCL_ERROR;
    }

    const char *s = argv[0];
    const char *chars = (argc >= 2) ? argv[1] : " \t\n\r\0";

    int len = strlen(s);
    while (len > 0 && strchr(chars, s[len - 1])) {
        len--;
    }

    char *trimmed = malloc(len + 1);
    if (!trimmed) {
        bcl_set_error(interp, "Out of memory");
        return BCL_ERROR;
    }

    memcpy(trimmed, s, len);
    trimmed[len] = '\0';

    if (result) {
        *result = bcl_value_create(trimmed);
    }

    free(trimmed);
    return BCL_OK;
}

/* ========================================================================== */
/* STRING INDEX                                                              */
/* ========================================================================== */

bcl_result_t bcl_string_index(bcl_interp_t *interp, int argc, char **argv,
                              bcl_value_t **result) {
    if (argc != 2) {
        bcl_set_error(interp, "wrong # args: should be \"STRING INDEX string index\"");
        return BCL_ERROR;
    }

    const char *s = argv[0];
    int len = strlen(s);
    int idx = parse_index(argv[1], len);

    if (idx < 0 || idx >= len) {
        /* Fuera de rango - devolver string vacío */
        if (result) {
            *result = bcl_value_create("");
        }
        return BCL_OK;
    }

    char ch[2] = { s[idx], '\0' };

    if (result) {
        *result = bcl_value_create(ch);
    }

    return BCL_OK;
}

/* ========================================================================== */
/* STRING RANGE                                                              */
/* ========================================================================== */

bcl_result_t bcl_string_range(bcl_interp_t *interp, int argc, char **argv,
                              bcl_value_t **result) {
    if (argc != 3) {
        bcl_set_error(interp, "wrong # args: should be \"STRING RANGE string first last\"");
        return BCL_ERROR;
    }

    const char *s = argv[0];
    int len = strlen(s);
    int first = parse_index(argv[1], len);
    int last = parse_index(argv[2], len);

    /* Reglas de Tcl: first<0 => 0; last>=len => end; first>last => "" */
    if (first < 0) first = 0;
    if (last >= len) last = len - 1;

    if (first > last || first >= len) {
        if (result) {
            *result = bcl_value_create("");
        }
        return BCL_OK;
    }

    int range_len = last - first + 1;
    char *range = malloc(range_len + 1);
    if (!range) {
        bcl_set_error(interp, "Out of memory");
        return BCL_ERROR;
    }

    memcpy(range, s + first, range_len);
    range[range_len] = '\0';

    if (result) {
        *result = bcl_value_create(range);
    }

    free(range);
    return BCL_OK;
}

/* ========================================================================== */
/* STRING FIRST / LAST                                                       */
/* ========================================================================== */

bcl_result_t bcl_string_first(bcl_interp_t *interp, int argc, char **argv,
                              bcl_value_t **result) {
    if (argc < 2 || argc > 4) {
        bcl_set_error(interp, "wrong # args: should be \"STRING FIRST needle haystack [START i]\"");
        return BCL_ERROR;
    }

    const char *needle = argv[0];
    const char *haystack = argv[1];
    int start = 0;

    /* Procesar opción START */
    if (argc >= 3 && bcl_strcasecmp(argv[2], "START") == 0) {
        if (argc != 4) {
            bcl_set_error(interp, "START requires an index argument");
            return BCL_ERROR;
        }
        start = atoi(argv[3]);
        if (start < 0) start = 0;
    }

    int haystack_len = strlen(haystack);
    if (start >= haystack_len) {
        /* Fuera de rango */
        if (result) {
            *result = bcl_value_create("-1");
        }
        return BCL_OK;
    }

    /* Búsqueda */
    const char *found = strstr(haystack + start, needle);
    int idx = found ? (int)(found - haystack) : -1;

    char buf[32];
    snprintf(buf, sizeof(buf), "%d", idx);

    if (result) {
        *result = bcl_value_create(buf);
    }

    return BCL_OK;
}

bcl_result_t bcl_string_last(bcl_interp_t *interp, int argc, char **argv,
                             bcl_value_t **result) {
    if (argc < 2 || argc > 4) {
        bcl_set_error(interp, "wrong # args: should be \"STRING LAST needle haystack [LAST i]\"");
        return BCL_ERROR;
    }

    const char *needle = argv[0];
    const char *haystack = argv[1];
    int haystack_len = strlen(haystack);
    int last_pos = haystack_len - 1;

    /* Procesar opción LAST */
    if (argc >= 3 && bcl_strcasecmp(argv[2], "LAST") == 0) {
        if (argc != 4) {
            bcl_set_error(interp, "LAST requires an index argument");
            return BCL_ERROR;
        }
        last_pos = parse_index(argv[3], haystack_len);
    }

    /* Búsqueda desde atrás */
    int needle_len = strlen(needle);
    int idx = -1;

    for (int i = 0; i <= last_pos && i + needle_len <= haystack_len; i++) {
        if (strncmp(haystack + i, needle, needle_len) == 0) {
            idx = i; /* Actualizar última coincidencia */
        }
    }

    char buf[32];
    snprintf(buf, sizeof(buf), "%d", idx);

    if (result) {
        *result = bcl_value_create(buf);
    }

    return BCL_OK;
}

/* ========================================================================== */
/* STRING COMPARE / EQUAL                                                    */
/* ========================================================================== */

bcl_result_t bcl_string_compare(bcl_interp_t *interp, int argc, char **argv,
                                bcl_value_t **result) {
    if (argc < 2) {
        bcl_set_error(interp, "wrong # args: should be \"STRING COMPARE s1 s2 [CASE NOCASE] [LENGTH n]\"");
        return BCL_ERROR;
    }

    const char *s1 = argv[0];
    const char *s2 = argv[1];
    bool nocase = false;
    int compare_len = -1;

    /* Procesar opciones */
    for (int i = 2; i < argc; i++) {
        if (bcl_strcasecmp(argv[i], "CASE") == 0 && i + 1 < argc) {
            if (bcl_strcasecmp(argv[i + 1], "NOCASE") == 0) {
                nocase = true;
                i++;
            }
        } else if (bcl_strcasecmp(argv[i], "LENGTH") == 0 && i + 1 < argc) {
            compare_len = atoi(argv[i + 1]);
            i++;
        }
    }

    int cmp;
    if (compare_len > 0) {
        cmp = nocase ? bcl_strncasecmp(s1, s2, compare_len) : strncmp(s1, s2, compare_len);
    } else {
        cmp = nocase ? bcl_strcasecmp(s1, s2) : strcmp(s1, s2);
    }

    /* Normalizar a -1, 0, 1 */
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", cmp < 0 ? -1 : (cmp > 0 ? 1 : 0));

    if (result) {
        *result = bcl_value_create(buf);
    }

    return BCL_OK;
}

bcl_result_t bcl_string_equal(bcl_interp_t *interp, int argc, char **argv,
                              bcl_value_t **result) {
    if (argc < 2) {
        bcl_set_error(interp, "wrong # args: should be \"STRING EQUAL s1 s2 [CASE NOCASE] [LENGTH n]\"");
        return BCL_ERROR;
    }

    const char *s1 = argv[0];
    const char *s2 = argv[1];
    bool nocase = false;
    int compare_len = -1;

    /* Procesar opciones */
    for (int i = 2; i < argc; i++) {
        if (bcl_strcasecmp(argv[i], "CASE") == 0 && i + 1 < argc) {
            if (bcl_strcasecmp(argv[i + 1], "NOCASE") == 0) {
                nocase = true;
                i++;
            }
        } else if (bcl_strcasecmp(argv[i], "LENGTH") == 0 && i + 1 < argc) {
            compare_len = atoi(argv[i + 1]);
            i++;
        }
    }

    int cmp;
    if (compare_len > 0) {
        cmp = nocase ? bcl_strncasecmp(s1, s2, compare_len) : strncmp(s1, s2, compare_len);
    } else {
        cmp = nocase ? bcl_strcasecmp(s1, s2) : strcmp(s1, s2);
    }

    if (result) {
        *result = bcl_value_create(cmp == 0 ? "1" : "0");
    }

    return BCL_OK;
}

/* ========================================================================== */
/* STRING REPLACE                                                            */
/* ========================================================================== */

bcl_result_t bcl_string_replace(bcl_interp_t *interp, int argc, char **argv,
                                bcl_value_t **result) {
    if (argc < 3 || argc > 4) {
        bcl_set_error(interp, "wrong # args: should be \"STRING REPLACE string first last [new]\"");
        return BCL_ERROR;
    }

    const char *s = argv[0];
    int len = strlen(s);
    int first = parse_index(argv[1], len);
    int last = parse_index(argv[2], len);
    const char *new_str = (argc >= 4) ? argv[3] : "";

    /* Validar rangos */
    if (first < 0) first = 0;
    if (last >= len) last = len - 1;

    if (first > last || first >= len) {
        /* Fuera de rango - devolver original */
        if (result) {
            *result = bcl_value_create(s);
        }
        return BCL_OK;
    }

    /* Construir resultado: parte antes + new_str + parte después */
    bcl_string_t *res = bcl_string_create("");

    /* Agregar parte antes de first */
    for (int i = 0; i < first; i++) {
        bcl_string_append_char(res, s[i]);
    }

    /* Agregar new_str */
    bcl_string_append(res, new_str);

    /* Agregar parte después de last */
    for (int i = last + 1; i < len; i++) {
        bcl_string_append_char(res, s[i]);
    }

    if (result) {
        *result = bcl_value_create(bcl_string_cstr(res));
    }

    bcl_string_destroy(res);
    return BCL_OK;
}

/* ========================================================================== */
/* STRING MAP                                                                */
/* ========================================================================== */

bcl_result_t bcl_string_map(bcl_interp_t *interp, int argc, char **argv,
                            bcl_value_t **result) {
    if (argc < 2) {
        bcl_set_error(interp, "wrong # args: should be \"STRING MAP mapping string [CASE NOCASE]\"");
        return BCL_ERROR;
    }

    const char *mapping_str = argv[0];
    const char *input = argv[1];
    bool nocase = false;

    /* Verificar opción NOCASE */
    if (argc >= 3 && bcl_strcasecmp(argv[2], "CASE") == 0 && argc >= 4) {
        if (bcl_strcasecmp(argv[3], "NOCASE") == 0) {
            nocase = true;
        }
    }

    /* Parsear mapping (lista de pares key value) */
    int map_argc;
    char **map_argv = bcl_parse_line(interp, mapping_str, &map_argc);

    if (!map_argv || map_argc % 2 != 0) {
        bcl_set_error(interp, "mapping must be a list with even number of elements");
        if (map_argv) bcl_free_tokens(map_argv, map_argc);
        return BCL_ERROR;
    }

    /* Aplicar reemplazos en orden */
    char *current = bcl_strdup(input);

    for (int i = 0; i < map_argc; i += 2) {
        const char *key = map_argv[i];
        const char *value = map_argv[i + 1];
        int key_len = strlen(key);

        /* Buscar y reemplazar todas las ocurrencias de key */
        bcl_string_t *temp = bcl_string_create("");
        int pos = 0;
        int len = strlen(current);

        while (pos < len) {
            int match = -1;

            /* Buscar key en posición actual */
            if (nocase) {
                if (bcl_strncasecmp(current + pos, key, key_len) == 0) {
                    match = pos;
                }
            } else {
                if (strncmp(current + pos, key, key_len) == 0) {
                    match = pos;
                }
            }

            if (match >= 0) {
                /* Encontrado - agregar value */
                bcl_string_append(temp, value);
                pos += key_len;
            } else {
                /* No encontrado - agregar carácter actual */
                bcl_string_append_char(temp, current[pos]);
                pos++;
            }
        }

        /* Actualizar current con el resultado */
        free(current);
        current = bcl_strdup(bcl_string_cstr(temp));
        bcl_string_destroy(temp);
    }

    if (result) {
        *result = bcl_value_create(current);
    }

    free(current);
    bcl_free_tokens(map_argv, map_argc);
    return BCL_OK;
}

/* ========================================================================== */
/* STRING MATCH (glob patterns)                                              */
/* ========================================================================== */

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
                    char c1 = nocase ? tolower(*pattern) : *pattern;
                    char c2 = nocase ? tolower(*(pattern + 2)) : *(pattern + 2);
                    char ch = nocase ? tolower(*str) : *str;

                    if (ch >= c1 && ch <= c2) match = true;
                    pattern += 3;
                } else {
                    /* Carácter simple */
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

bcl_result_t bcl_string_match(bcl_interp_t *interp, int argc, char **argv,
                              bcl_value_t **result) {
    if (argc < 2) {
        bcl_set_error(interp, "wrong # args: should be \"STRING MATCH pattern string [CASE NOCASE]\"");
        return BCL_ERROR;
    }

    const char *pattern = argv[0];
    const char *str = argv[1];
    bool nocase = false;

    /* Verificar opción NOCASE */
    if (argc >= 3 && bcl_strcasecmp(argv[2], "CASE") == 0 && argc >= 4) {
        if (bcl_strcasecmp(argv[3], "NOCASE") == 0) {
            nocase = true;
        }
    }

    bool matches = match_pattern(pattern, str, nocase);

    if (result) {
        *result = bcl_value_create(matches ? "1" : "0");
    }

    return BCL_OK;
}

/* ========================================================================== */
/* STRING IS (validaciones de clase)                                         */
/* ========================================================================== */

static bool is_alnum(const char *s) {
    if (!*s) return true; /* Empty string */
    while (*s) {
        if (!isalnum((unsigned char)*s)) return false;
        s++;
    }
    return true;
}

static bool is_alpha(const char *s) {
    if (!*s) return true;
    while (*s) {
        if (!isalpha((unsigned char)*s)) return false;
        s++;
    }
    return true;
}

static bool is_digit(const char *s) {
    if (!*s) return true;
    while (*s) {
        if (!isdigit((unsigned char)*s)) return false;
        s++;
    }
    return true;
}

static bool is_integer(const char *s) {
    if (!*s) return true;
    /* Permitir espacios iniciales/finales */
    while (*s && isspace((unsigned char)*s)) s++;
    if (*s == '+' || *s == '-') s++;
    if (!*s) return false;

    /* Hex */
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
        if (!*s) return false;
        while (*s && isxdigit((unsigned char)*s)) s++;
    } else {
        while (*s && isdigit((unsigned char)*s)) s++;
    }

    while (*s && isspace((unsigned char)*s)) s++;
    return !*s;
}

static bool is_double(const char *s) {
    if (!*s) return true;
    bool ok;
    bcl_str_to_number(s, &ok);
    return ok;
}

static bool is_space(const char *s) {
    if (!*s) return true;
    while (*s) {
        if (!isspace((unsigned char)*s)) return false;
        s++;
    }
    return true;
}

static bool is_upper(const char *s) {
    if (!*s) return true;
    while (*s) {
        if (isalpha((unsigned char)*s) && !isupper((unsigned char)*s)) return false;
        s++;
    }
    return true;
}

static bool is_lower(const char *s) {
    if (!*s) return true;
    while (*s) {
        if (isalpha((unsigned char)*s) && !islower((unsigned char)*s)) return false;
        s++;
    }
    return true;
}

bcl_result_t bcl_string_is(bcl_interp_t *interp, int argc, char **argv,
                           bcl_value_t **result) {
    if (argc < 2) {
        bcl_set_error(interp, "wrong # args: should be \"STRING IS class string [STRICT] [FAILINDEX var]\"");
        return BCL_ERROR;
    }

    const char *class = argv[0];
    const char *str = argv[1];
    bool strict = false;
    char *failindex_var = NULL;

    /* Procesar opciones */
    for (int i = 2; i < argc; i++) {
        if (bcl_strcasecmp(argv[i], "STRICT") == 0) {
            strict = true;
        } else if (bcl_strcasecmp(argv[i], "FAILINDEX") == 0 && i + 1 < argc) {
            failindex_var = argv[i + 1];
            i++;
        }
    }

    bool is_valid = false;
    int fail_idx = -1;

    /* STRICT: string vacío devuelve 0 */
    if (strict && !*str) {
        is_valid = false;
    } else {
        /* Clasificar */
        if (bcl_strcasecmp(class, "ALNUM") == 0) {
            is_valid = is_alnum(str);
        } else if (bcl_strcasecmp(class, "ALPHA") == 0) {
            is_valid = is_alpha(str);
        } else if (bcl_strcasecmp(class, "DIGIT") == 0) {
            is_valid = is_digit(str);
        } else if (bcl_strcasecmp(class, "INTEGER") == 0 || bcl_strcasecmp(class, "ENTIER") == 0) {
            is_valid = is_integer(str);
        } else if (bcl_strcasecmp(class, "DOUBLE") == 0) {
            is_valid = is_double(str);
        } else if (bcl_strcasecmp(class, "SPACE") == 0) {
            is_valid = is_space(str);
        } else if (bcl_strcasecmp(class, "UPPER") == 0) {
            is_valid = is_upper(str);
        } else if (bcl_strcasecmp(class, "LOWER") == 0) {
            is_valid = is_lower(str);
        } else if (bcl_strcasecmp(class, "BOOLEAN") == 0 || bcl_strcasecmp(class, "TRUE") == 0 || bcl_strcasecmp(class, "FALSE") == 0) {
            /* Simplificado: acepta 0/1, true/false, yes/no */
            is_valid = (bcl_strcasecmp(str, "true") == 0 || bcl_strcasecmp(str, "false") == 0 ||
                       bcl_strcasecmp(str, "yes") == 0 || bcl_strcasecmp(str, "no") == 0 ||
                       bcl_strcasecmp(str, "1") == 0 || bcl_strcasecmp(str, "0") == 0);
        } else {
            bcl_set_error(interp, "unknown class \"%s\"", class);
            return BCL_ERROR;
        }
    }

    /* Si falla, calcular índice de fallo */
    if (!is_valid && failindex_var) {
        /* Buscar primer carácter que falle */
        for (size_t i = 0; str[i]; i++) {
            /* Simplificado: reportar primera posición */
            fail_idx = i;
            break;
        }

        char buf[32];
        snprintf(buf, sizeof(buf), "%d", fail_idx);
        bcl_var_set(interp, failindex_var, buf);
    }

    if (result) {
        *result = bcl_value_create(is_valid ? "1" : "0");
    }

    return BCL_OK;
}

/* ========================================================================== */
/* STRING WORDSTART / WORDEND                                                */
/* ========================================================================== */

static bool is_wordchar(char c) {
    return isalnum((unsigned char)c) || c == '_';
}

bcl_result_t bcl_string_wordstart(bcl_interp_t *interp, int argc, char **argv,
                                  bcl_value_t **result) {
    if (argc != 2) {
        bcl_set_error(interp, "wrong # args: should be \"STRING WORDSTART string charIndex\"");
        return BCL_ERROR;
    }

    const char *s = argv[0];
    int len = strlen(s);
    int idx = parse_index(argv[1], len);

    if (idx < 0 || idx >= len) {
        /* Fuera de rango */
        if (result) {
            *result = bcl_value_create("-1");
        }
        return BCL_OK;
    }

    /* Retroceder al inicio de la palabra */
    int start = idx;
    if (is_wordchar(s[idx])) {
        while (start > 0 && is_wordchar(s[start - 1])) {
            start--;
        }
    }
    /* Si no es wordchar, el inicio es el mismo índice */

    char buf[32];
    snprintf(buf, sizeof(buf), "%d", start);

    if (result) {
        *result = bcl_value_create(buf);
    }

    return BCL_OK;
}

bcl_result_t bcl_string_wordend(bcl_interp_t *interp, int argc, char **argv,
                                bcl_value_t **result) {
    if (argc != 2) {
        bcl_set_error(interp, "wrong # args: should be \"STRING WORDEND string charIndex\"");
        return BCL_ERROR;
    }

    const char *s = argv[0];
    int len = strlen(s);
    int idx = parse_index(argv[1], len);

    if (idx < 0 || idx >= len) {
        /* Fuera de rango */
        if (result) {
            *result = bcl_value_create("-1");
        }
        return BCL_OK;
    }

    /* Avanzar al final de la palabra */
    int end = idx;
    if (is_wordchar(s[idx])) {
        while (end < len && is_wordchar(s[end])) {
            end++;
        }
    } else {
        /* Si no es wordchar, el final es idx + 1 */
        end = idx + 1;
    }

    char buf[32];
    snprintf(buf, sizeof(buf), "%d", end);

    if (result) {
        *result = bcl_value_create(buf);
    }

    return BCL_OK;
}

/* ========================================================================== */
/* STRING DISPATCHER                                                         */
/* ========================================================================== */

typedef struct {
    const char *name;
    bcl_command_func_t func;
} string_subcommand_t;

static const string_subcommand_t string_subcommands[] = {
    {"CAT",        bcl_string_cat},
    {"COMPARE",    bcl_string_compare},
    {"EQUAL",      bcl_string_equal},
    {"FIRST",      bcl_string_first},
    {"INDEX",      bcl_string_index},
    {"IS",         bcl_string_is},
    {"LAST",       bcl_string_last},
    {"LENGTH",     bcl_string_length},
    {"MAP",        bcl_string_map},
    {"MATCH",      bcl_string_match},
    {"RANGE",      bcl_string_range},
    {"REPEAT",     bcl_string_repeat},
    {"REPLACE",    bcl_string_replace},
    {"REVERSE",    bcl_string_reverse},
    {"TOLOWER",    bcl_string_tolower},
    {"TOTITLE",    bcl_string_totitle},
    {"TOUPPER",    bcl_string_toupper},
    {"TRIM",       bcl_string_trim},
    {"TRIMLEFT",   bcl_string_trimleft},
    {"TRIMRIGHT",  bcl_string_trimright},
    {"WORDEND",    bcl_string_wordend},
    {"WORDSTART",  bcl_string_wordstart},
    {NULL, NULL}
};

bcl_result_t bcl_cmd_string(bcl_interp_t *interp, int argc, char **argv,
                            bcl_value_t **result) {
    if (argc < 1) {
        bcl_set_error(interp, "wrong # args: should be \"STRING subcommand ...\"");
        return BCL_ERROR;
    }

    const char *subcmd = argv[0];

    /* Buscar subcomando */
    for (size_t i = 0; string_subcommands[i].name != NULL; i++) {
        if (bcl_strcasecmp(string_subcommands[i].name, subcmd) == 0) {
            return string_subcommands[i].func(interp, argc - 1, argv + 1, result);
        }
    }

    bcl_set_error(interp, "unknown or unimplemented STRING subcommand \"%s\"", subcmd);
    return BCL_ERROR;
}
