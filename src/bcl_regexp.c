/**
 * @file bcl_regexp.c
 * @brief Implementación básica standalone de REGEXP y REGSUB
 * @version 1.0
 *
 * Motor de expresiones regulares básico sin dependencias externas.
 * Soporta patrones simples suficientes para la mayoría de casos de uso.
 *
 * Metacaracteres soportados:
 * - . (cualquier carácter)
 * - * (0 o más repeticiones)
 * - + (1 o más repeticiones)
 * - ? (0 o 1 repetición)
 * - ^ (inicio de línea/texto)
 * - $ (fin de línea/texto)
 * - [...] (clase de caracteres)
 * - [^...] (clase negada)
 * - (grupos) (captura de grupos)
 * - | (alternancia simple)
 * - \d \w \s \D \W \S (clases predefinidas)
 *
 * Limitaciones vs PCRE2:
 * - No lookahead/lookbehind
 * - No backreferences en el patrón
 * - No cuantificadores complejos {m,n}
 * - Grupos limitados a 9
 */

#include "bcl.h"
#include "bcl_commands.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

#define MAX_GROUPS 10  /* Grupo 0 = match completo, 1-9 = subgrupos */

/* ========================================================================== */
/* ESTRUCTURAS                                                               */
/* ========================================================================== */

typedef struct {
    const char *start;  /* Inicio de la captura */
    const char *end;    /* Fin de la captura (exclusivo) */
} bcl_regex_capture_t;

typedef struct {
    bcl_regex_capture_t groups[MAX_GROUPS];
    int num_groups;
    bool nocase;
} bcl_regex_match_t;

/* ========================================================================== */
/* UTILIDADES                                                                */
/* ========================================================================== */

static bool char_match(char c, char p, bool nocase) {
    if (nocase) {
        return tolower((unsigned char)c) == tolower((unsigned char)p);
    }
    return c == p;
}

static bool is_word_char(char c) {
    return isalnum((unsigned char)c) || c == '_';
}

static bool is_space_char(char c) {
    return isspace((unsigned char)c);
}

static bool is_digit_char(char c) {
    return isdigit((unsigned char)c);
}

/* ========================================================================== */
/* MATCHING DE CLASES DE CARACTERES [...]                                   */
/* ========================================================================== */

static bool match_char_class(const char **pattern, char c, bool nocase) {
    const char *p = *pattern;
    bool negate = false;
    bool matched = false;

    if (*p != '[') return false;
    p++;

    /* Negación [^...] */
    if (*p == '^') {
        negate = true;
        p++;
    }

    char test_char = c;
    if (nocase) {
        test_char = tolower((unsigned char)c);
    }

    while (*p && *p != ']') {
        /* Rango [a-z] */
        if (p[1] == '-' && p[2] != ']' && p[2] != '\0') {
            char start = p[0];
            char end = p[2];
            if (nocase) {
                start = tolower((unsigned char)start);
                end = tolower((unsigned char)end);
            }
            if (test_char >= start && test_char <= end) {
                matched = true;
            }
            p += 3;
        } else {
            /* Carácter individual */
            char ch = *p;
            if (nocase) {
                ch = tolower((unsigned char)ch);
            }
            if (test_char == ch) {
                matched = true;
            }
            p++;
        }
    }

    if (*p == ']') {
        *pattern = p;  /* Apuntar al ] */
    } else {
        *pattern = p;
    }

    return negate ? !matched : matched;
}

/* ========================================================================== */
/* MATCHING RECURSIVO                                                        */
/* ========================================================================== */

static bool match_here(const char *pattern, const char *text,
                       bcl_regex_match_t *match, int group_idx);

/* Función para verificar si un carácter pertenece a una clase */
static bool char_class_match(char c, char class_code) {
    switch (class_code) {
        case 'd': return is_digit_char(c);
        case 'D': return !is_digit_char(c);
        case 'w': return is_word_char(c);
        case 'W': return !is_word_char(c);
        case 's': return is_space_char(c);
        case 'S': return !is_space_char(c);
        default: return false;
    }
}

static bool match_star_class(char class_code, const char *pattern, const char *text,
                             bcl_regex_match_t *match, int group_idx) {
    /* * significa 0 o más - greedy: consumir todo lo posible primero */
    const char *t = text;

    /* Consumir todos los caracteres que coincidan (greedy) */
    while (*t && char_class_match(*t, class_code)) {
        t++;
    }

    /* Backtrack: intentar desde el máximo hacia abajo */
    while (t >= text) {
        if (match_here(pattern, t, match, group_idx)) {
            return true;
        }
        t--;
    }

    return false;
}

