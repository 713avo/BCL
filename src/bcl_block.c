/**
 * @file bcl_block.c
 * @brief Parser de bloques estructurados multi-línea
 * @note Maneja IF/WHILE/FOR/FOREACH/SWITCH/PROC con anidamiento
 */

#include "bcl.h"
#include "bcl_commands.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

/* ========================================================================== */
/* TIPOS DE BLOQUES                                                          */
/* ========================================================================== */

typedef enum {
    BLOCK_NONE,
    BLOCK_IF,
    BLOCK_ELSEIF,
    BLOCK_ELSE,
    BLOCK_WHILE,
    BLOCK_FOR,
    BLOCK_FOREACH,
    BLOCK_SWITCH,
    BLOCK_CASE,
    BLOCK_DEFAULT,
    BLOCK_PROC
} block_type_t;

/* Item en la secuencia de ejecución (puede ser línea o bloque) */
typedef enum {
    ITEM_LINE,
    ITEM_BLOCK
} item_type_t;

typedef struct {
    item_type_t type;
    union {
        char *line;
        struct bcl_block *block;
    } data;
} block_item_t;

typedef struct bcl_block {
    block_type_t type;          /* Tipo de bloque */
    char *condition;            /* Condición (para IF, WHILE, etc) */
    char *proc_name;            /* Nombre del procedimiento (para PROC) */
    char *proc_params;          /* Parámetros del procedimiento (para PROC) */
    block_item_t *items;        /* Items: líneas o sub-bloques */
    size_t item_count;
    size_t item_capacity;
    struct bcl_block **children; /* Sub-bloques hermanos (ELSEIF, ELSE) */
    size_t child_count;
    size_t child_capacity;
    int start_line_num;         /* Número de línea original (para debug) */
} bcl_block_t;

/* ========================================================================== */
/* CREACIÓN Y DESTRUCCIÓN DE BLOQUES                                         */
/* ========================================================================== */

static bcl_block_t *block_create(block_type_t type) {
    bcl_block_t *block = calloc(1, sizeof(bcl_block_t));
    if (!block) return NULL;

    block->type = type;
    block->condition = NULL;
    block->proc_name = NULL;
    block->proc_params = NULL;
    block->items = malloc(sizeof(block_item_t) * 32);
    block->item_count = 0;
    block->item_capacity = 32;
    block->children = malloc(sizeof(bcl_block_t*) * 8);
    block->child_count = 0;
    block->child_capacity = 8;
    block->start_line_num = 0;

    if (!block->items || !block->children) {
        free(block->items);
        free(block->children);
        free(block);
        return NULL;
    }

    return block;
}

static void block_destroy(bcl_block_t *block) {
    if (!block) return;

    free(block->condition);
    free(block->proc_name);
    free(block->proc_params);

    for (size_t i = 0; i < block->item_count; i++) {
        if (block->items[i].type == ITEM_LINE) {
            free(block->items[i].data.line);
        } else if (block->items[i].type == ITEM_BLOCK) {
            block_destroy(block->items[i].data.block);
        }
    }
    free(block->items);

    for (size_t i = 0; i < block->child_count; i++) {
        block_destroy(block->children[i]);
    }
    free(block->children);

    free(block);
}

static void block_add_line(bcl_block_t *block, const char *line) {
    if (!block || !line) return;

    if (block->item_count >= block->item_capacity) {
        block->item_capacity *= 2;
        block->items = realloc(block->items, sizeof(block_item_t) * block->item_capacity);
    }

    block->items[block->item_count].type = ITEM_LINE;
    block->items[block->item_count].data.line = bcl_strdup(line);
    block->item_count++;
}

static void block_add_block_item(bcl_block_t *parent, bcl_block_t *child) {
    if (!parent || !child) return;

    if (parent->item_count >= parent->item_capacity) {
        parent->item_capacity *= 2;
        parent->items = realloc(parent->items, sizeof(block_item_t) * parent->item_capacity);
    }

    parent->items[parent->item_count].type = ITEM_BLOCK;
    parent->items[parent->item_count].data.block = child;
    parent->item_count++;
}

static void block_add_child(bcl_block_t *block, bcl_block_t *child) {
    if (!block || !child) return;

    if (block->child_count >= block->child_capacity) {
        block->child_capacity *= 2;
        block->children = realloc(block->children, sizeof(bcl_block_t*) * block->child_capacity);
    }

    block->children[block->child_count++] = child;
}

