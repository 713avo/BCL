/**
 * @file bcl_commands.h
 * @brief BCL - Declaraciones de comandos
 * @version 1.5.0
 *
 * Este archivo declara todos los comandos del lenguaje BCL organizados
 * por categorías según las especificaciones.
 */

#ifndef BCL_COMMANDS_H
#define BCL_COMMANDS_H

#include "bcl.h"

/* ========================================================================== */
/* COMANDOS DE VARIABLES                                                     */
/* ========================================================================== */

/**
 * SET nombre [valor]
 * Asigna o muestra el valor de una variable
 */
bcl_result_t bcl_cmd_set(bcl_interp_t *interp, int argc, char **argv,
                         bcl_value_t **result);

/**
 * UNSET nombre
 * Elimina una variable
 */
bcl_result_t bcl_cmd_unset(bcl_interp_t *interp, int argc, char **argv,
                           bcl_value_t **result);

/**
 * GLOBAL var1 var2 ...
 * Declara variables globales dentro de PROC
 */
bcl_result_t bcl_cmd_global(bcl_interp_t *interp, int argc, char **argv,
                            bcl_value_t **result);

/**
 * INCR var [incremento]
 * Incrementa una variable numérica
 */
bcl_result_t bcl_cmd_incr(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result);

/**
 * APPEND var valor1 valor2 ...
 * Concatena valores al final de una variable
 */
bcl_result_t bcl_cmd_append(bcl_interp_t *interp, int argc, char **argv,
                            bcl_value_t **result);

/* ========================================================================== */
/* COMANDOS DE I/O                                                           */
/* ========================================================================== */

/**
 * PUTS texto
 * Imprime texto con nueva línea
 */
bcl_result_t bcl_cmd_puts(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result);

/**
 * PUTSN texto
 * Imprime texto sin nueva línea
 */
bcl_result_t bcl_cmd_putsn(bcl_interp_t *interp, int argc, char **argv,
                           bcl_value_t **result);

/**
 * GETS [handle]
 * Lee una línea desde stdin o archivo
 */
bcl_result_t bcl_cmd_gets(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result);

/* ========================================================================== */
/* CONTROL DE FLUJO                                                          */
/* ========================================================================== */

/**
 * IF condición THEN ... [ELSEIF ...] [ELSE ...] END
 */
bcl_result_t bcl_cmd_if(bcl_interp_t *interp, int argc, char **argv,
                        bcl_value_t **result);

/**
 * SWITCH expr DO CASE val1 ... DEFAULT ... END
 */
bcl_result_t bcl_cmd_switch(bcl_interp_t *interp, int argc, char **argv,
                            bcl_value_t **result);

/**
 * WHILE condición DO ... END
 */
bcl_result_t bcl_cmd_while(bcl_interp_t *interp, int argc, char **argv,
                           bcl_value_t **result);

/**
 * FOR ... TO ... [STEP ...] DO ... END
 */
bcl_result_t bcl_cmd_for(bcl_interp_t *interp, int argc, char **argv,
                         bcl_value_t **result);

/**
 * FOREACH var IN lista DO ... END
 */
bcl_result_t bcl_cmd_foreach(bcl_interp_t *interp, int argc, char **argv,
                             bcl_value_t **result);

/**
 * BREAK
 */
bcl_result_t bcl_cmd_break(bcl_interp_t *interp, int argc, char **argv,
                           bcl_value_t **result);

/**
 * CONTINUE
 */
bcl_result_t bcl_cmd_continue(bcl_interp_t *interp, int argc, char **argv,
                              bcl_value_t **result);

/**
 * EXIT [código]
 */
bcl_result_t bcl_cmd_exit(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result);

/* ========================================================================== */
/* PROCEDIMIENTOS                                                            */
/* ========================================================================== */

/**
 * PROC nombre WITH param1 param2 ... DO ... END
 */
bcl_result_t bcl_cmd_proc(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result);

/**
 * RETURN [valor]
 */
bcl_result_t bcl_cmd_return(bcl_interp_t *interp, int argc, char **argv,
                            bcl_value_t **result);

/* ========================================================================== */
/* EXPRESIONES                                                               */
/* ========================================================================== */

/**
 * EXPR expresión
 * Evalúa expresión aritmética/lógica
 */
bcl_result_t bcl_cmd_expr(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result);

/* ========================================================================== */
/* ARRAYS                                                                    */
/* ========================================================================== */

/**
 * ARRAY subcomando arrayName ...
 * Comando principal de manipulación de arrays asociativos
 */
bcl_result_t bcl_cmd_array(bcl_interp_t *interp, int argc, char **argv,
                           bcl_value_t **result);

/* ========================================================================== */
/* BINARY DATA                                                               */
/* ========================================================================== */

/**
 * BINARY subcomando ...
 * Manipulación de datos binarios (FORMAT y SCAN)
 */
