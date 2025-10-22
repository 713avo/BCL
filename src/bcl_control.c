/**
 * @file bcl_control.c
 * @brief Comandos de control de flujo (IF, WHILE, FOR, FOREACH, SWITCH)
 * @note Implementación simplificada - Fase 1: sintaxis inline
 */

#include "bcl.h"
#include "bcl_commands.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ========================================================================== */
/* HELPER: EVALUACIÓN DE CONDICIONES                                         */
/* ========================================================================== */

/**
 * Evalúa una condición y devuelve true/false
 * La condición puede ser una expresión como "5 > 3" o una variable "$x"
 */
static bool eval_condition(bcl_interp_t *interp, const char *condition) {
    if (!condition || !*condition) return false;

    /* Expandir variables en la condición */
    char *expanded = bcl_expand_vars(interp, condition);
    if (!expanded) return false;

    /* Tokenizar para EXPR */
    int argc = 0;
    char *argv[256]; /* max 256 tokens */

    char *p = expanded;
    char token_buf[BCL_MAX_TOKEN_LEN];

    while (*p) {
        /* Saltar espacios */
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) break;

        /* Extraer token */
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

    /* Liberar argumentos */
    for (int i = 0; i < argc; i++) {
        free(argv[i]);
    }

    if (res != BCL_OK || !result) {
        if (result) bcl_value_destroy(result);
        return false;
    }

    /* Convertir resultado a booleano */
    bool is_true = bcl_value_to_bool(result);
    bcl_value_destroy(result);

    return is_true;
}

/* ========================================================================== */
/* IF simplificado - Sintaxis inline (Fase 1)                                */
/* ========================================================================== */

/**
 * IF simplificado para testing
 * Sintaxis: IF condition THEN command ELSE command END
 *
 * Por ahora solo soporta comandos inline en una línea.
 * La implementación multi-línea completa requiere cambios arquitectónicos.
 */
bcl_result_t bcl_cmd_if(bcl_interp_t *interp, int argc, char **argv,
                        bcl_value_t **result) {
    if (result) *result = NULL;

    /* Syntax: IF <cond> THEN <cmd> [ELSE <cmd>] END */
    if (argc < 4) {
        bcl_set_error(interp, "IF: wrong # args: should be \"IF condition THEN command [ELSE command] END\"");
        return BCL_ERROR;
    }

    /* Encontrar THEN */
    int then_idx = -1;
    for (int i = 1; i < argc; i++) {
        if (bcl_strcasecmp(argv[i], "THEN") == 0) {
            then_idx = i;
            break;
        }
    }

    if (then_idx < 0) {
        bcl_set_error(interp, "IF: missing THEN keyword");
        return BCL_ERROR;
    }

    /* La condición está entre argv[0] y THEN */
    bcl_string_t *condition = bcl_string_create("");
    for (int i = 0; i < then_idx; i++) {
        if (i > 0) bcl_string_append(condition, " ");
        bcl_string_append(condition, argv[i]);
    }

    /* Evaluar condición */
    bool cond_result = eval_condition(interp, bcl_string_cstr(condition));
    bcl_string_destroy(condition);

    /* Buscar ELSE y END */
    int else_idx = -1;
    int end_idx = -1;

    for (int i = then_idx + 1; i < argc; i++) {
        if (bcl_strcasecmp(argv[i], "ELSE") == 0 && else_idx < 0) {
            else_idx = i;
        } else if (bcl_strcasecmp(argv[i], "END") == 0) {
            end_idx = i;
            break;
        }
    }

    if (end_idx < 0) {
        bcl_set_error(interp, "IF: missing END keyword");
        return BCL_ERROR;
    }

    /* Ejecutar rama correspondiente */
    int cmd_start, cmd_end;

    if (cond_result) {
        /* Ejecutar THEN */
        cmd_start = then_idx + 1;
        cmd_end = (else_idx >= 0) ? else_idx : end_idx;
    } else if (else_idx >= 0) {
        /* Ejecutar ELSE */
        cmd_start = else_idx + 1;
        cmd_end = end_idx;
    } else {
        /* No hay ELSE y condición falsa */
        return BCL_OK;
    }

    /* Construir comando y ejecutar */
    bcl_string_t *cmd = bcl_string_create("");
    for (int i = cmd_start; i < cmd_end; i++) {
        if (i > cmd_start) bcl_string_append(cmd, " ");
        bcl_string_append(cmd, argv[i]);
    }

    bcl_result_t res = BCL_OK;
    if (cmd->len > 0) { /* check string length directly */
        bcl_value_t *cmd_result = NULL;
        res = bcl_eval(interp, bcl_string_cstr(cmd), &cmd_result);
        if (result) {
            *result = cmd_result;
        } else if (cmd_result) {
            bcl_value_destroy(cmd_result);
        }
    }

    bcl_string_destroy(cmd);
    return res;
}

/* ========================================================================== */
/* WHILE / DO / END                                                          */
/* ========================================================================== */

bcl_result_t bcl_cmd_while(bcl_interp_t *interp, int argc, char **argv,
                           bcl_value_t **result) {
    (void)argc;
    (void)argv;
    (void)result;

    bcl_set_error(interp, "WHILE: multi-line control structures not yet implemented");
    return BCL_ERROR;
}

/* ========================================================================== */
/* FOR / TO / STEP / DO / END                                                */
/* ========================================================================== */

bcl_result_t bcl_cmd_for(bcl_interp_t *interp, int argc, char **argv,
                         bcl_value_t **result) {
    (void)argc;
    (void)argv;
    (void)result;

    bcl_set_error(interp, "FOR: multi-line control structures not yet implemented");
    return BCL_ERROR;
}

/* ========================================================================== */
/* FOREACH / IN / DO / END                                                   */
/* ========================================================================== */

bcl_result_t bcl_cmd_foreach(bcl_interp_t *interp, int argc, char **argv,
                             bcl_value_t **result) {
    (void)argc;
    (void)argv;
    (void)result;

    bcl_set_error(interp, "FOREACH: multi-line control structures not yet implemented");
    return BCL_ERROR;
}

/* ========================================================================== */
/* SWITCH / CASE / DEFAULT / END                                             */
/* ========================================================================== */

bcl_result_t bcl_cmd_switch(bcl_interp_t *interp, int argc, char **argv,
                            bcl_value_t **result) {
    (void)argc;
    (void)argv;
    (void)result;

    bcl_set_error(interp, "SWITCH: multi-line control structures not yet implemented");
    return BCL_ERROR;
}