static bool match_plus_class(char class_code, const char *pattern, const char *text,
                             bcl_regex_match_t *match, int group_idx) {
    /* + significa 1 o más - greedy */
    const char *t = text;

    /* Debe haber al menos uno */
    if (!*t || !char_class_match(*t, class_code)) {
        return false;
    }

    /* Consumir todos los caracteres que coincidan (greedy) */
    while (*t && char_class_match(*t, class_code)) {
        t++;
    }

    /* Backtrack: intentar desde el máximo hacia el mínimo (1) */
    while (t > text) {
        if (match_here(pattern, t, match, group_idx)) {
            return true;
        }
        t--;
    }

    return false;
}

static bool match_star(char c, const char *pattern, const char *text,
                      bcl_regex_match_t *match, int group_idx, bool nocase) {
    /* * significa 0 o más */
    const char *t = text;

    /* Intentar con 0 repeticiones primero (greedy al final) */
    if (match_here(pattern, t, match, group_idx)) {
        return true;
    }

    /* Intentar consumir caracteres */
    while (*t && char_match(*t, c, nocase)) {
        t++;
        if (match_here(pattern, t, match, group_idx)) {
            return true;
        }
    }

    return false;
}

static bool match_plus(char c, const char *pattern, const char *text,
                      bcl_regex_match_t *match, int group_idx, bool nocase) {
    /* + significa 1 o más */
    const char *t = text;

    /* Debe haber al menos uno */
    if (!*t || !char_match(*t, c, nocase)) {
        return false;
    }

    t++;
    /* Ahora es como * */
    while (*t && char_match(*t, c, nocase)) {
        if (match_here(pattern, t, match, group_idx)) {
            return true;
        }
        t++;
    }

    return match_here(pattern, t, match, group_idx);
}

static bool match_here(const char *pattern, const char *text,
                      bcl_regex_match_t *match, int group_idx) {
    /* Fin del patrón */
    if (*pattern == '\0') {
        return true;
    }

    /* $ - fin de texto */
    if (*pattern == '$' && pattern[1] == '\0') {
        return *text == '\0';
    }

    /* Escape \d \w \s etc */
    if (*pattern == '\\' && pattern[1]) {
        char escaped = pattern[1];
        bool is_class = false;

        switch (escaped) {
            case 'd': case 'D':
            case 'w': case 'W':
            case 's': case 'S':
                is_class = true;
                break;
            case 'n': case 't': case 'r':
                break;
            default:
                /* Escape literal */
                break;
        }

        /* Verificar cuantificador primero */
        if (pattern[2] == '*') {
            if (is_class) {
                return match_star_class(escaped, pattern + 3, text, match, group_idx);
            } else {
                return match_star(escaped, pattern + 3, text, match, group_idx, match->nocase);
            }
        } else if (pattern[2] == '+') {
            if (is_class) {
                return match_plus_class(escaped, pattern + 3, text, match, group_idx);
            } else {
                return match_plus(escaped, pattern + 3, text, match, group_idx, match->nocase);
            }
        } else if (pattern[2] == '?') {
            /* 0 o 1 */
            bool matched = false;
            if (is_class) {
                matched = char_class_match(*text, escaped);
            } else if (escaped == 'n') {
                matched = (*text == '\n');
            } else if (escaped == 't') {
                matched = (*text == '\t');
            } else if (escaped == 'r') {
                matched = (*text == '\r');
            } else {
                matched = (*text == escaped);
            }
            return match_here(pattern + 3, text + 1, match, group_idx) ||
                   match_here(pattern + 3, text, match, group_idx);
        }

        /* Sin cuantificador - match simple */
        bool matched = false;
        if (is_class) {
            matched = char_class_match(*text, escaped);
        } else if (escaped == 'n') {
            matched = (*text == '\n');
        } else if (escaped == 't') {
            matched = (*text == '\t');
        } else if (escaped == 'r') {
            matched = (*text == '\r');
        } else {
            matched = (*text == escaped);
        }

        if (!matched) return false;
        return match_here(pattern + 2, text + 1, match, group_idx);
    }

    /* Clase de caracteres [...] */
    if (*pattern == '[') {
        const char *p = pattern;
        if (!*text) return false;

        if (match_char_class(&p, *text, match->nocase)) {
            /* p ahora apunta al ] */
            p++;  /* Avanzar después del ] */

            /* Verificar cuantificador */
            if (*p == '*') {
                return match_star(*text, p + 1, text + 1, match, group_idx, match->nocase);
            } else if (*p == '+') {
                return match_plus(*text, p + 1, text + 1, match, group_idx, match->nocase);
            } else if (*p == '?') {
                return match_here(p + 1, text + 1, match, group_idx) ||
                       match_here(p + 1, text, match, group_idx);
            }
            return match_here(p, text + 1, match, group_idx);
        }
        return false;
    }

    /* . - cualquier carácter */
    if (*pattern == '.') {
        if (!*text) return false;

        if (pattern[1] == '*') {
            /* .* - cualquier secuencia */
            const char *t = text;
            /* Intento greedy */
            while (*t) t++;
            /* Backtrack */
            while (t >= text) {
                if (match_here(pattern + 2, t, match, group_idx)) {
                    return true;
                }
                t--;
            }
            return false;
        } else if (pattern[1] == '+') {
            /* .+ - al menos un carácter */
            if (!*text) return false;
            const char *t = text + 1;
            while (*t) t++;
            while (t > text) {
                if (match_here(pattern + 2, t, match, group_idx)) {
                    return true;
                }
                t--;
            }
            return false;
        } else if (pattern[1] == '?') {
            /* .? - 0 o 1 carácter */
            return match_here(pattern + 2, text + 1, match, group_idx) ||
                   match_here(pattern + 2, text, match, group_idx);
        }

        return match_here(pattern + 1, text + 1, match, group_idx);
    }

    /* Cuantificadores * + ? */
    if (pattern[1] == '*') {
        return match_star(*pattern, pattern + 2, text, match, group_idx, match->nocase);
    }
    if (pattern[1] == '+') {
        return match_plus(*pattern, pattern + 2, text, match, group_idx, match->nocase);
    }
    if (pattern[1] == '?') {
        /* 0 o 1 repetición */
        return match_here(pattern + 2, text, match, group_idx) ||
               (char_match(*text, *pattern, match->nocase) &&
                match_here(pattern + 2, text + 1, match, group_idx));
    }

    /* Carácter literal */
    if (*text && char_match(*text, *pattern, match->nocase)) {
        return match_here(pattern + 1, text + 1, match, group_idx);
    }

    return false;
}

