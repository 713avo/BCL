/**
 * @file bcl_binary.c
 * @brief Comando BINARY - Manipulación de datos binarios
 * @note Inspirado en el comando binary de Tcl
 */

#include "bcl.h"
#include "bcl_commands.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <ctype.h>

/* Determinar endianness en tiempo de compilación */
static inline bool is_little_endian(void) {
    union {
        uint32_t i;
        uint8_t c[4];
    } test = {0x01020304};
    return test.c[0] == 4;
}

/* ========================================================================== */
/* UTILIDADES PARA MANIPULACIÓN DE BYTES                                     */
/* ========================================================================== */

static void write_int16_le(uint8_t *buf, int16_t val) {
    buf[0] = val & 0xFF;
    buf[1] = (val >> 8) & 0xFF;
}

static void write_int16_be(uint8_t *buf, int16_t val) {
    buf[0] = (val >> 8) & 0xFF;
    buf[1] = val & 0xFF;
}

static void write_int32_le(uint8_t *buf, int32_t val) {
    buf[0] = val & 0xFF;
    buf[1] = (val >> 8) & 0xFF;
    buf[2] = (val >> 16) & 0xFF;
    buf[3] = (val >> 24) & 0xFF;
}

static void write_int32_be(uint8_t *buf, int32_t val) {
    buf[0] = (val >> 24) & 0xFF;
    buf[1] = (val >> 16) & 0xFF;
    buf[2] = (val >> 8) & 0xFF;
    buf[3] = val & 0xFF;
}

static int16_t read_int16_le(const uint8_t *buf) {
    return (int16_t)(buf[0] | (buf[1] << 8));
}

static int16_t read_int16_be(const uint8_t *buf) {
    return (int16_t)((buf[0] << 8) | buf[1]);
}

static int32_t read_int32_le(const uint8_t *buf) {
    return (int32_t)(buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24));
}

static int32_t read_int32_be(const uint8_t *buf) {
    return (int32_t)((buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3]);
}

/* ========================================================================== */
/* BINARY FORMAT - Construir cadenas binarias                               */
/* ========================================================================== */

