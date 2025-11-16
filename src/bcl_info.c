/**
 * @file bcl_info.c
 * @brief Comando INFO - Introspección del intérprete
 */

#include "bcl.h"
#include "bcl_commands.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ========================================================================== */
/* INFO COMMANDS - Lista todos los comandos disponibles                     */
/* ========================================================================== */

static bcl_result_t info_commands(bcl_interp_t *interp, int argc, char **argv,
                                  bcl_value_t **result) {
    BCL_UNUSED(argc);
    BCL_UNUSED(argv);

    /* Obtener tabla de comandos */
    size_t count;
    const bcl_command_entry_t *commands = bcl_get_command_table(&count);

    /* Construir lista de comandos */
    bcl_string_t *output = bcl_string_create("");
    for (size_t i = 0; i < count; i++) {
        if (i > 0) bcl_string_append(output, " ");
        bcl_string_append(output, commands[i].name);
    }

    *result = bcl_value_create(bcl_string_cstr(output));
    bcl_string_destroy(output);

    return BCL_OK;
}

/* ========================================================================== */
/* INFO VARS - Lista todas las variables                                     */
/* ========================================================================== */

static bcl_result_t info_vars(bcl_interp_t *interp, int argc, char **argv,
                              bcl_value_t **result) {
    BCL_UNUSED(argc);
    BCL_UNUSED(argv);

    /* Obtener variables globales */
    size_t count;
    char **keys = bcl_hash_keys(interp->global_vars, &count);

    if (!keys || count == 0) {
        *result = bcl_value_create("");
        return BCL_OK;
    }

    /* Construir lista */
    bcl_string_t *output = bcl_string_create("");
    for (size_t i = 0; i < count; i++) {
        if (i > 0) bcl_string_append(output, " ");
        bcl_string_append(output, keys[i]);
    }

    /* Liberar keys */
    for (size_t i = 0; i < count; i++) {
        free(keys[i]);
    }
    free(keys);

    *result = bcl_value_create(bcl_string_cstr(output));
    bcl_string_destroy(output);

    return BCL_OK;
}

/* ========================================================================== */
/* INFO PROCS - Lista todos los procedimientos                               */
/* ========================================================================== */

static bcl_result_t info_procs(bcl_interp_t *interp, int argc, char **argv,
                               bcl_value_t **result) {
    BCL_UNUSED(argc);
    BCL_UNUSED(argv);

    /* Obtener procedimientos */
    size_t count;
    char **keys = bcl_hash_keys(interp->procedures, &count);

    if (!keys || count == 0) {
        *result = bcl_value_create("");
        return BCL_OK;
    }

    /* Construir lista */
    bcl_string_t *output = bcl_string_create("");
    for (size_t i = 0; i < count; i++) {
        if (i > 0) bcl_string_append(output, " ");
        bcl_string_append(output, keys[i]);
    }

    /* Liberar keys */
    for (size_t i = 0; i < count; i++) {
        free(keys[i]);
    }
    free(keys);

    *result = bcl_value_create(bcl_string_cstr(output));
    bcl_string_destroy(output);

    return BCL_OK;
}

/* ========================================================================== */
/* INFO EXISTS - Verifica si existe una variable                             */
/* ========================================================================== */

static bcl_result_t info_exists(bcl_interp_t *interp, int argc, char **argv,
                                bcl_value_t **result) {
    if (argc < 1) {
        bcl_set_error(interp, "INFO EXISTS: wrong # args: should be \"INFO EXISTS varname\"");
        return BCL_ERROR;
    }

    const char *varname = argv[0];
    bool exists = bcl_var_exists(interp, varname);

    *result = bcl_value_create(exists ? "1" : "0");
    return BCL_OK;
}

/* ========================================================================== */
/* INFO GLOBALS - Lista variables globales                                   */
/* ========================================================================== */

static bcl_result_t info_globals(bcl_interp_t *interp, int argc, char **argv,
                                 bcl_value_t **result) {
    /* Por ahora es igual que INFO VARS ya que no tenemos scopes locales activos */
    return info_vars(interp, argc, argv, result);
}

/* ========================================================================== */
/* INFO LOCALS - Lista variables locales                                     */
/* ========================================================================== */

static bcl_result_t info_locals(bcl_interp_t *interp, int argc, char **argv,
                                bcl_value_t **result) {
    BCL_UNUSED(argc);
    BCL_UNUSED(argv);

    /* TODO: Cuando implementemos scopes, listar variables del scope actual */
    /* Por ahora, retornar vacío */
    *result = bcl_value_create("");
    return BCL_OK;
}

/* ========================================================================== */
/* INFO ARGS - Argumentos de un procedimiento                                */
/* ========================================================================== */