bcl_result_t bcl_cmd_binary(bcl_interp_t *interp, int argc, char **argv,
                            bcl_value_t **result);

/* ========================================================================== */
/* LISTAS                                                                    */
/* ========================================================================== */

bcl_result_t bcl_cmd_list(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result);
bcl_result_t bcl_cmd_split(bcl_interp_t *interp, int argc, char **argv,
                           bcl_value_t **result);
bcl_result_t bcl_cmd_join(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result);
bcl_result_t bcl_cmd_lindex(bcl_interp_t *interp, int argc, char **argv,
                            bcl_value_t **result);
bcl_result_t bcl_cmd_lrange(bcl_interp_t *interp, int argc, char **argv,
                            bcl_value_t **result);
bcl_result_t bcl_cmd_llength(bcl_interp_t *interp, int argc, char **argv,
                             bcl_value_t **result);
bcl_result_t bcl_cmd_lappend(bcl_interp_t *interp, int argc, char **argv,
                             bcl_value_t **result);
bcl_result_t bcl_cmd_linsert(bcl_interp_t *interp, int argc, char **argv,
                             bcl_value_t **result);
bcl_result_t bcl_cmd_lreplace(bcl_interp_t *interp, int argc, char **argv,
                              bcl_value_t **result);
bcl_result_t bcl_cmd_concat(bcl_interp_t *interp, int argc, char **argv,
                            bcl_value_t **result);
bcl_result_t bcl_cmd_lsort(bcl_interp_t *interp, int argc, char **argv,
                           bcl_value_t **result);
bcl_result_t bcl_cmd_lsearch(bcl_interp_t *interp, int argc, char **argv,
                             bcl_value_t **result);

/* ========================================================================== */
/* STRINGS                                                                   */
/* ========================================================================== */

/**
 * STRING subcomando ...
 * Comando principal de manipulación de strings
 */
bcl_result_t bcl_cmd_string(bcl_interp_t *interp, int argc, char **argv,
                            bcl_value_t **result);

/* Subcomandos de STRING (implementaciones internas) */
bcl_result_t bcl_string_cat(bcl_interp_t *interp, int argc, char **argv,
                            bcl_value_t **result);
bcl_result_t bcl_string_compare(bcl_interp_t *interp, int argc, char **argv,
                                bcl_value_t **result);
bcl_result_t bcl_string_equal(bcl_interp_t *interp, int argc, char **argv,
                              bcl_value_t **result);
bcl_result_t bcl_string_first(bcl_interp_t *interp, int argc, char **argv,
                              bcl_value_t **result);
bcl_result_t bcl_string_last(bcl_interp_t *interp, int argc, char **argv,
                             bcl_value_t **result);
bcl_result_t bcl_string_index(bcl_interp_t *interp, int argc, char **argv,
                              bcl_value_t **result);
bcl_result_t bcl_string_is(bcl_interp_t *interp, int argc, char **argv,
                           bcl_value_t **result);
bcl_result_t bcl_string_length(bcl_interp_t *interp, int argc, char **argv,
                               bcl_value_t **result);
bcl_result_t bcl_string_map(bcl_interp_t *interp, int argc, char **argv,
                            bcl_value_t **result);
bcl_result_t bcl_string_match(bcl_interp_t *interp, int argc, char **argv,
                              bcl_value_t **result);
bcl_result_t bcl_string_range(bcl_interp_t *interp, int argc, char **argv,
                              bcl_value_t **result);
bcl_result_t bcl_string_repeat(bcl_interp_t *interp, int argc, char **argv,
                               bcl_value_t **result);
bcl_result_t bcl_string_replace(bcl_interp_t *interp, int argc, char **argv,
                                bcl_value_t **result);
bcl_result_t bcl_string_reverse(bcl_interp_t *interp, int argc, char **argv,
                                bcl_value_t **result);
bcl_result_t bcl_string_tolower(bcl_interp_t *interp, int argc, char **argv,
                                bcl_value_t **result);
bcl_result_t bcl_string_totitle(bcl_interp_t *interp, int argc, char **argv,
                                bcl_value_t **result);
bcl_result_t bcl_string_toupper(bcl_interp_t *interp, int argc, char **argv,
                                bcl_value_t **result);
bcl_result_t bcl_string_trim(bcl_interp_t *interp, int argc, char **argv,
                             bcl_value_t **result);
bcl_result_t bcl_string_trimleft(bcl_interp_t *interp, int argc, char **argv,
                                 bcl_value_t **result);
bcl_result_t bcl_string_trimright(bcl_interp_t *interp, int argc, char **argv,
                                  bcl_value_t **result);
bcl_result_t bcl_string_wordstart(bcl_interp_t *interp, int argc, char **argv,
                                  bcl_value_t **result);
bcl_result_t bcl_string_wordend(bcl_interp_t *interp, int argc, char **argv,
                                bcl_value_t **result);

/* ========================================================================== */
/* FORMAT Y SCAN                                                             */
/* ========================================================================== */