static bcl_result_t binary_format(bcl_interp_t *interp, int argc, char **argv,
                                   bcl_value_t **result) {
    if (argc < 1) {
        bcl_set_error(interp, "BINARY FORMAT: wrong # args: should be \"BINARY FORMAT formatString ?arg ...?\"");
        return BCL_ERROR;
    }

    const char *format = argv[0];
    int arg_idx = 1;

    /* Buffer dinámico para resultado */
    size_t buf_capacity = 256;
    size_t buf_len = 0;
    uint8_t *buf = malloc(buf_capacity);
    if (!buf) {
        bcl_set_error(interp, "Out of memory");
        return BCL_ERROR;
    }

    /* Procesar formato */
    const char *p = format;
    while (*p) {
        /* Saltar espacios */
        while (*p && isspace(*p)) p++;
        if (!*p) break;

        char type = *p++;

        /* Obtener conteo (opcional) */
        int count = 1;
        bool has_count = false;
        if (*p == '*') {
            count = -1;  /* Usar todo el argumento */
            has_count = true;
            p++;
        } else if (isdigit(*p)) {
            count = 0;
            while (isdigit(*p)) {
                count = count * 10 + (*p - '0');
                p++;
            }
            has_count = true;
        }

        /* Asegurar espacio en buffer */
        size_t needed = buf_len + 1024;  /* Estimación conservadora */
        if (needed > buf_capacity) {
            buf_capacity = needed * 2;
            buf = realloc(buf, buf_capacity);
            if (!buf) {
                bcl_set_error(interp, "Out of memory");
                return BCL_ERROR;
            }
        }

        /* Procesar según tipo */
        switch (type) {
            case 'a':  /* ASCII string, null padding */
            case 'A':  /* ASCII string, space padding */
            {
                if (arg_idx >= argc) {
                    bcl_set_error(interp, "BINARY FORMAT: not enough arguments");
                    free(buf);
                    return BCL_ERROR;
                }

                const char *str = argv[arg_idx++];
                size_t str_len = strlen(str);

                if (!has_count) count = 1;
                if (count == -1) count = str_len;

                /* Copiar string */
                size_t copy_len = (str_len < (size_t)count) ? str_len : (size_t)count;
                memcpy(buf + buf_len, str, copy_len);
                buf_len += copy_len;

                /* Padding */
                char pad = (type == 'a') ? '\0' : ' ';
                for (size_t i = copy_len; i < (size_t)count; i++) {
                    buf[buf_len++] = pad;
                }
                break;
            }

            case 'c':  /* 8-bit integer */
            {
                if (!has_count) count = 1;

                if (arg_idx >= argc) {
                    bcl_set_error(interp, "BINARY FORMAT: not enough arguments");
                    free(buf);
                    return BCL_ERROR;
                }

                const char *str = argv[arg_idx++];

                /* Parsear como lista de enteros */
                char *str_copy = bcl_strdup(str);
                char *saveptr = NULL;
                char *token = strtok_r(str_copy, " \t\n", &saveptr);

                int i = 0;
                while (token && (count == -1 || i < count)) {
                    bool ok;
                    double val = bcl_str_to_number(token, &ok);
                    if (ok) {
                        buf[buf_len++] = (uint8_t)((int)val & 0xFF);
                        i++;
                    }
                    token = strtok_r(NULL, " \t\n", &saveptr);
                }

                free(str_copy);
                break;
            }

            case 's':  /* 16-bit integer, little-endian */
            case 'S':  /* 16-bit integer, big-endian */
            {
                if (!has_count) count = 1;

                if (arg_idx >= argc) {
                    bcl_set_error(interp, "BINARY FORMAT: not enough arguments");
                    free(buf);
                    return BCL_ERROR;
                }

                const char *str = argv[arg_idx++];
                char *str_copy = bcl_strdup(str);
                char *saveptr = NULL;
                char *token = strtok_r(str_copy, " \t\n", &saveptr);

                int i = 0;
                while (token && (count == -1 || i < count)) {
                    bool ok;
                    double val = bcl_str_to_number(token, &ok);
                    if (ok) {
                        int16_t ival = (int16_t)val;
                        if (type == 's') {
                            write_int16_le(buf + buf_len, ival);
                        } else {
                            write_int16_be(buf + buf_len, ival);
                        }
                        buf_len += 2;
                        i++;
                    }
                    token = strtok_r(NULL, " \t\n", &saveptr);
                }

                free(str_copy);
                break;
            }

            case 'i':  /* 32-bit integer, little-endian */
            case 'I':  /* 32-bit integer, big-endian */
            {
                if (!has_count) count = 1;

                if (arg_idx >= argc) {
                    bcl_set_error(interp, "BINARY FORMAT: not enough arguments");
                    free(buf);
                    return BCL_ERROR;
                }

                const char *str = argv[arg_idx++];
                char *str_copy = bcl_strdup(str);
                char *saveptr = NULL;
                char *token = strtok_r(str_copy, " \t\n", &saveptr);

                int i = 0;
                while (token && (count == -1 || i < count)) {
                    bool ok;
                    double val = bcl_str_to_number(token, &ok);
                    if (ok) {
                        int32_t ival = (int32_t)val;
                        if (type == 'i') {
                            write_int32_le(buf + buf_len, ival);
                        } else {
                            write_int32_be(buf + buf_len, ival);
                        }
                        buf_len += 4;
                        i++;
                    }
                    token = strtok_r(NULL, " \t\n", &saveptr);
                }

                free(str_copy);
                break;
            }

            case 'H':  /* Hex digits, high-to-low */
            case 'h':  /* Hex digits, low-to-high */
            {
                if (arg_idx >= argc) {
                    bcl_set_error(interp, "BINARY FORMAT: not enough arguments");
                    free(buf);
                    return BCL_ERROR;
                }

                const char *str = argv[arg_idx++];
                size_t str_len = strlen(str);

                if (!has_count) count = 1;
                if (count == -1) count = str_len;

                for (int i = 0; i < count && i < (int)str_len; i += 2) {
                    uint8_t byte = 0;

                    if (type == 'H') {
                        /* High nibble primero */
                        if (i < (int)str_len && isxdigit(str[i])) {
                            int val = isdigit(str[i]) ? (str[i] - '0') :
                                     (tolower(str[i]) - 'a' + 10);
                            byte = val << 4;
                        }
                        if (i + 1 < (int)str_len && isxdigit(str[i+1])) {
                            int val = isdigit(str[i+1]) ? (str[i+1] - '0') :
                                     (tolower(str[i+1]) - 'a' + 10);
                            byte |= val;
                        }
                    } else {
                        /* Low nibble primero */
                        if (i < (int)str_len && isxdigit(str[i])) {
                            int val = isdigit(str[i]) ? (str[i] - '0') :
                                     (tolower(str[i]) - 'a' + 10);
                            byte = val;
                        }
                        if (i + 1 < (int)str_len && isxdigit(str[i+1])) {
                            int val = isdigit(str[i+1]) ? (str[i+1] - '0') :
                                     (tolower(str[i+1]) - 'a' + 10);
                            byte |= val << 4;
                        }
                    }

                    buf[buf_len++] = byte;
                }
                break;
            }

            case 'x':  /* Null byte */
            {
                if (!has_count) count = 1;
                for (int i = 0; i < count; i++) {
                    buf[buf_len++] = 0;
                }
                break;
            }

            case 'X':  /* Backup */
            {
                if (!has_count) count = 1;
                if (count > (int)buf_len) count = buf_len;
                buf_len -= count;
                break;
            }

            case '@':  /* Absolute position */
            {
                if (!has_count) count = 0;
                if (count < 0) count = 0;
                buf_len = count;
                /* Asegurar que hay espacio */
                if (buf_len > buf_capacity) {
                    buf_capacity = buf_len + 256;
                    buf = realloc(buf, buf_capacity);
                }
                break;
            }

            default:
                bcl_set_error(interp, "BINARY FORMAT: bad field specifier '%c'", type);
                free(buf);
                return BCL_ERROR;
        }
    }

    /* Crear resultado como string que puede contener bytes nulos */
    bcl_string_t *res = bcl_string_create_empty(buf_len + 1);
    for (size_t i = 0; i < buf_len; i++) {
        bcl_string_append_char(res, buf[i]);
    }

    *result = bcl_value_create(bcl_string_cstr(res));
    bcl_string_destroy(res);
    free(buf);

    return BCL_OK;
}

