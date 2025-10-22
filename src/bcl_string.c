/**
 * @file bcl_string.c
 * @brief Implementación de strings dinámicos
 */

#include "bcl.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* ========================================================================== */
/* STRINGS DINÁMICOS                                                         */
/* ========================================================================== */

bcl_string_t *bcl_string_create(const char *initial) {
    bcl_string_t *str = malloc(sizeof(bcl_string_t));
    if (!str) return NULL;

    size_t init_len = initial ? strlen(initial) : 0;
    str->capacity = init_len + 64;  /* Buffer inicial */
    str->len = init_len;
    str->data = malloc(str->capacity);

    if (!str->data) {
        free(str);
        return NULL;
    }

    if (initial) {
        strcpy(str->data, initial);
    } else {
        str->data[0] = '\0';
    }

    return str;
}

bcl_string_t *bcl_string_create_empty(size_t capacity) {
    bcl_string_t *str = malloc(sizeof(bcl_string_t));
    if (!str) return NULL;

    str->capacity = capacity > 0 ? capacity : 64;
    str->len = 0;
    str->data = malloc(str->capacity);

    if (!str->data) {
        free(str);
        return NULL;
    }

    str->data[0] = '\0';
    return str;
}

void bcl_string_destroy(bcl_string_t *str) {
    if (!str) return;
    free(str->data);
    free(str);
}

static void bcl_string_ensure_capacity(bcl_string_t *str, size_t needed) {
    if (needed <= str->capacity) return;

    size_t new_capacity = str->capacity * 2;
    while (new_capacity < needed) {
        new_capacity *= 2;
    }

    char *new_data = realloc(str->data, new_capacity);
    if (new_data) {
        str->data = new_data;
        str->capacity = new_capacity;
    }
}

void bcl_string_append(bcl_string_t *str, const char *text) {
    if (!str || !text) return;

    size_t text_len = strlen(text);
    bcl_string_ensure_capacity(str, str->len + text_len + 1);

    strcpy(str->data + str->len, text);
    str->len += text_len;
}

void bcl_string_append_char(bcl_string_t *str, char c) {
    if (!str) return;

    bcl_string_ensure_capacity(str, str->len + 2);
    str->data[str->len++] = c;
    str->data[str->len] = '\0';
}

void bcl_string_clear(bcl_string_t *str) {
    if (!str) return;
    str->len = 0;
    str->data[0] = '\0';
}

char *bcl_string_cstr(bcl_string_t *str) {
    return str ? str->data : NULL;
}

/* ========================================================================== */
/* UTILIDADES DE STRINGS                                                     */
/* ========================================================================== */

char *bcl_strdup(const char *str) {
    if (!str) return NULL;
    size_t len = strlen(str);
    char *dup = malloc(len + 1);
    if (dup) {
        strcpy(dup, str);
    }
    return dup;
}

char *bcl_strtolower(const char *str) {
    if (!str) return NULL;

    size_t len = strlen(str);
    char *lower = malloc(len + 1);
    if (!lower) return NULL;

    for (size_t i = 0; i < len; i++) {
        lower[i] = tolower((unsigned char)str[i]);
    }
    lower[len] = '\0';

    return lower;
}

int bcl_strcasecmp(const char *s1, const char *s2) {
    if (!s1 || !s2) return s1 ? 1 : (s2 ? -1 : 0);

    while (*s1 && *s2) {
        int c1 = tolower((unsigned char)*s1);
        int c2 = tolower((unsigned char)*s2);
        if (c1 != c2) return c1 - c2;
        s1++;
        s2++;
    }

    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

int bcl_strncasecmp(const char *s1, const char *s2, size_t n) {
    if (!s1 || !s2) return s1 ? 1 : (s2 ? -1 : 0);
    if (n == 0) return 0;

    while (*s1 && *s2 && n > 0) {
        int c1 = tolower((unsigned char)*s1);
        int c2 = tolower((unsigned char)*s2);
        if (c1 != c2) return c1 - c2;
        s1++;
        s2++;
        n--;
    }

    if (n == 0) return 0;
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

bool bcl_is_number(const char *str) {
    if (!str || !*str) return false;

    /* Permitir signo */
    if (*str == '+' || *str == '-') str++;

    bool has_digit = false;
    bool has_dot = false;
    bool has_exp = false;

    while (*str) {
        if (isdigit((unsigned char)*str)) {
            has_digit = true;
        } else if (*str == '.' && !has_dot && !has_exp) {
            has_dot = true;
        } else if ((*str == 'e' || *str == 'E') && !has_exp && has_digit) {
            has_exp = true;
            str++;
            if (*str == '+' || *str == '-') str++;
            continue;
        } else if (*str == 'x' || *str == 'X') {
            /* Hexadecimal */
            str++;
            while (*str && isxdigit((unsigned char)*str)) str++;
            return *str == '\0';
        } else {
            return false;
        }
        str++;
    }

    return has_digit;
}

double bcl_str_to_number(const char *str, bool *ok) {
    if (!str || !*str) {
        if (ok) *ok = false;
        return 0.0;
    }

    char *endptr;
    double result = strtod(str, &endptr);

    /* Verificar si la conversión fue completa */
    while (*endptr && isspace((unsigned char)*endptr)) endptr++;

    if (ok) *ok = (*endptr == '\0');
    return result;
}