/* ========================================================================== */
/* UTILIDADES DE PARSING                                                     */
/* ========================================================================== */

/**
 * Extrae el primer token de una línea (case-insensitive)
 */
static char *get_first_token(const char *line) {
    if (!line) return NULL;

    /* Saltar espacios iniciales */
    while (*line && isspace((unsigned char)*line)) line++;
    if (!*line) return NULL;

    /* Extraer primer token */
    const char *start = line;
    while (*line && !isspace((unsigned char)*line)) line++;

    size_t len = line - start;
    if (len == 0) return NULL;

    char *token = malloc(len + 1);
    if (!token) return NULL;

    memcpy(token, start, len);
    token[len] = '\0';

    return token;
}

/**
 * Verifica si una línea contiene "END" como token separado
 * (para detectar sintaxis inline tipo: IF cond THEN cmd END)
 */
static bool line_contains_end(const char *line) {
    if (!line) return false;

    const char *p = line;
    while (*p) {
        /* Saltar espacios */
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) break;

        /* Inicio de un token */
        const char *token_start = p;
        while (*p && !isspace((unsigned char)*p)) p++;

        size_t token_len = p - token_start;

        /* Verificar si el token es "END" */
        if (token_len == 3 && bcl_strncasecmp(token_start, "END", 3) == 0) {
            return true;
        }
    }

    return false;
}

/**
 * Extrae la condición después de la keyword (hasta THEN o DO)
 */
static char *extract_condition(const char *line, const char *keyword) {
    if (!line || !keyword) return NULL;

    /* Buscar la keyword (case-insensitive) */
    const char *p = line;
    size_t kw_len = strlen(keyword);

    while (*p && isspace((unsigned char)*p)) p++;

    /* Verificar que empiece con keyword */
    if (bcl_strncasecmp(p, keyword, kw_len) != 0) {
        return NULL;
    }

    p += kw_len;
    while (*p && isspace((unsigned char)*p)) p++;

    /* Buscar THEN o DO */
    const char *end = p;
    const char *then_pos = NULL;
    const char *do_pos = NULL;

    while (*end) {
        if (isspace((unsigned char)*end)) {
            const char *next = end + 1;
            while (*next && isspace((unsigned char)*next)) next++;

            if (bcl_strncasecmp(next, "THEN", 4) == 0) {
                then_pos = end;
                break;
            }
            if (bcl_strncasecmp(next, "DO", 2) == 0) {
                do_pos = end;
                break;
            }
        }
        end++;
    }

    const char *cond_end = then_pos ? then_pos : (do_pos ? do_pos : end);
    if (cond_end == p) return bcl_strdup("");

    size_t cond_len = cond_end - p;
    char *condition = malloc(cond_len + 1);
    if (!condition) return NULL;

    memcpy(condition, p, cond_len);
    condition[cond_len] = '\0';

    /* Trim trailing spaces */
    while (cond_len > 0 && isspace((unsigned char)condition[cond_len - 1])) {
        condition[--cond_len] = '\0';
    }

    /* Special case for CASE: remove surrounding quotes if present */
    if (bcl_strcasecmp(keyword, "CASE") == 0 && cond_len >= 2) {
        if ((condition[0] == '"' && condition[cond_len - 1] == '"') ||
            (condition[0] == '\'' && condition[cond_len - 1] == '\'')) {
            /* Remove quotes */
            char *unquoted = malloc(cond_len - 1);
            if (unquoted) {
                memcpy(unquoted, condition + 1, cond_len - 2);
                unquoted[cond_len - 2] = '\0';
                free(condition);
                condition = unquoted;
            }
        }
    }

    return condition;
}

/* ========================================================================== */
/* PARSER DE BLOQUES                                                         */
/* ========================================================================== */

/**
 * Parsea un array de líneas en una estructura de bloques
 */
