/**
 * @file bcl_system.c
 * @brief Comandos de sistema (EVAL, SOURCE, EXEC, ENV, ARGV, AFTER)
 * @version 1.0
 *
 * Comandos para interacción con el sistema y meta-programación
 */

#include "bcl.h"
#include "bcl_commands.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>

#ifndef BCL_NO_EXEC
#include <unistd.h>
#include <sys/wait.h>
#endif

/* ========================================================================== */
/* EVAL - Evaluar código BCL dinámicamente                                   */
/* ========================================================================== */

bcl_result_t bcl_cmd_eval(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result) {
    if (argc < 1) {
        bcl_set_error(interp, "wrong # args: should be \"EVAL code\"");
        return BCL_ERROR;
    }

    /* Concatenar todos los argumentos */
    bcl_string_t *code = bcl_string_create("");
    for (int i = 0; i < argc; i++) {
        if (i > 0) bcl_string_append(code, " ");
        bcl_string_append(code, argv[i]);
    }

    /* Evaluar el código y capturar resultado */
    bcl_value_t *eval_result = NULL;
    bcl_result_t res = bcl_eval(interp, bcl_string_cstr(code), &eval_result);

    /* Si hay resultado de retorno, usarlo */
    if (res == BCL_RETURN && interp->return_value) {
        if (result) {
            *result = bcl_value_clone(interp->return_value);
        }
        /* Limpiar estado de return */
        if (eval_result) bcl_value_destroy(eval_result);
        bcl_value_destroy(interp->return_value);
        interp->return_value = NULL;
        interp->flow_result = BCL_OK;
        res = BCL_OK;
    } else if (res == BCL_OK) {
        /* Retornar el resultado de la evaluación */
        if (result) {
            *result = eval_result ? eval_result : bcl_value_create("");
        } else if (eval_result) {
            bcl_value_destroy(eval_result);
        }
    } else {
        /* Error - limpiar */
        if (eval_result) bcl_value_destroy(eval_result);
    }

    bcl_string_destroy(code);
    return res;
}

/* ========================================================================== */
/* SOURCE - Ejecutar archivo BCL                                             */
/* ========================================================================== */

bcl_result_t bcl_cmd_source(bcl_interp_t *interp, int argc, char **argv,
                            bcl_value_t **result) {
    if (argc != 1) {
        bcl_set_error(interp, "wrong # args: should be \"SOURCE filename\"");
        return BCL_ERROR;
    }

    const char *filename = argv[0];

    /* Evaluar el archivo */
    bcl_result_t res = bcl_eval_file(interp, filename);

    /* Manejar resultado similar a EVAL */
    if (res == BCL_RETURN && interp->return_value) {
        if (result) {
            *result = bcl_value_clone(interp->return_value);
        }
        bcl_value_destroy(interp->return_value);
        interp->return_value = NULL;
        interp->flow_result = BCL_OK;
        res = BCL_OK;
    } else if (res == BCL_OK) {
        if (result) {
            *result = bcl_value_create("");
        }
    }

    return res;
}

/* ========================================================================== */
/* ENV - Obtener variable de entorno                                         */
/* ========================================================================== */

bcl_result_t bcl_cmd_env(bcl_interp_t *interp, int argc, char **argv,
                         bcl_value_t **result) {
    if (argc != 1) {
        bcl_set_error(interp, "wrong # args: should be \"ENV varname\"");
        return BCL_ERROR;
    }

    const char *varname = argv[0];
    const char *value = getenv(varname);

    /* Si la variable no existe, retornar cadena vacía en lugar de error */
    if (result) {
        *result = bcl_value_create(value ? value : "");
    }

    return BCL_OK;
}

/* ========================================================================== */
/* ARGV - Obtener argumentos del script                                      */
/* ========================================================================== */

bcl_result_t bcl_cmd_argv(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result) {
    BCL_UNUSED(argc);
    BCL_UNUSED(argv);

    /* Construir lista de argumentos */
    bcl_string_t *args_list = bcl_string_create("");

    for (int i = 0; i < interp->argc; i++) {
        if (i > 0) bcl_string_append(args_list, " ");
        bcl_string_append(args_list, interp->argv[i]);
    }

    if (result) {
        *result = bcl_value_create(bcl_string_cstr(args_list));
    }

    bcl_string_destroy(args_list);
    return BCL_OK;
}

/* ========================================================================== */
/* EXEC - Ejecutar comando externo                                           */
/* ========================================================================== */

#ifndef BCL_NO_EXEC

bcl_result_t bcl_cmd_exec(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result) {
    if (argc < 1) {
        bcl_set_error(interp, "wrong # args: should be \"EXEC command ?args?\"");
        return BCL_ERROR;
    }

    /* Construir comando completo */
    bcl_string_t *command = bcl_string_create("");
    for (int i = 0; i < argc; i++) {
        if (i > 0) bcl_string_append(command, " ");

        /* Escapar argumentos con espacios */
        if (strchr(argv[i], ' ')) {
            bcl_string_append(command, "\"");
            bcl_string_append(command, argv[i]);
            bcl_string_append(command, "\"");
        } else {
            bcl_string_append(command, argv[i]);
        }
    }

    const char *cmd = bcl_string_cstr(command);

    /* Ejecutar comando y capturar salida */
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        bcl_string_destroy(command);
        bcl_set_error(interp, "couldn't execute \"%s\": %s", cmd, strerror(errno));
        return BCL_ERROR;
    }

    /* Leer salida */
    bcl_string_t *output = bcl_string_create("");
    char buffer[4096];

    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        bcl_string_append(output, buffer);
    }

    /* Cerrar pipe y obtener código de salida */
    int status = pclose(fp);
    int exit_code = WEXITSTATUS(status);

    /* Eliminar newline final si existe */
    char *output_str = bcl_string_cstr(output);
    size_t len = strlen(output_str);
    if (len > 0 && output_str[len - 1] == '\n') {
        output_str[len - 1] = '\0';
    }

    /* Retornar salida del comando */
    if (result) {
        *result = bcl_value_create(output_str);
    }

    bcl_string_destroy(output);
    bcl_string_destroy(command);

    /* Si el comando falló, podríamos opcionalmente retornar error */
    /* Por ahora, solo retornamos la salida, como Tcl exec -ignorestderr */
    BCL_UNUSED(exit_code);

    return BCL_OK;
}

#else

bcl_result_t bcl_cmd_exec(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result) {
    BCL_UNUSED(argc);
    BCL_UNUSED(argv);
    BCL_UNUSED(result);
    bcl_set_error(interp, "EXEC command not available (BCL_NO_EXEC defined)");
    return BCL_ERROR;
}

#endif /* BCL_NO_EXEC */

/* ========================================================================== */
/* AFTER - Pausa/delay                                                       */
/* ========================================================================== */

bcl_result_t bcl_cmd_after(bcl_interp_t *interp, int argc, char **argv,
                           bcl_value_t **result) {
    if (argc != 1) {
        bcl_set_error(interp, "wrong # args: should be \"AFTER milliseconds\"");
        return BCL_ERROR;
    }

    /* Parsear milisegundos */
    bool ok;
    double ms = bcl_str_to_number(argv[0], &ok);

    if (!ok || ms < 0) {
        bcl_set_error(interp, "expected non-negative integer but got \"%s\"", argv[0]);
        return BCL_ERROR;
    }

    /* Convertir a microsegundos */
    unsigned long usec = (unsigned long)(ms * 1000);

    /* Dormir */
#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    usleep(usec);
#endif

    if (result) {
        *result = bcl_value_create("");
    }

    return BCL_OK;
}
