/**
 * @file bcl_format.c
 * @brief Implementación de FORMAT y SCAN
 * @version 1.0
 *
 * Comandos tipo printf/scanf para formateo de I/O
 */

#include "bcl.h"
#include "bcl_commands.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>

/* ========================================================================== */
/* FORMAT - Formateo estilo printf                                           */
/* ========================================================================== */

/**
 * @brief Procesa un especificador de formato y retorna el valor formateado
 */
static char *format_specifier(const char *spec, const char *arg, size_t *consumed) {
    static char buffer[4096];
    char format[64];
    int spec_len = 0;

    /* Construir el especificador de formato completo */
    format[spec_len++] = '%';
    const char *p = spec + 1; /* Saltar el % inicial */

    /* Flags: -, +, espacio, 0, # */
    while (*p && strchr("-+ 0#", *p)) {
        format[spec_len++] = *p++;
    }

    /* Ancho */
    while (*p && isdigit((unsigned char)*p)) {
        format[spec_len++] = *p++;
    }

    /* Precisión */
    if (*p == '.') {
        format[spec_len++] = *p++;
        while (*p && isdigit((unsigned char)*p)) {
            format[spec_len++] = *p++;
        }
    }

    /* Tipo de conversión */
    if (!*p) {
        /* Error: falta tipo */
        strcpy(buffer, "%");
        *consumed = 1;
        return buffer;
    }

    char conv = *p++;
    format[spec_len++] = conv;
    format[spec_len] = '\0';

    *consumed = p - spec;

    /* Aplicar formato según tipo */
    switch (conv) {
        case 'd':
        case 'i': {
            long val = atol(arg);
            snprintf(buffer, sizeof(buffer), format, val);
            break;
        }
        case 'u':
        case 'o':
        case 'x':
        case 'X': {
            unsigned long val = strtoul(arg, NULL, 10);
            snprintf(buffer, sizeof(buffer), format, val);
            break;
        }
        case 'f':
        case 'F':
        case 'e':
        case 'E':
        case 'g':
        case 'G': {
            double val = atof(arg);
            snprintf(buffer, sizeof(buffer), format, val);
            break;
        }
        case 's': {
            snprintf(buffer, sizeof(buffer), format, arg);
            break;
        }
        case 'c': {
            char ch = arg[0];
            snprintf(buffer, sizeof(buffer), format, ch);
            break;
        }
        case '%': {
            strcpy(buffer, "%");
            break;
        }
        default: {
            /* Tipo desconocido */
            snprintf(buffer, sizeof(buffer), "%%%c", conv);
            break;
        }
    }

    return buffer;
}

bcl_result_t bcl_cmd_format(bcl_interp_t *interp, int argc, char **argv,
                            bcl_value_t **result) {
    if (argc < 1) {
        bcl_set_error(interp, "wrong # args: should be \"FORMAT template arg1 arg2 ...\"");
        return BCL_ERROR;
    }

    const char *template = argv[0];
    int arg_index = 1;

    bcl_string_t *output = bcl_string_create("");
    const char *p = template;

    while (*p) {
        if (*p == '%') {
            if (*(p + 1) == '%') {
                /* %% -> % literal */
                bcl_string_append_char(output, '%');
                p += 2;
            } else {
                /* Especificador de formato */
                if (arg_index >= argc) {
                    /* No hay más argumentos */
                    bcl_set_error(interp, "not enough arguments for format string");
                    bcl_string_destroy(output);
                    return BCL_ERROR;
                }

                size_t consumed;
                char *formatted = format_specifier(p, argv[arg_index], &consumed);
                bcl_string_append(output, formatted);

                p += consumed;
                arg_index++;
            }
        } else {
            /* Carácter literal */
            bcl_string_append_char(output, *p);
            p++;
        }
    }

    if (result) {
        *result = bcl_value_create(bcl_string_cstr(output));
    }

    bcl_string_destroy(output);
    return BCL_OK;
}

/* ========================================================================== */
/* SCAN - Parseo estilo scanf                                                */
/* ========================================================================== */

