/**
 * @file bcl_parser.c
 * @brief Parser y tokenizador BCL
 */

#include "bcl.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* ========================================================================== */
/* UTILIDADES DE PARSING                                                     */
/* ========================================================================== */

/**
 * @brief Verifica si un carácter necesita escape
 */
static bool needs_escape(char c) {
    return (c == 'n' || c == 't' || c == 'r' || c == 'a' ||
            c == 'b' || c == 'f' || c == '\\' || c == '"' ||
            c == '\'' || c == 'u' ||
            c == 'd' || c == 'D' || c == 'w' || c == 'W' ||
            c == 's' || c == 'S' || c == '[' || c == ']');
}

/**
 * @brief Procesa secuencias de escape
 */
static char process_escape(char c) {
    switch (c) {
        case 'n': return '\n';
        case 't': return '\t';
        case 'r': return '\r';
        case 'a': return '\a';
        case 'b': return '\b';
        case 'f': return '\f';
        case '\\': return '\\';
        case '"': return '"';
        case '\'': return '\'';
        /* Para regex: preservar el backslash */
        case 'd': case 'D':
        case 'w': case 'W':
        case 's': case 'S':
        case '[': case ']':
            return c;  /* Devolver solo el carácter, el backslash se maneja en decode_escapes */
        default: return c;
    }
}

/**
 * @brief Decodifica secuencias de escape en un string
 */
char *bcl_decode_escapes(const char *str) {
    if (!str) return NULL;

    size_t len = strlen(str);
    char *result = malloc(len * 2 + 1);  /* Espacio extra para preservar backslashes */
    if (!result) return NULL;

    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        if (str[i] == '\\' && i + 1 < len) {
            if (str[i+1] == 'u' && i + 5 < len) {
                /* TODO: Procesar \uXXXX Unicode */
                /* Por ahora, solo copiar literal */
                result[j++] = str[i];
            } else if (str[i+1] == 'd' || str[i+1] == 'D' ||
                       str[i+1] == 'w' || str[i+1] == 'W' ||
                       str[i+1] == 's' || str[i+1] == 'S' ||
                       str[i+1] == '[' || str[i+1] == ']') {
                /* Preservar backslash para regex */
                result[j++] = '\\';
                result[j++] = str[i+1];
                i++;
            } else if (needs_escape(str[i+1])) {
                result[j++] = process_escape(str[i+1]);
                i++; /* Saltar el carácter escapado */
            } else {
                result[j++] = str[i];
            }
        } else {
            result[j++] = str[i];
        }
    }

    result[j] = '\0';
    return result;
}

/* ========================================================================== */
/* TOKENIZADOR                                                               */
/* ========================================================================== */

/**
 * @brief Salta espacios en blanco
 */
static const char *skip_whitespace(const char *str) {
    while (*str && isspace((unsigned char)*str)) {
        str++;
    }
    return str;
}

/**
 * @brief Verifica si estamos al inicio de un comentario
 */
static bool is_comment(const char *str) {
    str = skip_whitespace(str);
    return *str == '#';
}

/**
 * @brief Extrae el próximo token de una línea
 * @param line Línea a parsear
 * @param pos Posición actual (se actualiza)
 * @param token Buffer donde guardar el token
 * @param token_size Tamaño del buffer
 * @return true si se extrajo un token, false si llegamos al final
 */
bool bcl_next_token(const char *line, size_t *pos, char *token, size_t token_size) {
    if (!line || !pos || !token || token_size == 0) return false;

    size_t p = *pos;  /* Usar variable local para evitar desreferencias múltiples */

    /* Saltar espacios */
    while (line[p] && isspace((unsigned char)line[p])) {
        p++;
    }

    /* Fin de línea o comentario */
    if (!line[p] || line[p] == '#') {
        *pos = p;
        return false;
    }

    size_t token_idx = 0;

    /* Comillas dobles */
    if (line[p] == '"') {
        p++; /* Saltar comilla inicial */
        while (line[p] && line[p] != '"' && token_idx < token_size - 1) {
            if (line[p] == '\\' && line[p + 1] && token_idx < token_size - 2) {
                /* Escape - copiar ambos caracteres */
                token[token_idx++] = line[p++];
                if (token_idx < token_size - 1) {
                    token[token_idx++] = line[p++];
                }
            } else {
                token[token_idx++] = line[p++];
            }
        }
        if (line[p] == '"') p++; /* Saltar comilla final */
        token[token_idx] = '\0';
        *pos = p;
        return true;
    }

    /* Comillas simples */
    if (line[p] == '\'') {
        p++; /* Saltar comilla inicial */
        while (line[p] && line[p] != '\'' && token_idx < token_size - 1) {
            token[token_idx++] = line[p++];
        }
        if (line[p] == '\'') p++; /* Saltar comilla final */
        token[token_idx] = '\0';
        *pos = p;
        return true;
    }

    /* Corchetes [ ] para subcomandos */
    if (line[p] == '[') {
        int bracket_count = 1;
        p++; /* Saltar [ inicial */

        while (line[p] && bracket_count > 0 && token_idx < token_size - 1) {
            if (line[p] == '[') bracket_count++;
            else if (line[p] == ']') bracket_count--;

            if (bracket_count > 0) {
                token[token_idx++] = line[p];
            }
            p++;
        }
        token[token_idx] = '\0';
        *pos = p;
        return true;
    }

    /* Token normal (hasta espacio o fin de línea) */
    while (line[p] && !isspace((unsigned char)line[p]) &&
           line[p] != '#' && token_idx < token_size - 1) {
        /* Detener en [ o ] */
        if (line[p] == '[' || line[p] == ']') break;

        token[token_idx++] = line[p++];
    }

    token[token_idx] = '\0';
    *pos = p;
    return token_idx > 0;
}

