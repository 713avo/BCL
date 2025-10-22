/**
 * @file bcl_commands.c
 * @brief Implementación de comandos básicos de BCL
 */

#include "bcl.h"
#include "bcl_commands.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ========================================================================== */
/* SET - Asignar o mostrar variable                                          */
/* ========================================================================== */

bcl_result_t bcl_cmd_set(bcl_interp_t *interp, int argc, char **argv,
                         bcl_value_t **result) {
    if (argc < 1 || argc > 2) {
        bcl_set_error(interp, "SET: wrong # args: should be \"SET varname ?value?\"");
        return BCL_ERROR;
    }

    const char *varname = argv[0];

    if (argc == 1) {
        /* Leer valor */
        bcl_value_t *val = bcl_var_get(interp, varname);
        if (!val) {
            bcl_set_error(interp, "can't read \"%s\": no such variable", varname);
            return BCL_ERROR;
        }
        *result = bcl_value_clone(val);
    } else {
        /* Asignar valor */
        const char *value = argv[1];
        if (bcl_var_set(interp, varname, value) != BCL_OK) {
            return BCL_ERROR;
        }
        *result = bcl_value_create(value);
    }

    return BCL_OK;
}

/* ========================================================================== */
/* UNSET - Eliminar variable                                                 */
/* ========================================================================== */

bcl_result_t bcl_cmd_unset(bcl_interp_t *interp, int argc, char **argv,
                           bcl_value_t **result) {
    if (argc != 1) {
        bcl_set_error(interp, "UNSET: wrong # args: should be \"UNSET varname\"");
        return BCL_ERROR;
    }

    bcl_var_unset(interp, argv[0]);
    *result = bcl_value_create("");
    return BCL_OK;
}

/* ========================================================================== */
/* INCR - Incrementar variable                                               */
/* ========================================================================== */

bcl_result_t bcl_cmd_incr(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result) {
    if (argc < 1 || argc > 2) {
        bcl_set_error(interp, "INCR: wrong # args: should be \"INCR varname ?increment?\"");
        return BCL_ERROR;
    }

    const char *varname = argv[0];
    double increment = 1.0;

    if (argc == 2) {
        bool ok;
        increment = bcl_str_to_number(argv[1], &ok);
        if (!ok) {
            bcl_set_error(interp, "expected integer but got \"%s\"", argv[1]);
            return BCL_ERROR;
        }
    }

    /* Obtener valor actual */
    bcl_value_t *val = bcl_var_get(interp, varname);
    double current = 0.0;

    if (val) {
        bool ok;
        current = bcl_value_to_number(val, &ok);
        if (!ok) {
            bcl_set_error(interp, "expected integer but got \"%s\"",
                         bcl_value_get(val));
            return BCL_ERROR;
        }
    }

    /* Incrementar */
    double new_value = current + increment;

    /* Guardar */
    char buf[64];
    snprintf(buf, sizeof(buf), "%.0f", new_value);
    bcl_var_set(interp, varname, buf);

    *result = bcl_value_create(buf);
    return BCL_OK;
}

/* ========================================================================== */
/* APPEND - Concatenar a variable                                            */
/* ========================================================================== */

bcl_result_t bcl_cmd_append(bcl_interp_t *interp, int argc, char **argv,
                            bcl_value_t **result) {
    if (argc < 1) {
        bcl_set_error(interp, "APPEND: wrong # args: should be \"APPEND varname ?value ...?\"");
        return BCL_ERROR;
    }

    const char *varname = argv[0];

    /* Obtener valor actual */
    bcl_value_t *val = bcl_var_get(interp, varname);
    bcl_string_t *str;

    if (val) {
        str = bcl_string_create(bcl_value_get(val));
    } else {
        str = bcl_string_create("");
    }

    /* Concatenar argumentos */
    for (int i = 1; i < argc; i++) {
        bcl_string_append(str, argv[i]);
    }

    /* Guardar */
    bcl_var_set(interp, varname, bcl_string_cstr(str));

    *result = bcl_value_create(bcl_string_cstr(str));
    bcl_string_destroy(str);

    return BCL_OK;
}

/* ========================================================================== */
/* PUTS - Imprimir con nueva línea                                           */
/* ========================================================================== */

bcl_result_t bcl_cmd_puts(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result) {
#ifndef BCL_NO_FILES
    /* Si el primer argumento es un handle válido, escribir al archivo */
    if (argc >= 1 && bcl_is_file_handle(interp, argv[0])) {
        /* Concatenar el resto de argumentos */
        bcl_string_t *text = bcl_string_create("");
        for (int i = 1; i < argc; i++) {
            if (i > 1) bcl_string_append_char(text, ' ');
            bcl_string_append(text, argv[i]);
        }

        bcl_result_t res = bcl_file_puts(interp, argv[0], bcl_string_cstr(text), result);
        bcl_string_destroy(text);
        return res;
    }
#endif

    /* Sin handle: escribir todo a stdout */
    for (int i = 0; i < argc; i++) {
        if (i > 0) fwrite(" ", 1, 1, stdout);
        /* Usar fwrite para soportar bytes binarios (secuencias ANSI) */
        size_t len = strlen(argv[i]);
        fwrite(argv[i], 1, len, stdout);
    }
    fwrite("\n", 1, 1, stdout);
    fflush(stdout);

    *result = bcl_value_create("");
    return BCL_OK;
}

/* ========================================================================== */
/* PUTSN - Imprimir sin nueva línea                                          */
/* ========================================================================== */