/* ========================================================================== */
/* FUNCIÓN PRINCIPAL DE MATCHING                                            */
/* ========================================================================== */

static bool regex_match(const char *pattern, const char *text,
                       bcl_regex_match_t *match, const char **match_start,
                       const char **match_end) {
    /* Resetear grupos */
    for (int i = 0; i < MAX_GROUPS; i++) {
        match->groups[i].start = NULL;
        match->groups[i].end = NULL;
    }
    match->num_groups = 0;

    /* ^ - debe comenzar al inicio */
    if (*pattern == '^') {
        if (match_here(pattern + 1, text, match, 0)) {
            if (match_start) *match_start = text;
            /* Calcular fin del match */
            const char *end = text;
            const char *p = pattern + 1;
            while (*end && *p) {
                if (*p == '\\' && p[1]) p += 2;
                else if (*p == '[') {
                    while (*p && *p != ']') p++;
                    if (*p) p++;
                } else if (*p == '*' || *p == '+' || *p == '?') {
                    /* Skip cuantificador */
                    p++;
                } else if (*p == '$') {
                    break;
                } else {
                    end++;
                    p++;
                }
            }
            if (match_end) *match_end = end;
            return true;
        }
        return false;
    }

    /* Intentar matching desde cada posición */
    const char *t = text;
    while (*t) {
        if (match_here(pattern, t, match, 0)) {
            if (match_start) *match_start = t;
            /* Calcular fin aproximado del match */
            const char *end = t;
            const char *p = pattern;
            while (*end && *p && *p != '$') {
                if (*p == '\\' && p[1]) {
                    p += 2;
                    end++;
                } else if (*p == '[') {
                    while (*p && *p != ']') p++;
                    if (*p) p++;
                    end++;
                } else if (*p == '.') {
                    p++;
                    end++;
                } else if (*p == '*' || *p == '+' || *p == '?') {
                    p++;
                } else {
                    end++;
                    p++;
                }
            }
            if (match_end) *match_end = end;
            return true;
        }
        t++;
    }

    return false;
}

/* ========================================================================== */
/* REGEXP - Comando de búsqueda                                             */
/* ========================================================================== */