/* ========================================================================== */
/* EXPANSIÓN DE VARIABLES                                                    */
/* ========================================================================== */

/**
 * @brief Expande variables $var en un string
 */
char *bcl_expand_vars(bcl_interp_t *interp, const char *str) {
    if (!str) return bcl_strdup("");
    if (!interp) return bcl_strdup(str);

    bcl_string_t *result = bcl_string_create_empty(strlen(str) * 2);
    if (!result) return NULL;

    size_t i = 0;
    while (str[i]) {
        if (str[i] == '$' && str[i+1]) {
            /* Extraer nombre de variable */
            i++; /* Saltar $ */
            char varname[256];
            size_t j = 0;

            while (str[i] && (isalnum((unsigned char)str[i]) || str[i] == '_')
                   && j < sizeof(varname) - 1) {
                varname[j++] = str[i++];
            }
            varname[j] = '\0';

            /* Verificar si es un array: nombre(índice) */
            if (str[i] == '(') {
                i++; /* Saltar ( */

                /* Extraer el índice hasta ) */
                char index_str[256];
                size_t k = 0;
                int paren_depth = 1;

                while (str[i] && paren_depth > 0 && k < sizeof(index_str) - 1) {
                    if (str[i] == '(') {
                        paren_depth++;
                    } else if (str[i] == ')') {
                        paren_depth--;
                        if (paren_depth == 0) break;
                    }
                    index_str[k++] = str[i++];
                }
                index_str[k] = '\0';

                if (str[i] == ')') {
                    i++; /* Saltar ) */
                }

                /* Expandir variables en el índice (soporta $array($var)) */
                char *expanded_index = bcl_expand_vars(interp, index_str);
                if (expanded_index) {
                    /* Construir nombre completo: nombre(índice) */
                    char full_varname[512];
                    snprintf(full_varname, sizeof(full_varname), "%s(%s)",
                             varname, expanded_index);
                    free(expanded_index);

                    /* Obtener valor del array */
                    bcl_value_t *val = bcl_var_get(interp, full_varname);
                    if (val) {
                        bcl_string_append(result, bcl_value_get(val));
                    }
                } else {
                    /* Si falla la expansión, construir con índice original */
                    char full_varname[512];
                    snprintf(full_varname, sizeof(full_varname), "%s(%s)",
                             varname, index_str);

                    bcl_value_t *val = bcl_var_get(interp, full_varname);
                    if (val) {
                        bcl_string_append(result, bcl_value_get(val));
                    }
                }
            } else {
                /* Variable normal (no array) */
                bcl_value_t *val = bcl_var_get(interp, varname);
                if (val) {
                    bcl_string_append(result, bcl_value_get(val));
                }
            }
            /* Si no existe, se reemplaza por cadena vacía */

        } else {
            bcl_string_append_char(result, str[i++]);
        }
    }

    char *final = bcl_strdup(bcl_string_cstr(result));
    bcl_string_destroy(result);
    return final;
}

/* ========================================================================== */
/* EVALUACIÓN DE SUBCOMANDOS [...]                                           */
/* ========================================================================== */

/**
 * @brief Evalúa un subcomando entre [ ]
 */
char *bcl_eval_subcommand(bcl_interp_t *interp, const char *cmd) {
    if (!cmd || !interp) return bcl_strdup("");

    bcl_value_t *result = NULL;
    bcl_result_t res = bcl_eval(interp, cmd, &result);

    if (res != BCL_OK || !result) {
        return bcl_strdup("");
    }

    char *ret = bcl_strdup(bcl_value_get(result));
    bcl_value_destroy(result);
    return ret;
}

/**
 * @brief Expande todos los subcomandos [..] en un string
 * @note Maneja anidamiento de corchetes
 */