bcl_block_t *bcl_parse_blocks(const char *code) {
    if (!code) return NULL;

    /* Convertir código a array de líneas */
    char **lines = NULL;
    size_t line_count = 0;
    size_t line_capacity = 256;

    lines = malloc(sizeof(char*) * line_capacity);
    if (!lines) return NULL;

    char *code_copy = bcl_strdup(code);
    if (!code_copy) {
        free(lines);
        return NULL;
    }

    char *saveptr = NULL;
    char *line = strtok_r(code_copy, "\n", &saveptr);
    while (line) {
        if (line_count >= line_capacity) {
            line_capacity *= 2;
            lines = realloc(lines, sizeof(char*) * line_capacity);
        }
        lines[line_count++] = bcl_strdup(line);
        line = strtok_r(NULL, "\n", &saveptr);
    }
    free(code_copy);

    /* Parsear recursivamente */
    bcl_block_t *root = block_create(BLOCK_NONE);
    if (!root) {
        for (size_t i = 0; i < line_count; i++) free(lines[i]);
        free(lines);
        return NULL;
    }

    /* Stack para tracking de bloques anidados */
    bcl_block_t *stack[256];
    int stack_top = 0;
    stack[stack_top] = root;

    for (size_t i = 0; i < line_count; i++) {
        const char *line_text = lines[i];

        /* Saltar líneas vacías y comentarios */
        const char *p = line_text;
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p || *p == '#') continue;

        char *first_token = get_first_token(p);
        if (!first_token) {
            /* Si no hay token, agregar línea al bloque actual */
            bcl_block_t *current_block = stack[stack_top];
            block_add_line(current_block, line_text);
            continue;
        }

        /* Verificar keywords estructurales */
        if (bcl_strcasecmp(first_token, "IF") == 0) {
            /* Verificar si es sintaxis inline (contiene END en la misma línea) */
            if (line_contains_end(line_text)) {
                /* Sintaxis inline: IF cond THEN cmd END */
                /* Tratar como línea normal, no como bloque */
                bcl_block_t *current_block = stack[stack_top];
                block_add_line(current_block, line_text);
                free(first_token);
                continue;
            }

            /* Sintaxis multi-línea: crear bloque */
            bcl_block_t *current_block = stack[stack_top];
            bcl_block_t *if_block = block_create(BLOCK_IF);
            if_block->condition = extract_condition(line_text, "IF");
            if_block->start_line_num = i + 1;
            block_add_block_item(current_block, if_block); /* Agregar a items, no a children */
            stack[++stack_top] = if_block;
            free(first_token);
            continue;

        } else if (bcl_strcasecmp(first_token, "ELSEIF") == 0) {
            /* ELSEIF es child del IF, not sibling */
            if (stack_top > 0) {
                bcl_block_t *if_block = stack[stack_top];
                stack_top--; /* Pop el IF/ELSEIF actual */

                bcl_block_t *elseif_block = block_create(BLOCK_ELSEIF);
                elseif_block->condition = extract_condition(line_text, "ELSEIF");
                elseif_block->start_line_num = i + 1;
                block_add_child(if_block, elseif_block); /* Agregar como child del IF */
                stack[++stack_top] = elseif_block;
            }
            free(first_token);
            continue;

        } else if (bcl_strcasecmp(first_token, "ELSE") == 0) {
            if (stack_top > 0) {
                bcl_block_t *if_block = stack[stack_top];
                stack_top--; /* Pop el IF/ELSEIF actual */

                bcl_block_t *else_block = block_create(BLOCK_ELSE);
                else_block->start_line_num = i + 1;
                block_add_child(if_block, else_block); /* Agregar como child del IF */
                stack[++stack_top] = else_block;
            }
            free(first_token);
            continue;

        } else if (bcl_strcasecmp(first_token, "WHILE") == 0) {
            bcl_block_t *current_block = stack[stack_top];
            bcl_block_t *while_block = block_create(BLOCK_WHILE);
            while_block->condition = extract_condition(line_text, "WHILE");
            while_block->start_line_num = i + 1;
            block_add_block_item(current_block, while_block);
            stack[++stack_top] = while_block;
            free(first_token);
            continue;

        } else if (bcl_strcasecmp(first_token, "FOR") == 0) {
            bcl_block_t *current_block = stack[stack_top];
            bcl_block_t *for_block = block_create(BLOCK_FOR);
            for_block->condition = extract_condition(line_text, "FOR");
            for_block->start_line_num = i + 1;
            block_add_block_item(current_block, for_block);
            stack[++stack_top] = for_block;
            free(first_token);
            continue;

        } else if (bcl_strcasecmp(first_token, "FOREACH") == 0) {
            bcl_block_t *current_block = stack[stack_top];
            bcl_block_t *foreach_block = block_create(BLOCK_FOREACH);
            foreach_block->condition = extract_condition(line_text, "FOREACH");
            foreach_block->start_line_num = i + 1;
            block_add_block_item(current_block, foreach_block);
            stack[++stack_top] = foreach_block;
            free(first_token);
            continue;

        } else if (bcl_strcasecmp(first_token, "SWITCH") == 0) {
            bcl_block_t *current_block = stack[stack_top];
            bcl_block_t *switch_block = block_create(BLOCK_SWITCH);
            switch_block->condition = extract_condition(line_text, "SWITCH");
            switch_block->start_line_num = i + 1;
            block_add_block_item(current_block, switch_block);
            stack[++stack_top] = switch_block;
            free(first_token);
            continue;

        } else if (bcl_strcasecmp(first_token, "CASE") == 0) {
            if (stack_top > 0) {
                /* CASE puede ser hermano de otro CASE - pop previous CASE if exists */
                if (stack[stack_top]->type == BLOCK_CASE || stack[stack_top]->type == BLOCK_DEFAULT) {
                    stack_top--;
                }
                bcl_block_t *case_block = block_create(BLOCK_CASE);
                case_block->condition = extract_condition(line_text, "CASE");
                case_block->start_line_num = i + 1;
                block_add_child(stack[stack_top], case_block);
                stack[++stack_top] = case_block;
            }
            free(first_token);
            continue;

        } else if (bcl_strcasecmp(first_token, "DEFAULT") == 0) {
            if (stack_top > 0) {
                if (stack[stack_top]->type == BLOCK_CASE) {
                    stack_top--;
                }
                bcl_block_t *default_block = block_create(BLOCK_DEFAULT);
                default_block->start_line_num = i + 1;
                block_add_child(stack[stack_top], default_block);
                stack[++stack_top] = default_block;
            }
            free(first_token);
            continue;

        } else if (bcl_strcasecmp(first_token, "PROC") == 0) {
            /* PROC nombre WITH param1 param2 ... DO  o  PROC nombre DO */
            bcl_block_t *current_block = stack[stack_top];
            bcl_block_t *proc_block = block_create(BLOCK_PROC);
            proc_block->start_line_num = i + 1;

            /* Parsear: PROC nombre [WITH params] DO */
            const char *p = line_text;
            while (*p && isspace((unsigned char)*p)) p++;
            p += 4; /* skip "PROC" */
            while (*p && isspace((unsigned char)*p)) p++;

            /* Extraer nombre */
            const char *name_start = p;
            while (*p && !isspace((unsigned char)*p)) p++;
            size_t name_len = p - name_start;
            if (name_len > 0) {
                proc_block->proc_name = malloc(name_len + 1);
                memcpy(proc_block->proc_name, name_start, name_len);
                proc_block->proc_name[name_len] = '\0';
            }

            /* Buscar WITH o DO */
            while (*p && isspace((unsigned char)*p)) p++;

            /* Verificar si viene DO directamente (sin WITH) */
            if (bcl_strncasecmp(p, "DO", 2) == 0 &&
                (p[2] == '\0' || isspace((unsigned char)p[2]))) {
                /* PROC nombre DO - sin parámetros, proc_params queda NULL */
            } else if (bcl_strncasecmp(p, "WITH", 4) == 0) {
                /* PROC nombre WITH ... */
                p += 4;
                while (*p && isspace((unsigned char)*p)) p++;

                /* Verificar si viene DO inmediatamente después de WITH */
                if (bcl_strncasecmp(p, "DO", 2) == 0 &&
                    (p[2] == '\0' || isspace((unsigned char)p[2]))) {
                    /* PROC nombre WITH DO - sin parámetros */
                } else {
                    /* Extraer parámetros hasta DO */
                    const char *param_start = p;
                    const char *do_pos = NULL;

                    /* Buscar DO como palabra completa */
                    while (*p) {
                        if (isspace((unsigned char)*p)) {
                            const char *next = p;
                            while (*next && isspace((unsigned char)*next)) next++;
                            if (bcl_strncasecmp(next, "DO", 2) == 0 &&
                                (next[2] == '\0' || isspace((unsigned char)next[2]))) {
                                do_pos = p;
                                break;
                            }
                        }
                        p++;
                    }

                    if (do_pos && do_pos > param_start) {
                        size_t param_len = do_pos - param_start;
                        proc_block->proc_params = malloc(param_len + 1);
                        memcpy(proc_block->proc_params, param_start, param_len);
                        proc_block->proc_params[param_len] = '\0';
                        /* Trim trailing spaces */
                        while (param_len > 0 && isspace((unsigned char)proc_block->proc_params[param_len - 1])) {
                            proc_block->proc_params[--param_len] = '\0';
                        }
                    } else if (*p && !do_pos) {
                        /* Sin DO - params hasta final de línea */
                        size_t param_len = strlen(param_start);
                        if (param_len > 0) {
                            proc_block->proc_params = bcl_strdup(param_start);
                        }
                    }
                }
            }

            block_add_block_item(current_block, proc_block);
            stack[++stack_top] = proc_block;
            free(first_token);
            continue;

        } else if (bcl_strcasecmp(first_token, "END") == 0) {
            /* Pop del stack */
            if (stack_top > 0) {
                block_type_t current_type = stack[stack_top]->type;
                stack_top--;

                /* Si acabamos de cerrar un CASE/DEFAULT, también cerrar el SWITCH padre */
                if (stack_top > 0 && (current_type == BLOCK_CASE || current_type == BLOCK_DEFAULT)) {
                    if (stack[stack_top]->type == BLOCK_SWITCH) {
                        stack_top--;
                    }
                }
            }
            free(first_token);
            continue;

        } else {
            /* Línea normal - agregar al bloque actual */
            bcl_block_t *current_block = stack[stack_top];
            block_add_line(current_block, line_text);
            free(first_token);
            continue;
        }
    }

    /* Liberar líneas temporales */
    for (size_t i = 0; i < line_count; i++) {
        free(lines[i]);
    }
    free(lines);

    return root;
}

