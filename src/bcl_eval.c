/**
 * @file bcl_eval.c
 * @brief Evaluador principal y dispatcher de comandos
 */

#include "bcl.h"
#include "bcl_commands.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Forward declarations for extension system */
bool bcl_is_extension_command(bcl_interp_t *interp, const char *name);
bcl_result_t bcl_call_extension_command(bcl_interp_t *interp, const char *name,
                                       int argc, char **argv, bcl_value_t **result);

/* ========================================================================== */
/* TABLA DE COMANDOS                                                         */
/* ========================================================================== */

static const bcl_command_entry_t command_table[] = {
    /* Variables */
    {"SET",      bcl_cmd_set},
    {"UNSET",    bcl_cmd_unset},
    {"INCR",     bcl_cmd_incr},
    {"APPEND",   bcl_cmd_append},
    {"GLOBAL",   bcl_cmd_global},

    /* I/O */
    {"PUTS",     bcl_cmd_puts},
    {"PUTSN",    bcl_cmd_putsn},
    {"GETS",     bcl_cmd_gets},

    /* Expresiones */
    {"EXPR",     bcl_cmd_expr},

    /* Control de Flujo */
    {"IF",       bcl_cmd_if},
    {"WHILE",    bcl_cmd_while},
    {"FOR",      bcl_cmd_for},
    {"FOREACH",  bcl_cmd_foreach},
    {"SWITCH",   bcl_cmd_switch},
    {"BREAK",    bcl_cmd_break},
    {"CONTINUE", bcl_cmd_continue},
    {"RETURN",   bcl_cmd_return},
    {"EXIT",     bcl_cmd_exit},

    /* Arrays */
    {"ARRAY",    bcl_cmd_array},

    /* Binary */
    {"BINARY",   bcl_cmd_binary},

    /* Listas */
    {"LIST",     bcl_cmd_list},
    {"SPLIT",    bcl_cmd_split},
    {"JOIN",     bcl_cmd_join},
    {"LINDEX",   bcl_cmd_lindex},
    {"LRANGE",   bcl_cmd_lrange},
    {"LLENGTH",  bcl_cmd_llength},
    {"LAPPEND",  bcl_cmd_lappend},
    {"LINSERT",  bcl_cmd_linsert},
    {"LREPLACE", bcl_cmd_lreplace},
    {"CONCAT",   bcl_cmd_concat},
    {"LSORT",    bcl_cmd_lsort},
    {"LSEARCH",  bcl_cmd_lsearch},

    /* Introspección */
    {"INFO",     bcl_cmd_info},

    /* Tiempo */
    {"CLOCK",    bcl_cmd_clock},

    /* Strings */
    {"STRING",   bcl_cmd_string},

    /* Formateo */
    {"FORMAT",   bcl_cmd_format},
    {"SCAN",     bcl_cmd_scan},

    /* Expresiones Regulares */
    {"REGEXP",   bcl_cmd_regexp},
    {"REGSUB",   bcl_cmd_regsub},

    /* Archivos */
#ifndef BCL_NO_FILES
    {"OPEN",     bcl_cmd_open},
    {"CLOSE",    bcl_cmd_close},
    {"READ",     bcl_cmd_read},
    {"TELL",     bcl_cmd_tell},
    {"SEEK",     bcl_cmd_seek},
    {"EOF",      bcl_cmd_eof},
    {"PWD",      bcl_cmd_pwd},
    {"FILE",     bcl_cmd_file},
    {"GLOB",     bcl_cmd_glob},
#endif

    /* Sistema */
    {"EVAL",     bcl_cmd_eval},
    {"SOURCE",   bcl_cmd_source},
    {"LOAD",     bcl_cmd_load},
    {"ENV",      bcl_cmd_env},
    {"ARGV",     bcl_cmd_argv},
    {"EXEC",     bcl_cmd_exec},
    {"AFTER",    bcl_cmd_after},
    {"EVENT",    bcl_cmd_event},

    /* Fin de tabla */
    {NULL, NULL}
};

const bcl_command_entry_t *bcl_get_command_table(size_t *count) {
    if (count) {
        size_t n = 0;
        while (command_table[n].name != NULL) n++;
        *count = n;
    }
    return command_table;
}

/* ========================================================================== */
/* DISPATCHER DE COMANDOS                                                    */
/* ========================================================================== */

bcl_result_t bcl_dispatch_command(bcl_interp_t *interp, const char *name,
                                  int argc, char **argv, bcl_value_t **result) {
    if (!interp || !name) return BCL_ERROR;

    /* Buscar comando en la tabla */
    for (size_t i = 0; command_table[i].name != NULL; i++) {
        if (bcl_strcasecmp(command_table[i].name, name) == 0) {
            return command_table[i].func(interp, argc, argv, result);
        }
    }

    /* Buscar procedimiento definido por usuario */
    if (bcl_proc_exists(interp, name)) {
        /* Preparar argumentos como valores */
        bcl_value_t **args = NULL;
        if (argc > 0) {
            args = malloc(sizeof(bcl_value_t*) * argc);
            if (!args) {
                bcl_set_error(interp, "Out of memory");
                return BCL_ERROR;
            }
            for (int i = 0; i < argc; i++) {
                args[i] = bcl_value_create(argv[i]);
            }
        }

        /* Llamar procedimiento */
        bcl_result_t res = bcl_proc_call(interp, name, args, argc, result);

        /* Liberar argumentos */
        if (args) {
            for (int i = 0; i < argc; i++) {
                bcl_value_destroy(args[i]);
            }
            free(args);
        }

        return res;
    }

    /* Buscar comando de extensión */
    if (bcl_is_extension_command(interp, name)) {
        return bcl_call_extension_command(interp, name, argc, argv, result);
    }

    /* Comando no encontrado */
    bcl_set_error(interp, "invalid command name \"%s\"", name);
    return BCL_ERROR;
}