/**
 * @brief Extrae un valor según especificador de formato
 * @param next_char El siguiente carácter después del especificador en el template (o '\0' si es el último)
 */
static int scan_specifier(const char *text, const char *spec, char *output,
                          size_t output_size, size_t *text_consumed,
                          size_t *spec_consumed, char next_char) {
    const char *p = spec + 1; /* Saltar % */
    const char *spec_start = spec;
    int width = -1;

    /* Ancho máximo */
    if (isdigit((unsigned char)*p)) {
        width = atoi(p);
        while (isdigit((unsigned char)*p)) p++;
    }

    /* Tipo de conversión */
    char conv = *p;
    if (conv && conv != '[') p++; /* Avanzar past el tipo (excepto para [) */

    /* Saltar espacios en blanco iniciales (excepto para %c y %[) */
    const char *t = text;
    if (conv != 'c' && conv != '[') {
        while (*t && isspace((unsigned char)*t)) t++;
    }

    size_t consumed = 0;
    int matched = 0;

    switch (conv) {
        case 'd':
        case 'i': {
            /* Entero decimal */
            long val;
            char temp[128];
            int i = 0;

            /* Signo opcional */
            if (*t == '+' || *t == '-') {
                temp[i++] = *t++;
                consumed++;
            }

            /* Dígitos */
            while (*t && isdigit((unsigned char)*t) && (width < 0 || consumed < (size_t)width)) {
                temp[i++] = *t++;
                consumed++;
            }

            if (i > 0 && (i > 1 || (temp[0] != '+' && temp[0] != '-'))) {
                temp[i] = '\0';
                val = atol(temp);
                snprintf(output, output_size, "%ld", val);
                matched = 1;
            }
            break;
        }

        case 'u':
        case 'o':
        case 'x':
        case 'X': {
            /* Entero sin signo */
            unsigned long val;
            char temp[128];
            int i = 0;
            int base = (conv == 'o') ? 8 : (conv == 'x' || conv == 'X') ? 16 : 10;

            while (*t && (width < 0 || consumed < (size_t)width)) {
                if ((base == 10 && isdigit((unsigned char)*t)) ||
                    (base == 8 && *t >= '0' && *t <= '7') ||
                    (base == 16 && isxdigit((unsigned char)*t))) {
                    temp[i++] = *t++;
                    consumed++;
                } else {
                    break;
                }
            }

            if (i > 0) {
                temp[i] = '\0';
                val = strtoul(temp, NULL, base);
                snprintf(output, output_size, "%lu", val);
                matched = 1;
            }
            break;
        }

        case 'f':
        case 'e':
        case 'E':
        case 'g':
        case 'G': {
            /* Flotante */
            double val;
            char temp[128];
            int i = 0;

            /* Signo */
            if (*t == '+' || *t == '-') {
                temp[i++] = *t++;
                consumed++;
            }

            /* Dígitos y punto decimal */
            bool has_dot = false;
            bool has_exp = false;

            while (*t && (width < 0 || consumed < (size_t)width)) {
                if (isdigit((unsigned char)*t)) {
                    temp[i++] = *t++;
                    consumed++;
                } else if (*t == '.' && !has_dot && !has_exp) {
                    temp[i++] = *t++;
                    consumed++;
                    has_dot = true;
                } else if ((*t == 'e' || *t == 'E') && !has_exp && i > 0) {
                    temp[i++] = *t++;
                    consumed++;
                    has_exp = true;
                    /* Signo del exponente */
                    if (*t == '+' || *t == '-') {
                        temp[i++] = *t++;
                        consumed++;
                    }
                } else {
                    break;
                }
            }

            if (i > 0) {
                temp[i] = '\0';
                val = atof(temp);
                snprintf(output, output_size, "%g", val);
                matched = 1;
            }
            break;
        }

        case 's': {
            /* String - comportamiento depende del contexto */
            int i = 0;

            /* Si estamos al final del template (next_char == '\0'),
             * capturar todo lo que queda (incluyendo espacios) */
            bool capture_all = (next_char == '\0');

            if (capture_all) {
                /* Capturar todo hasta el final */
                while (*t && (width < 0 || i < width)) {
                    if (i < (int)output_size - 1) {
                        output[i++] = *t;
                    }
                    t++;
                    consumed++;
                }
            } else {
                /* Comportamiento estándar: hasta espacio o next_char */
                while (*t && !isspace((unsigned char)*t) && *t != next_char &&
                       (width < 0 || i < width)) {
                    if (i < (int)output_size - 1) {
                        output[i++] = *t;
                    }
                    t++;
                    consumed++;
                }
            }

            output[i] = '\0';
            if (i > 0) matched = 1;
            break;
        }

        case 'c': {
            /* Carácter */
            if (*t) {
                output[0] = *t;
                output[1] = '\0';
                consumed = 1;
                matched = 1;
            }
            break;
        }

        case '[': {
            /* Set de caracteres %[^,] */
            p++; /* Saltar [ */
            bool negate = false;
            if (*p == '^') {
                negate = true;
                p++;
            }

            /* Construir set */
            char set[256];
            int set_len = 0;
            while (*p && *p != ']') {
                set[set_len++] = *p++;
            }
            if (*p == ']') p++; /* Saltar ] final */
            set[set_len] = '\0';

            /* Leer caracteres que coincidan */
            int i = 0;
            while (*t && (width < 0 || i < width)) {
                bool in_set = (strchr(set, *t) != NULL);
                if ((negate && !in_set) || (!negate && in_set)) {
                    if (i < (int)output_size - 1) {
                        output[i++] = *t;
                    }
                    t++;
                    consumed++;
                } else {
                    break;
                }
            }

            output[i] = '\0';
            if (i > 0) matched = 1;
            break;
        }

        default:
            /* Tipo no soportado */
            output[0] = '\0';
            break;
    }

    *text_consumed = (t - text);
    *spec_consumed = (p - spec_start);
    return matched;
}