/* ========================================================================== */
/* EJECUCIÓN DE BLOQUES                                                      */
/* ========================================================================== */

/**
 * Evalúa una condición y retorna true/false
 */
static bool eval_block_condition(bcl_interp_t *interp, const char *condition) {
    if (!condition || !*condition) return false;

    /* Expandir subcomandos [..] primero */
    char *with_cmds = bcl_expand_subcommands(interp, condition);
    if (!with_cmds) with_cmds = bcl_strdup(condition);

    /* Expandir variables */
    char *expanded = bcl_expand_vars(interp, with_cmds);
    free(with_cmds);
    if (!expanded) return false;

    /* Tokenizar para EXPR */
    int argc = 0;
    char *argv[256];

    char *p = expanded;
    char token_buf[BCL_MAX_TOKEN_LEN];

    while (*p) {
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) break;

        size_t i = 0;
        while (*p && !isspace((unsigned char)*p) && i < sizeof(token_buf) - 1) {
            token_buf[i++] = *p++;
        }
        token_buf[i] = '\0';

        if (i > 0 && argc < 256) {
            argv[argc++] = bcl_strdup(token_buf);
        }
    }

    free(expanded);

    if (argc == 0) return false;

    /* Llamar a EXPR */
    bcl_value_t *result = NULL;
    bcl_result_t res = bcl_cmd_expr(interp, argc, argv, &result);

    for (int i = 0; i < argc; i++) {
        free(argv[i]);
    }

    if (res != BCL_OK || !result) {
        if (result) bcl_value_destroy(result);
        return false;
    }

    bool is_true = bcl_value_to_bool(result);
    bcl_value_destroy(result);

    return is_true;
}