bcl_result_t bcl_cmd_putsn(bcl_interp_t *interp, int argc, char **argv,
                           bcl_value_t **result) {
#ifndef BCL_NO_FILES
    /* Si el primer argumento es un handle válido, escribir al archivo */
    if (argc >= 1 && bcl_is_file_handle(interp, argv[0])) {
        /* Concatenar el resto de argumentos */
        bcl_string_t *text = bcl_string_create("");
        for (int i = 1; i < argc; i++) {
            if (i > 1) bcl_string_append_char(text, ' ');
            bcl_string_append(text, argv[i]);
        }

        bcl_result_t res = bcl_file_putsn(interp, argv[0], bcl_string_cstr(text), result);
        bcl_string_destroy(text);
        return res;
    }
#endif

    /* Sin handle: escribir todo a stdout */
    for (int i = 0; i < argc; i++) {
        if (i > 0) fwrite(" ", 1, 1, stdout);
        /* Usar fwrite para soportar bytes binarios (secuencias ANSI) */
        size_t len = strlen(argv[i]);
        fwrite(argv[i], 1, len, stdout);
    }
    fflush(stdout);

    *result = bcl_value_create("");
    return BCL_OK;
}

/* ========================================================================== */
/* GETS - Leer línea                                                         */
/* ========================================================================== */

bcl_result_t bcl_cmd_gets(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result) {
#ifndef BCL_NO_FILES
    /* Si hay argumento y es un handle válido, leer del archivo */
    if (argc == 1 && bcl_is_file_handle(interp, argv[0])) {
        return bcl_file_gets(interp, argv[0], result);
    }
#endif

    /* Sin argumentos o no es handle: leer de stdin */
    if (argc > 0) {
        bcl_set_error(interp, "wrong # args: should be \"GETS ?handle?\"");
        return BCL_ERROR;
    }

    char line[BCL_MAX_LINE_LEN];
    if (fgets(line, sizeof(line), stdin)) {
        /* Eliminar newline final si existe */
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        *result = bcl_value_create(line);
    } else {
        *result = bcl_value_create("");
    }

    return BCL_OK;
}

/* ========================================================================== */
/* EXIT - Salir                                                              */
/* ========================================================================== */

bcl_result_t bcl_cmd_exit(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result) {
    int code = 0;

    if (argc > 0) {
        bool ok;
        code = (int)bcl_str_to_number(argv[0], &ok);
        if (!ok) {
            bcl_set_error(interp, "expected integer but got \"%s\"", argv[0]);
            return BCL_ERROR;
        }
    }

    interp->exit_code = code;
    *result = bcl_value_create("");
    return BCL_EXIT;
}

/* ========================================================================== */
/* GLOBAL - Declarar globales (placeholder)                                  */
/* ========================================================================== */

bcl_result_t bcl_cmd_global(bcl_interp_t *interp, int argc, char **argv,
                            bcl_value_t **result) {
    if (result) *result = NULL;

    if (argc < 1) {
        bcl_set_error(interp, "GLOBAL: wrong # args: should be \"GLOBAL varName ?varName ...?\"");
        return BCL_ERROR;
    }

    /* GLOBAL solo tiene efecto si estamos en un scope local */
    if (interp->scope_depth == 0) {
        /* Estamos en scope global, GLOBAL no hace nada */
        if (result) *result = bcl_value_create("");
        return BCL_OK;
    }

    /* Obtener scope actual */
    bcl_scope_t *scope = interp->scope_stack[interp->scope_depth - 1];
    if (!scope) return BCL_ERROR;

    /* Marcar cada variable como global */
    for (int i = 0; i < argc; i++) {
        const char *varname = argv[i];

        /* Agregar a global_refs con un valor dummy */
        bcl_value_t *marker = bcl_value_create("1");
        bcl_hash_set(scope->global_refs, varname, marker);
    }

    if (result) *result = bcl_value_create("");
    return BCL_OK;
}

/* ========================================================================== */
/* BREAK - Romper bucle                                                      */
/* ========================================================================== */

bcl_result_t bcl_cmd_break(bcl_interp_t *interp, int argc, char **argv,
                           bcl_value_t **result) {
    BCL_UNUSED(interp);
    BCL_UNUSED(argc);
    BCL_UNUSED(argv);

    *result = bcl_value_create("");
    return BCL_BREAK;
}

/* ========================================================================== */
/* CONTINUE - Continuar bucle                                                */
/* ========================================================================== */

bcl_result_t bcl_cmd_continue(bcl_interp_t *interp, int argc, char **argv,
                              bcl_value_t **result) {
    BCL_UNUSED(interp);
    BCL_UNUSED(argc);
    BCL_UNUSED(argv);

    *result = bcl_value_create("");
    return BCL_CONTINUE;
}

/* ========================================================================== */
/* RETURN - Retornar de procedimiento                                        */
/* ========================================================================== */

bcl_result_t bcl_cmd_return(bcl_interp_t *interp, int argc, char **argv,
                            bcl_value_t **result) {
    /* Guardar valor de retorno en el intérprete */
    if (argc > 0) {
        /* Concatenar todos los argumentos como valor de retorno */
        bcl_string_t *ret_val = bcl_string_create("");
        for (int i = 0; i < argc; i++) {
            if (i > 0) bcl_string_append(ret_val, " ");
            bcl_string_append(ret_val, argv[i]);
        }

        if (interp->return_value) {
            bcl_value_destroy(interp->return_value);
        }
        interp->return_value = bcl_value_create(bcl_string_cstr(ret_val));
        bcl_string_destroy(ret_val);
    } else {
        /* Sin valor - retornar cadena vacía */
        if (interp->return_value) {
            bcl_value_destroy(interp->return_value);
        }
        interp->return_value = bcl_value_create("");
    }

    /* Retornar el valor también en result si se proporciona */
    if (result) {
        *result = bcl_value_clone(interp->return_value);
    }

    return BCL_RETURN;
}