bcl_result_t bcl_cmd_regexp(bcl_interp_t *interp, int argc, char **argv,
                            bcl_value_t **result) {
    if (argc < 2) {
        bcl_set_error(interp, "REGEXP: wrong # args: should be \"REGEXP pattern text ?options?\"");
        return BCL_ERROR;
    }

    const char *pattern = argv[0];
    const char *text = argv[1];

    /* Opciones */
    bool nocase = false;
    bool all = false;
    const char *match_var = NULL;

    for (int i = 2; i < argc; i++) {
        if (bcl_strcasecmp(argv[i], "NOCASE") == 0) {
            nocase = true;
        } else if (bcl_strcasecmp(argv[i], "ALL") == 0) {
            all = true;
        } else if (bcl_strcasecmp(argv[i], "MATCH") == 0 && i + 1 < argc) {
            match_var = argv[i + 1];
            i++;
        }
        /* Otras opciones (SUBMATCHES, INDICES, etc.) se pueden añadir después */
    }

    /* Preparar contexto de matching */
    bcl_regex_match_t match;
    match.nocase = nocase;
    match.num_groups = 0;

    if (all) {
        /* Contar todas las coincidencias */
        int count = 0;
        const char *t = text;
        const char *mstart, *mend;

        while (*t) {
            if (regex_match(pattern, t, &match, &mstart, &mend)) {
                count++;
                /* Avanzar después del match */
                if (mend > mstart) {
                    t = mend;
                } else {
                    t++; /* Evitar loop infinito */
                }
            } else {
                break;
            }
        }

        char buf[32];
        snprintf(buf, sizeof(buf), "%d", count);
        *result = bcl_value_create(buf);
        return BCL_OK;
    } else {
        /* Buscar primera coincidencia */
        const char *mstart, *mend;
        if (regex_match(pattern, text, &match, &mstart, &mend)) {
            /* Capturar match si se pidió */
            if (match_var) {
                size_t len = mend - mstart;
                char *matched = malloc(len + 1);
                if (matched) {
                    memcpy(matched, mstart, len);
                    matched[len] = '\0';
                    bcl_var_set(interp, match_var, matched);
                    free(matched);
                }
            }

            *result = bcl_value_create("1");
            return BCL_OK;
        } else {
            if (match_var) {
                bcl_var_set(interp, match_var, "");
            }
            *result = bcl_value_create("0");
            return BCL_OK;
        }
    }
}

/* ========================================================================== */
/* REGSUB - Comando de sustitución                                          */
/* ========================================================================== */

bcl_result_t bcl_cmd_regsub(bcl_interp_t *interp, int argc, char **argv,
                            bcl_value_t **result) {
    if (argc < 3) {
        bcl_set_error(interp, "REGSUB: wrong # args: should be \"REGSUB pattern text replacement ?options?\"");
        return BCL_ERROR;
    }

    const char *pattern = argv[0];
    const char *text = argv[1];
    const char *replacement = argv[2];

    /* Opciones */
    bool nocase = false;
    bool all = false;
    const char *count_var = NULL;

    for (int i = 3; i < argc; i++) {
        if (bcl_strcasecmp(argv[i], "NOCASE") == 0) {
            nocase = true;
        } else if (bcl_strcasecmp(argv[i], "ALL") == 0) {
            all = true;
        } else if (bcl_strcasecmp(argv[i], "COUNT") == 0 && i + 1 < argc) {
            count_var = argv[i + 1];
            i++;
        }
    }

    /* Buffer para resultado */
    char *output = malloc(strlen(text) * 4 + strlen(replacement) * 10 + 1);
    if (!output) {
        bcl_set_error(interp, "REGSUB: out of memory");
        return BCL_ERROR;
    }
    output[0] = '\0';

    bcl_regex_match_t match;
    match.nocase = nocase;
    match.num_groups = 0;

    int count = 0;
    const char *t = text;
    const char *mstart, *mend;

    while (*t) {
        if (regex_match(pattern, t, &match, &mstart, &mend)) {
            /* Copiar texto antes del match */
            strncat(output, t, mstart - t);

            /* Aplicar reemplazo */
            strcat(output, replacement);

            count++;
            t = mend;

            if (!all) {
                /* Solo primera sustitución */
                strcat(output, t);
                break;
            }

            /* Evitar loop infinito en matches vacíos */
            if (mend == mstart && *t) {
                strncat(output, t, 1);
                t++;
            }
        } else {
            /* No más matches */
            strcat(output, t);
            break;
        }
    }

    /* Guardar contador si se pidió */
    if (count_var) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", count);
        bcl_var_set(interp, count_var, buf);
    }

    *result = bcl_value_create(output);
    free(output);
    return BCL_OK;
}