char *bcl_expand_subcommands(bcl_interp_t *interp, const char *str) {
    if (!str || !interp) return bcl_strdup("");

    bcl_string_t *result = bcl_string_create_empty(strlen(str) * 2);
    if (!result) return NULL;

    size_t i = 0;
    size_t len = strlen(str);

    while (i < len) {
        /* Buscar inicio de subcomando */
        if (str[i] == '[') {
            /* Encontrar el ] correspondiente, manejando anidamiento */
            size_t start = i + 1;
            int bracket_count = 1;
            i++;

            while (i < len && bracket_count > 0) {
                if (str[i] == '[') {
                    bracket_count++;
                } else if (str[i] == ']') {
                    bracket_count--;
                }
                if (bracket_count > 0) {
                    i++;
                }
            }

            /* Extraer contenido del subcomando */
            size_t cmd_len = i - start;
            char *cmd = malloc(cmd_len + 1);
            if (cmd) {
                strncpy(cmd, str + start, cmd_len);
                cmd[cmd_len] = '\0';

                /* Expandir recursivamente subcomandos anidados */
                char *expanded_cmd = bcl_expand_subcommands(interp, cmd);
                free(cmd);

                /* Evaluar subcomando */
                char *cmd_result = bcl_eval_subcommand(interp, expanded_cmd);
                free(expanded_cmd);

                /* Agregar resultado al output */
                if (cmd_result) {
                    /* Si el resultado contiene espacios, necesitamos protegerlo */
                    bool has_spaces = false;
                    for (const char *p = cmd_result; *p; p++) {
                        if (isspace((unsigned char)*p)) {
                            has_spaces = true;
                            break;
                        }
                    }

                    if (has_spaces) {
                        /* Envolver en comillas y escapar comillas internas */
                        bcl_string_append_char(result, '"');
                        for (const char *p = cmd_result; *p; p++) {
                            if (*p == '"') {
                                /* Escapar comilla */
                                bcl_string_append_char(result, '\\');
                            }
                            bcl_string_append_char(result, *p);
                        }
                        bcl_string_append_char(result, '"');
                    } else {
                        bcl_string_append(result, cmd_result);
                    }
                    free(cmd_result);
                }
            }

            /* Saltar el ] final */
            if (i < len && str[i] == ']') {
                i++;
            }
        } else {
            /* Carácter normal, copiar */
            bcl_string_append_char(result, str[i]);
            i++;
        }
    }

    char *final = bcl_strdup(bcl_string_cstr(result));
    bcl_string_destroy(result);
    return final;
}

/* ========================================================================== */
/* PARSEO DE LÍNEA A TOKENS                                                  */
/* ========================================================================== */

/**
 * @brief Divide una línea en tokens procesados
 * @param interp Intérprete (para expansión de variables)
 * @param line Línea a parsear
 * @param argc Puntero donde guardar número de argumentos
 * @return Array de strings (debe liberarse con bcl_free_tokens)
 */
char **bcl_parse_line(bcl_interp_t *interp, const char *line, int *argc) {
    if (!line || !argc) return NULL;

    /* Ignorar comentarios */
    if (is_comment(line)) {
        *argc = 0;
        return NULL;
    }

    /* Paso 1: Expandir subcomandos [..] en toda la línea */
    char *expanded_line = bcl_expand_subcommands(interp, line);
    if (!expanded_line) {
        *argc = 0;
        return NULL;
    }

    /* Extraer tokens crudos */
    char **raw_tokens = malloc(sizeof(char *) * 256); /* Máximo 256 tokens */
    if (!raw_tokens) {
        free(expanded_line);
        return NULL;
    }

    size_t pos = 0;
    int count = 0;

    char token[BCL_MAX_TOKEN_LEN];
    while (bcl_next_token(expanded_line, &pos, token, sizeof(token))) {
        raw_tokens[count++] = bcl_strdup(token);
        if (count >= 256) break;
    }

    free(expanded_line);

    if (count == 0) {
        free(raw_tokens);
        *argc = 0;
        return NULL;
    }

    /* Procesar tokens: decodificación de escapes y expansión de variables */
    char **processed = malloc(sizeof(char *) * count);
    if (!processed) {
        for (int i = 0; i < count; i++) {
            free(raw_tokens[i]);
        }
        free(raw_tokens);
        *argc = 0;
        return NULL;
    }

    for (int i = 0; i < count; i++) {
        char *token_str = raw_tokens[i];

        /* Decodificar escapes */
        char *decoded = bcl_decode_escapes(token_str);
        if (!decoded) decoded = bcl_strdup("");

        /* Expandir variables */
        char *expanded = bcl_expand_vars(interp, decoded);
        free(decoded);

        if (!expanded) expanded = bcl_strdup("");

        processed[i] = expanded;
        free(token_str);
    }

    free(raw_tokens);
    *argc = count;
    return processed;
}

/**
 * @brief Libera array de tokens
 */
void bcl_free_tokens(char **tokens, int count) {
    if (!tokens) return;

    for (int i = 0; i < count; i++) {
        free(tokens[i]);
    }
    free(tokens);
}