static bcl_result_t info_args(bcl_interp_t *interp, int argc, char **argv,
                              bcl_value_t **result) {
    if (argc < 1) {
        bcl_set_error(interp, "INFO ARGS: wrong # args: should be \"INFO ARGS procname\"");
        return BCL_ERROR;
    }

    const char *procname = argv[0];

    /* Buscar procedimiento */
    if (!bcl_proc_exists(interp, procname)) {
        bcl_set_error(interp, "INFO ARGS: \"%s\" isn't a procedure", procname);
        return BCL_ERROR;
    }

    /* Obtener procedimiento */
    bcl_value_t *proc_val = bcl_hash_get(interp->procedures, procname);
    if (!proc_val) {
        bcl_set_error(interp, "INFO ARGS: procedure \"%s\" not found", procname);
        return BCL_ERROR;
    }

    /* Convertir string a puntero */
    bcl_procedure_t *proc;
    sscanf(bcl_value_get(proc_val), "%p", (void**)&proc);

    if (!proc) {
        bcl_set_error(interp, "INFO ARGS: corrupted procedure \"%s\"", procname);
        return BCL_ERROR;
    }

    /* Construir lista de parámetros */
    bcl_string_t *params = bcl_string_create("");
    for (size_t i = 0; i < proc->param_count; i++) {
        if (i > 0) bcl_string_append(params, " ");
        if (proc->params[i].optional) {
            bcl_string_append(params, "@");
        }
        bcl_string_append(params, proc->params[i].name);
    }

    *result = bcl_value_create(bcl_string_cstr(params));
    bcl_string_destroy(params);

    return BCL_OK;
}

/* ========================================================================== */
/* INFO BODY - Retorna el cuerpo de un procedimiento                         */
/* ========================================================================== */

static bcl_result_t info_body(bcl_interp_t *interp, int argc, char **argv,
                              bcl_value_t **result) {
    if (argc < 1) {
        bcl_set_error(interp, "INFO BODY: wrong # args: should be \"INFO BODY procname\"");
        return BCL_ERROR;
    }

    const char *procname = argv[0];

    /* Buscar procedimiento */
    if (!bcl_proc_exists(interp, procname)) {
        bcl_set_error(interp, "INFO BODY: \"%s\" isn't a procedure", procname);
        return BCL_ERROR;
    }

    /* Obtener procedimiento */
    bcl_value_t *proc_val = bcl_hash_get(interp->procedures, procname);
    if (!proc_val) {
        bcl_set_error(interp, "INFO BODY: procedure \"%s\" not found", procname);
        return BCL_ERROR;
    }

    /* El cuerpo del procedimiento está almacenado como un bloque pre-parseado
     * Por ahora, retornamos un placeholder indicando que el cuerpo existe */
    *result = bcl_value_create("[procedure body]");
    return BCL_OK;
}

/* ========================================================================== */
/* INFO BCLVERSION - Versión del intérprete                                  */
/* ========================================================================== */

static bcl_result_t info_bclversion(bcl_interp_t *interp, int argc, char **argv,
                                    bcl_value_t **result) {
    BCL_UNUSED(interp);
    BCL_UNUSED(argc);
    BCL_UNUSED(argv);

    *result = bcl_value_create(BCL_VERSION);
    return BCL_OK;
}

/* ========================================================================== */
/* INFO - Comando principal                                                  */
/* ========================================================================== */

bcl_result_t bcl_cmd_info(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result) {
    if (argc < 1) {
        bcl_set_error(interp, "INFO: wrong # args: should be \"INFO subcommand ?arg ...?\"");
        return BCL_ERROR;
    }

    const char *subcmd = argv[0];

    /* Dispatch a subcomandos según BCL_INFO_Referencia_v1.0.txt */
    if (bcl_strcasecmp(subcmd, "EXISTS") == 0) {
        return info_exists(interp, argc - 1, argv + 1, result);
    }
    else if (bcl_strcasecmp(subcmd, "ARGS") == 0) {
        return info_args(interp, argc - 1, argv + 1, result);
    }
    else if (bcl_strcasecmp(subcmd, "COMMANDS") == 0) {
        return info_commands(interp, argc - 1, argv + 1, result);
    }
    else if (bcl_strcasecmp(subcmd, "GLOBALS") == 0) {
        return info_globals(interp, argc - 1, argv + 1, result);
    }
    else if (bcl_strcasecmp(subcmd, "LOCALS") == 0) {
        return info_locals(interp, argc - 1, argv + 1, result);
    }
    else if (bcl_strcasecmp(subcmd, "PROCS") == 0) {
        return info_procs(interp, argc - 1, argv + 1, result);
    }
    else if (bcl_strcasecmp(subcmd, "VARS") == 0) {
        return info_vars(interp, argc - 1, argv + 1, result);
    }
    else if (bcl_strcasecmp(subcmd, "BODY") == 0) {
        return info_body(interp, argc - 1, argv + 1, result);
    }
    else if (bcl_strcasecmp(subcmd, "BCLVERSION") == 0) {
        return info_bclversion(interp, argc - 1, argv + 1, result);
    }
    else {
        bcl_set_error(interp, "INFO: unknown subcommand \"%s\": must be EXISTS, ARGS, BODY, COMMANDS, GLOBALS, LOCALS, PROCS, VARS, or BCLVERSION", subcmd);
        return BCL_ERROR;
    }
}