/**
 * FORMAT plantilla arg1 arg2 ...
 */
bcl_result_t bcl_cmd_format(bcl_interp_t *interp, int argc, char **argv,
                            bcl_value_t **result);

/**
 * SCAN texto plantilla var1 var2 ...
 */
bcl_result_t bcl_cmd_scan(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result);

/* ========================================================================== */
/* ARCHIVOS                                                                  */
/* ========================================================================== */

#ifndef BCL_NO_FILES

/* Comandos de archivo */
bcl_result_t bcl_cmd_open(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result);
bcl_result_t bcl_cmd_close(bcl_interp_t *interp, int argc, char **argv,
                           bcl_value_t **result);
bcl_result_t bcl_cmd_read(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result);
bcl_result_t bcl_cmd_tell(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result);
bcl_result_t bcl_cmd_seek(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result);
bcl_result_t bcl_cmd_eof(bcl_interp_t *interp, int argc, char **argv,
                         bcl_value_t **result);

/**
 * FILE subcomando ...
 */
bcl_result_t bcl_cmd_file(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result);

/**
 * PWD
 */
bcl_result_t bcl_cmd_pwd(bcl_interp_t *interp, int argc, char **argv,
                         bcl_value_t **result);

/**
 * GLOB patrón [opciones...]
 */
bcl_result_t bcl_cmd_glob(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result);

/* Helpers para GETS/PUTS/PUTSN con handles */
bcl_result_t bcl_file_gets(bcl_interp_t *interp, const char *handle_name,
                           bcl_value_t **result);
bcl_result_t bcl_file_puts(bcl_interp_t *interp, const char *handle_name,
                           const char *text, bcl_value_t **result);
bcl_result_t bcl_file_putsn(bcl_interp_t *interp, const char *handle_name,
                            const char *text, bcl_value_t **result);
bool bcl_is_file_handle(bcl_interp_t *interp, const char *name);

#endif /* BCL_NO_FILES */

/* ========================================================================== */
/* EXPRESIONES REGULARES                                                     */
/* ========================================================================== */

/**
 * REGEXP patrón texto [opciones...]
 */
bcl_result_t bcl_cmd_regexp(bcl_interp_t *interp, int argc, char **argv,
                            bcl_value_t **result);

/**
 * REGSUB patrón texto reemplazo [opciones...]
 */
bcl_result_t bcl_cmd_regsub(bcl_interp_t *interp, int argc, char **argv,
                            bcl_value_t **result);

/* ========================================================================== */
/* INTROSPECCIÓN (INFO)                                                      */
/* ========================================================================== */

/**
 * INFO subcomando ...
 */
bcl_result_t bcl_cmd_info(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result);

/* ========================================================================== */
/* TIEMPO (CLOCK)                                                            */
/* ========================================================================== */

/**
 * CLOCK subcomando ...
 */
bcl_result_t bcl_cmd_clock(bcl_interp_t *interp, int argc, char **argv,
                           bcl_value_t **result);

/* ========================================================================== */
/* SISTEMA                                                                   */
/* ========================================================================== */

/**
 * EVAL código
 */
bcl_result_t bcl_cmd_eval(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result);

/**
 * SOURCE ruta
 */
bcl_result_t bcl_cmd_source(bcl_interp_t *interp, int argc, char **argv,
                            bcl_value_t **result);

/**
 * LOAD ruta_extension
 * Carga una extensión dinámica (.so)
 */
bcl_result_t bcl_cmd_load(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result);

/**
 * AFTER milisegundos
 */
bcl_result_t bcl_cmd_after(bcl_interp_t *interp, int argc, char **argv,
                           bcl_value_t **result);

/**
 * EXEC comando [args...]
 */
bcl_result_t bcl_cmd_exec(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result);

/**
 * ENV nombre
 */
bcl_result_t bcl_cmd_env(bcl_interp_t *interp, int argc, char **argv,
                         bcl_value_t **result);

/**
 * ARGV
 */
bcl_result_t bcl_cmd_argv(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result);

/* ========================================================================== */
/* DISPATCHER DE COMANDOS                                                    */
/* ========================================================================== */

/**
 * @brief Tipo de función para comandos
 */
typedef bcl_result_t (*bcl_command_func_t)(bcl_interp_t *interp, int argc,
                                           char **argv, bcl_value_t **result);

/**
 * @brief Registro de comando
 */
typedef struct {
    const char *name;
    bcl_command_func_t func;
} bcl_command_entry_t;

/**
 * @brief Despacha un comando por nombre
 */
bcl_result_t bcl_dispatch_command(bcl_interp_t *interp, const char *name,
                                  int argc, char **argv, bcl_value_t **result);

/**
 * @brief Obtiene tabla de comandos
 */
const bcl_command_entry_t *bcl_get_command_table(size_t *count);

#endif /* BCL_COMMANDS_H */