/* ========================================================================== */
/* EVALUADOR PRINCIPAL                                                       */
/* ========================================================================== */

bcl_result_t bcl_eval(bcl_interp_t *interp, const char *code,
                      bcl_value_t **result) {
    if (!interp || !code) return BCL_ERROR;

    /* Inicializar resultado */
    if (result) *result = NULL;

    /* Dividir en líneas y procesar */
    char *code_copy = bcl_strdup(code);
    if (!code_copy) {
        bcl_set_error(interp, "Out of memory");
        return BCL_ERROR;
    }

    bcl_result_t final_result = BCL_OK;
    bcl_value_t *last_result = NULL;

    /* Usar strtok_r para evitar problemas de reentrancia */
    char *saveptr = NULL;
    char *line = strtok_r(code_copy, "\n", &saveptr);
    while (line) {
        /* Parsear línea */
        int argc;
        char **argv = bcl_parse_line(interp, line, &argc);

        if (argv && argc > 0) {
            /* El primer token es el comando */
            const char *cmd_name = argv[0];

            /* Despachar comando */
            bcl_value_t *cmd_result = NULL;
            bcl_result_t res = bcl_dispatch_command(interp, cmd_name,
                                                    argc - 1, argv + 1,
                                                    &cmd_result);

            /* Liberar resultado anterior */
            if (last_result) {
                bcl_value_destroy(last_result);
            }
            last_result = cmd_result;

            /* Liberar argv */
            bcl_free_tokens(argv, argc);

            /* Manejar resultado especial */
            if (res == BCL_ERROR) {
                final_result = BCL_ERROR;
                break;
            } else if (res == BCL_EXIT) {
                final_result = BCL_EXIT;
                break;
            } else if (res == BCL_BREAK || res == BCL_CONTINUE) {
                /* BREAK/CONTINUE - devolver para que el llamador maneje */
                /* Si estamos en bucle, el bucle lo manejará */
                /* Si no, será error en nivel superior */
                final_result = res;
                break;
            } else if (res == BCL_RETURN) {
                /* RETURN - devolver para que bcl_proc_call lo maneje */
                /* Si estamos en PROC, bcl_proc_call lo manejará */
                /* Si no, será error en nivel superior (REPL, archivo) */
                final_result = BCL_RETURN;
                break;
            }
        }

        line = strtok_r(NULL, "\n", &saveptr);
    }

    free(code_copy);

    /* Retornar último resultado */
    if (result) {
        *result = last_result ? last_result : bcl_value_create("");
    } else if (last_result) {
        bcl_value_destroy(last_result);
    }

    return final_result;
}

/* ========================================================================== */
/* EVALUACIÓN ESTRUCTURADA (CON BLOQUES)                                     */
/* ========================================================================== */

bcl_result_t bcl_eval_structured(bcl_interp_t *interp, const char *code) {
    if (!interp || !code) return BCL_ERROR;

    /* Parsear código en bloques */
    bcl_block_t *root = bcl_parse_blocks(code);
    if (!root) {
        bcl_set_error(interp, "Failed to parse code blocks");
        return BCL_ERROR;
    }

    /* Ejecutar bloques */
    bcl_result_t res = bcl_exec_block(interp, root);

    /* Liberar bloques */
    bcl_block_free(root);

    return res;
}

/* ========================================================================== */
/* EVALUACIÓN DE ARCHIVO                                                     */
/* ========================================================================== */

bcl_result_t bcl_eval_file(bcl_interp_t *interp, const char *filename) {
    if (!interp || !filename) return BCL_ERROR;

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        bcl_set_error(interp, "couldn't read file \"%s\": no such file or directory",
                     filename);
        return BCL_ERROR;
    }

    /* Leer todo el archivo */
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (size < 0 || size > 10 * 1024 * 1024) { /* Límite 10MB */
        fclose(fp);
        bcl_set_error(interp, "file too large or error reading file");
        return BCL_ERROR;
    }

    char *code = malloc(size + 1);
    if (!code) {
        fclose(fp);
        bcl_set_error(interp, "Out of memory");
        return BCL_ERROR;
    }

    size_t read = fread(code, 1, size, fp);
    code[read] = '\0';
    fclose(fp);

    /* Evaluar con parser estructurado */
    bcl_result_t res = bcl_eval_structured(interp, code);

    free(code);

    return res;
}