/**
 * Ejecuta los items de un bloque (líneas y sub-bloques) en orden
 */
static bcl_result_t exec_block_items(bcl_interp_t *interp, bcl_block_t *block) {
    if (!interp || !block) return BCL_ERROR;

    for (size_t i = 0; i < block->item_count; i++) {
        block_item_t *item = &block->items[i];

        if (item->type == ITEM_LINE) {
            /* Ejecutar línea */
            const char *line = item->data.line;

            int argc;
            char **argv = bcl_parse_line(interp, line, &argc);

            if (argv && argc > 0) {
                bcl_value_t *cmd_result = NULL;
                bcl_result_t res = bcl_dispatch_command(interp, argv[0], argc - 1, argv + 1, &cmd_result);

                if (cmd_result) bcl_value_destroy(cmd_result);
                bcl_free_tokens(argv, argc);

                if (res != BCL_OK) {
                    return res;
                }
            }

        } else if (item->type == ITEM_BLOCK) {
            /* Ejecutar sub-bloque */
            bcl_result_t res = bcl_exec_block(interp, item->data.block);
            if (res != BCL_OK) {
                return res;
            }
        }
    }

    return BCL_OK;
}

/**
 * Ejecuta un bloque recursivamente
 */
bcl_result_t bcl_exec_block(bcl_interp_t *interp, bcl_block_t *block) {
    if (!interp || !block) return BCL_ERROR;

    switch (block->type) {
        case BLOCK_NONE:
            /* Bloque raíz - ejecutar items en orden */
            return exec_block_items(interp, block);

        case BLOCK_IF:
        case BLOCK_ELSEIF:
            /* Evaluar condición y ejecutar si es verdadera */
            if (eval_block_condition(interp, block->condition)) {
                bcl_result_t res = exec_block_items(interp, block);
                if (res != BCL_OK) return res;

                /* Ejecutar hermanos ELSEIF/ELSE si existen (skip) */
                /* Los hermanos están en block->children */
                /* Pero como ya ejecutamos, no ejecutar hermanos */
                return BCL_OK;
            } else {
                /* Condición falsa - probar hermanos ELSEIF/ELSE */
                for (size_t i = 0; i < block->child_count; i++) {
                    bcl_block_t *sibling = block->children[i];

                    if (sibling->type == BLOCK_ELSEIF || sibling->type == BLOCK_ELSE) {
                        /* Recursively execute ELSEIF/ELSE block */
                        /* bcl_exec_block will handle checking condition and children */
                        return bcl_exec_block(interp, sibling);
                    }
                }
            }
            break;

        case BLOCK_ELSE:
            /* Ejecutar incondicionalmente */
            return exec_block_items(interp, block);

        case BLOCK_WHILE:
            /* Loop mientras condición sea verdadera */
            while (eval_block_condition(interp, block->condition)) {
                bcl_result_t res = exec_block_items(interp, block);

                if (res == BCL_BREAK) {
                    return BCL_OK;
                } else if (res == BCL_CONTINUE) {
                    continue;
                } else if (res != BCL_OK) {
                    return res;
                }
            }
            break;

        case BLOCK_FOR:
            /* FOR contador interno: inicio TO fin [STEP paso] */
            {
                if (!block->condition) {
                    bcl_set_error(interp, "FOR: missing condition");
                    return BCL_ERROR;
                }

                /* Parsear: "inicio TO fin [STEP paso]" */
                char *cond = bcl_strdup(block->condition);
                char *saveptr = NULL;
                char *inicio_str = strtok_r(cond, " \t", &saveptr);
                char *to_keyword = strtok_r(NULL, " \t", &saveptr);
                char *fin_str = strtok_r(NULL, " \t", &saveptr);
                char *step_keyword = strtok_r(NULL, " \t", &saveptr);
                char *paso_str = NULL;

                if (step_keyword && bcl_strcasecmp(step_keyword, "STEP") == 0) {
                    paso_str = strtok_r(NULL, " \t", &saveptr);
                }

                if (!inicio_str || !to_keyword || !fin_str ||
                    bcl_strcasecmp(to_keyword, "TO") != 0) {
                    free(cond);
                    bcl_set_error(interp, "FOR: invalid syntax, expected 'inicio TO fin [STEP paso]'");
                    return BCL_ERROR;
                }

                /* Evaluar inicio, fin y paso */
                double inicio = atof(inicio_str);
                double fin = atof(fin_str);
                double paso = paso_str ? atof(paso_str) : 1.0;

                free(cond);

                /* Ejecutar bucle */
                for (double i = inicio;
                     (paso > 0 && i <= fin) || (paso < 0 && i >= fin);
                     i += paso) {

                    /* Establecer variable $__FOR */
                    char __for_value[64];
                    if (i == floor(i)) {
                        snprintf(__for_value, sizeof(__for_value), "%.0f", i);
                    } else {
                        snprintf(__for_value, sizeof(__for_value), "%.15g", i);
                    }
                    bcl_var_set(interp, "__FOR", __for_value);

                    /* Ejecutar cuerpo */
                    bcl_result_t res = exec_block_items(interp, block);

                    if (res == BCL_BREAK) {
                        return BCL_OK;
                    } else if (res == BCL_CONTINUE) {
                        continue;
                    } else if (res != BCL_OK) {
                        return res;
                    }
                }
            }
            break;

        case BLOCK_FOREACH:
            /* FOREACH var IN lista DO ... END */
            {
                if (!block->condition) {
                    bcl_set_error(interp, "FOREACH: missing condition");
                    return BCL_ERROR;
                }

                /* Parsear: "var IN lista" o "var lista" (aceptar ambas sintaxis) */
                char *cond = bcl_strdup(block->condition);
                char *saveptr = NULL;
                char *varname = strtok_r(cond, " \t", &saveptr);

                if (!varname) {
                    free(cond);
                    bcl_set_error(interp, "FOREACH: missing variable name");
                    return BCL_ERROR;
                }

                /* Siguiente token: puede ser "IN" o la lista directamente */
                char *next_token = strtok_r(NULL, " \t", &saveptr);
                char *lista_expr = NULL;

                if (next_token && (strcmp(next_token, "IN") == 0 || strcmp(next_token, "in") == 0)) {
                    /* Sintaxis: FOREACH var IN lista */
                    lista_expr = saveptr;
                } else if (next_token) {
                    /* Sintaxis: FOREACH var lista (sin IN) */
                    /* Reconstruir la lista: next_token + resto */
                    size_t len = strlen(next_token) + (saveptr ? strlen(saveptr) : 0) + 2;
                    lista_expr = malloc(len);
                    if (saveptr && strlen(saveptr) > 0) {
                        snprintf((char*)lista_expr, len, "%s %s", next_token, saveptr);
                    } else {
                        snprintf((char*)lista_expr, len, "%s", next_token);
                    }
                }

                if (!lista_expr || strlen(lista_expr) == 0) {
                    free(cond);
                    if (lista_expr && lista_expr != saveptr) free((char*)lista_expr);
                    bcl_set_error(interp, "FOREACH: missing list");
                    return BCL_ERROR;
                }

                /* Hacer una copia del varname antes de liberar cond */
                char *var_copy = bcl_strdup(varname);

                /* Recordar si lista_expr fue asignado con malloc */
                bool lista_expr_allocated = (next_token && strcmp(next_token, "IN") != 0 && strcmp(next_token, "in") != 0);

                /* Evaluar la expresión de lista (puede ser $variable) */
                char *lista_val = NULL;
                if (lista_expr[0] == '$') {
                    /* Es una variable */
                    char *vname = lista_expr + 1;
                    bcl_value_t *val = bcl_var_get(interp, vname);
                    if (val) {
                        lista_val = bcl_strdup(bcl_value_get(val));
                    } else {
                        lista_val = bcl_strdup("");
                    }
                } else {
                    /* Es un literal */
                    lista_val = bcl_strdup(lista_expr);
                }

                free(cond);
                if (lista_expr_allocated && lista_expr != saveptr) {
                    free((char*)lista_expr);
                }

                /* Dividir lista por espacios o newlines */
                char *item = strtok_r(lista_val, " \t\n", &saveptr);

                while (item != NULL) {
                    /* Establecer variable de iteración */
                    bcl_var_set(interp, var_copy, item);

                    /* Ejecutar cuerpo del loop */
                    bcl_result_t res = exec_block_items(interp, block);

                    if (res == BCL_BREAK) {
                        free(var_copy);
                        free(lista_val);
                        return BCL_OK;
                    } else if (res == BCL_CONTINUE) {
                        item = strtok_r(NULL, " \t\n", &saveptr);
                        continue;
                    } else if (res != BCL_OK) {
                        free(var_copy);
                        free(lista_val);
                        return res;
                    }

                    /* Siguiente item */
                    item = strtok_r(NULL, " \t\n", &saveptr);
                }

                free(var_copy);
                free(lista_val);
            }
            break;

        case BLOCK_PROC:
            /* PROC nombre WITH param1 param2 ... DO ... END */
            /* Este bloque DEFINE el procedimiento, no lo ejecuta */
            {
                if (!block->proc_name) {
                    bcl_set_error(interp, "PROC: missing procedure name");
                    return BCL_ERROR;
                }

                /* Parsear parámetros */
                bcl_param_t params[32];
                size_t param_count = 0;
                char *params_copy = NULL;

                if (block->proc_params && strlen(block->proc_params) > 0) {
                    params_copy = bcl_strdup(block->proc_params);
                    if (params_copy) {
                        char *saveptr = NULL;
                        char *token = strtok_r(params_copy, " \t", &saveptr);

                        while (token && param_count < 32) {
                            /* Skip empty tokens */
                            if (token[0] == '\0') {
                                token = strtok_r(NULL, " \t", &saveptr);
                                continue;
                            }

                            bool optional = false;
                            if (token[0] == '@') {
                                optional = true;
                                token++; /* Skip @ */
                            }

                            /* Skip if name is empty after removing @ */
                            if (token[0] == '\0') {
                                token = strtok_r(NULL, " \t", &saveptr);
                                continue;
                            }

                            params[param_count].name = token;
                            params[param_count].optional = optional;
                            param_count++;

                            token = strtok_r(NULL, " \t", &saveptr);
                        }
                    }
                }

                /* Crear bloque contenedor para el cuerpo del procedimiento */
                /* Este bloque contendrá todos los items (líneas y sub-bloques) */
                bcl_block_t *body_block = block_create(BLOCK_NONE);
                if (!body_block) {
                    if (params_copy) free(params_copy);
                    bcl_set_error(interp, "Out of memory creating procedure body");
                    return BCL_ERROR;
                }

                /* Transferir todos los items del PROC al bloque contenedor */
                for (size_t i = 0; i < block->item_count; i++) {
                    if (block->items[i].type == ITEM_LINE) {
                        /* Copiar línea */
                        block_add_line(body_block, block->items[i].data.line);
                    } else if (block->items[i].type == ITEM_BLOCK) {
                        /* Transferir propiedad del sub-bloque (IF, WHILE, etc) */
                        block_add_block_item(body_block, block->items[i].data.block);
                        /* NULL la referencia para evitar double-free cuando se libere el PROC block */
                        block->items[i].data.block = NULL;
                    }
                }

                /* Definir procedimiento con el bloque pre-parseado */
                bcl_result_t res = bcl_proc_define(interp, block->proc_name,
                                                   params, param_count,
                                                   body_block);

                /* Liberar recursos */
                if (params_copy) free(params_copy);

                return res;
            }

        case BLOCK_SWITCH:
            /* SWITCH expr DO CASE val1 ... CASE val2 ... DEFAULT ... END */
            {
                if (!block->condition) {
                    bcl_set_error(interp, "SWITCH: missing expression");
                    return BCL_ERROR;
                }

                /* Evaluar expresión del SWITCH */
                char *switch_value = bcl_expand_vars(interp, block->condition);
                if (!switch_value) switch_value = bcl_strdup("");

                /* Buscar CASE que coincida */
                bool matched = false;
                for (size_t i = 0; i < block->child_count && !matched; i++) {
                    bcl_block_t *case_block = block->children[i];

                    if (case_block->type == BLOCK_CASE) {
                        /* Comparar valor */
                        char *case_value = bcl_expand_vars(interp, case_block->condition);
                        if (!case_value) case_value = bcl_strdup("");

                        if (strcmp(switch_value, case_value) == 0) {
                            free(case_value);
                            bcl_result_t res = exec_block_items(interp, case_block);
                            free(switch_value);
                            return res;
                        }
                        free(case_value);

                    } else if (case_block->type == BLOCK_DEFAULT) {
                        /* DEFAULT - ejecutar si no hubo match */
                        if (!matched) {
                            bcl_result_t res = exec_block_items(interp, case_block);
                            free(switch_value);
                            return res;
                        }
                    }
                }

                free(switch_value);
            }
            break;

        default:
            /* Otros tipos de bloques */
            bcl_set_error(interp, "Block type not yet implemented");
            return BCL_ERROR;
    }

    return BCL_OK;
}

/**
 * Libera un bloque y todos sus hijos
 */
void bcl_block_free(bcl_block_t *block) {
    block_destroy(block);
}