/* ========================================================================== */
/* BINARY SCAN - Extraer campos de cadenas binarias                         */
/* ========================================================================== */

static bcl_result_t binary_scan(bcl_interp_t *interp, int argc, char **argv,
                                 bcl_value_t **result) {
    if (argc < 2) {
        bcl_set_error(interp, "BINARY SCAN: wrong # args: should be \"BINARY SCAN string formatString ?varName ...?\"");
        return BCL_ERROR;
    }

    const char *data = argv[0];
    size_t data_len = strlen(data);
    const char *format = argv[1];
    int var_idx = 2;

    size_t pos = 0;  /* Posición actual en data */
    int conversions = 0;  /* Contador de conversiones exitosas */

    /* Procesar formato */
    const char *p = format;
    while (*p) {
        /* Saltar espacios */
        while (*p && isspace(*p)) p++;
        if (!*p) break;

        char type = *p++;

        /* Obtener conteo */
        int count = 1;
        bool has_count = false;
        if (*p == '*') {
            count = -1;
            has_count = true;
            p++;
        } else if (isdigit(*p)) {
            count = 0;
            while (isdigit(*p)) {
                count = count * 10 + (*p - '0');
                p++;
            }
            has_count = true;
        }

        /* Procesar según tipo */
        switch (type) {
            case 'a':  /* ASCII string */
            case 'A':  /* ASCII string, trim spaces/nulls */
            {
                if (var_idx >= argc) {
                    bcl_set_error(interp, "BINARY SCAN: not enough variables");
                    return BCL_ERROR;
                }

                if (!has_count) count = 1;
                if (count == -1) count = data_len - pos;

                if (pos + count > data_len) {
                    count = data_len - pos;
                }

                bcl_string_t *str = bcl_string_create_empty(count + 1);
                for (int i = 0; i < count && pos < data_len; i++, pos++) {
                    bcl_string_append_char(str, data[pos]);
                }

                char *value = bcl_strdup(bcl_string_cstr(str));
                bcl_string_destroy(str);

                /* Para tipo 'A', eliminar espacios y nulos finales */
                if (type == 'A') {
                    int len = strlen(value);
                    while (len > 0 && (value[len-1] == ' ' || value[len-1] == '\0')) {
                        value[--len] = '\0';
                    }
                }

                bcl_var_set(interp, argv[var_idx], value);
                free(value);
                var_idx++;
                conversions++;
                break;
            }

            case 'c':  /* 8-bit integers */
            {
                if (var_idx >= argc) {
                    bcl_set_error(interp, "BINARY SCAN: not enough variables");
                    return BCL_ERROR;
                }

                if (!has_count) count = 1;
                if (count == -1) count = data_len - pos;

                bcl_string_t *list = bcl_string_create_empty(256);
                for (int i = 0; i < count && pos < data_len; i++, pos++) {
                    int8_t val = (int8_t)((uint8_t)data[pos]);
                    char num[32];
                    snprintf(num, sizeof(num), "%d", val);
                    if (bcl_string_cstr(list)[0] != '\0') {
                        bcl_string_append(list, " ");
                    }
                    bcl_string_append(list, num);
                }

                bcl_var_set(interp, argv[var_idx], bcl_string_cstr(list));
                bcl_string_destroy(list);
                var_idx++;
                conversions++;
                break;
            }

            case 's':  /* 16-bit integer, little-endian */
            case 'S':  /* 16-bit integer, big-endian */
            {
                if (var_idx >= argc) {
                    bcl_set_error(interp, "BINARY SCAN: not enough variables");
                    return BCL_ERROR;
                }

                if (!has_count) count = 1;
                if (count == -1) count = (data_len - pos) / 2;

                bcl_string_t *list = bcl_string_create_empty(256);
                for (int i = 0; i < count && pos + 1 < data_len; i++, pos += 2) {
                    int16_t val;
                    if (type == 's') {
                        val = read_int16_le((const uint8_t*)(data + pos));
                    } else {
                        val = read_int16_be((const uint8_t*)(data + pos));
                    }

                    char num[32];
                    snprintf(num, sizeof(num), "%d", val);
                    if (bcl_string_cstr(list)[0] != '\0') {
                        bcl_string_append(list, " ");
                    }
                    bcl_string_append(list, num);
                }

                bcl_var_set(interp, argv[var_idx], bcl_string_cstr(list));
                bcl_string_destroy(list);
                var_idx++;
                conversions++;
                break;
            }

            case 'i':  /* 32-bit integer, little-endian */
            case 'I':  /* 32-bit integer, big-endian */
            {
                if (var_idx >= argc) {
                    bcl_set_error(interp, "BINARY SCAN: not enough variables");
                    return BCL_ERROR;
                }

                if (!has_count) count = 1;
                if (count == -1) count = (data_len - pos) / 4;

                bcl_string_t *list = bcl_string_create_empty(256);
                for (int i = 0; i < count && pos + 3 < data_len; i++, pos += 4) {
                    int32_t val;
                    if (type == 'i') {
                        val = read_int32_le((const uint8_t*)(data + pos));
                    } else {
                        val = read_int32_be((const uint8_t*)(data + pos));
                    }

                    char num[32];
                    snprintf(num, sizeof(num), "%d", val);
                    if (bcl_string_cstr(list)[0] != '\0') {
                        bcl_string_append(list, " ");
                    }
                    bcl_string_append(list, num);
                }

                bcl_var_set(interp, argv[var_idx], bcl_string_cstr(list));
                bcl_string_destroy(list);
                var_idx++;
                conversions++;
                break;
            }

            case 'H':  /* Hex digits, high-to-low */
            {
                if (var_idx >= argc) {
                    bcl_set_error(interp, "BINARY SCAN: not enough variables");
                    return BCL_ERROR;
                }

                if (!has_count) count = 1;
                if (count == -1) count = (data_len - pos) * 2;

                bcl_string_t *hex = bcl_string_create_empty(count + 1);
                int nibbles = 0;
                while (nibbles < count && pos < data_len) {
                    uint8_t byte = (uint8_t)data[pos];

                    if (nibbles < count) {
                        char c = "0123456789abcdef"[(byte >> 4) & 0xF];
                        bcl_string_append_char(hex, c);
                        nibbles++;
                    }

                    if (nibbles < count) {
                        char c = "0123456789abcdef"[byte & 0xF];
                        bcl_string_append_char(hex, c);
                        nibbles++;
                    }

                    pos++;
                }

                bcl_var_set(interp, argv[var_idx], bcl_string_cstr(hex));
                bcl_string_destroy(hex);
                var_idx++;
                conversions++;
                break;
            }

            case 'x':  /* Skip bytes */
            {
                if (!has_count) count = 1;
                pos += count;
                if (pos > data_len) pos = data_len;
                break;
            }

            case 'X':  /* Backup */
            {
                if (!has_count) count = 1;
                if (count > (int)pos) count = pos;
                pos -= count;
                break;
            }

            case '@':  /* Absolute position */
            {
                if (!has_count) count = 0;
                if (count < 0) count = 0;
                pos = (size_t)count;
                if (pos > data_len) pos = data_len;
                break;
            }

            default:
                bcl_set_error(interp, "BINARY SCAN: bad field specifier '%c'", type);
                return BCL_ERROR;
        }
    }

    /* Retornar número de conversiones */
    char num[32];
    snprintf(num, sizeof(num), "%d", conversions);
    *result = bcl_value_create(num);

    return BCL_OK;
}

/* ========================================================================== */
/* BINARY - Comando principal                                                */
/* ========================================================================== */

bcl_result_t bcl_cmd_binary(bcl_interp_t *interp, int argc, char **argv,
                             bcl_value_t **result) {
    if (argc < 1) {
        bcl_set_error(interp, "BINARY: wrong # args: should be \"BINARY option ...\"");
        return BCL_ERROR;
    }

    const char *option = argv[0];

    if (bcl_strcasecmp(option, "FORMAT") == 0) {
        return binary_format(interp, argc - 1, argv + 1, result);
    } else if (bcl_strcasecmp(option, "SCAN") == 0) {
        return binary_scan(interp, argc - 1, argv + 1, result);
    } else {
        bcl_set_error(interp, "BINARY: bad option \"%s\": must be FORMAT or SCAN", option);
        return BCL_ERROR;
    }
}