bcl_result_t bcl_cmd_scan(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result) {
    if (argc < 2) {
        bcl_set_error(interp, "wrong # args: should be \"SCAN string template var1 var2 ...\"");
        return BCL_ERROR;
    }

    const char *text = argv[0];
    const char *template = argv[1];
    int var_index = 2;
    int matched_count = 0;

    const char *p = template;
    const char *t = text;

    while (*p && *t) {
        if (*p == '%' && *(p + 1) != '%') {
            /* Especificador de formato */
            if (var_index >= argc) {
                /* No hay más variables */
                break;
            }

            char value[1024];
            size_t text_consumed, spec_consumed;

            /* Determinar el siguiente carácter en el template después del especificador */
            const char *spec_end = p + 1;
            while (*spec_end && *spec_end != '%' && !isspace((unsigned char)*spec_end) &&
                   strchr("diouxXeEfFgGsc[", *spec_end) == NULL) {
                spec_end++;
            }
            if (*spec_end && strchr("diouxXeEfFgGsc[", *spec_end)) {
                spec_end++; /* Avanzar past el tipo */
            }
            char next_char = *spec_end;

            int matched = scan_specifier(t, p, value, sizeof(value), &text_consumed, &spec_consumed, next_char);

            if (matched) {
                /* Asignar a variable */
                bcl_var_set(interp, argv[var_index], value);
                matched_count++;
                var_index++;
            }

            t += text_consumed;
            p += spec_consumed;
        } else if (*p == '%' && *(p + 1) == '%') {
            /* %% literal */
            if (*t == '%') {
                t++;
                p += 2;
            } else {
                /* No coincide */
                break;
            }
        } else if (isspace((unsigned char)*p)) {
            /* Espacio en template: saltar espacios en texto */
            while (*t && isspace((unsigned char)*t)) t++;
            while (*p && isspace((unsigned char)*p)) p++;
        } else {
            /* Carácter literal: debe coincidir */
            if (*p == *t) {
                p++;
                t++;
            } else {
                /* No coincide */
                break;
            }
        }
    }

    /* Retornar número de valores leídos */
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", matched_count);

    if (result) {
        *result = bcl_value_create(buf);
    }

    return BCL_OK;
}
