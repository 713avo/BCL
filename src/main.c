/**
 * @file main.c
 * @brief Punto de entrada del intérprete BCL
 */

#include "bcl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [script.bcl [args...]]\n", progname);
    fprintf(stderr, "\n");
    fprintf(stderr, "If no script is provided, enters REPL mode.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Examples:\n");
    fprintf(stderr, "  %s                    # REPL mode\n", progname);
    fprintf(stderr, "  %s script.bcl         # Execute script\n", progname);
    fprintf(stderr, "  %s prog.bcl a b c     # Execute with arguments\n", progname);
}

int main(int argc, char **argv) {
    /* Crear intérprete */
    bcl_interp_t *interp = bcl_interp_create();
    if (!interp) {
        fprintf(stderr, "Error: Failed to create interpreter\n");
        return 1;
    }

    int exit_code = 0;

    if (argc == 1) {
        /* Modo REPL */
        exit_code = bcl_repl(interp);
    } else {
        /* Modo script */
        const char *script_file = argv[1];

        /* Guardar argumentos para ARGV */
        interp->argc = argc - 2;  /* Excluir programa y script */
        interp->argv = argv + 2;  /* Apuntar a los args del script */

        /* Ejecutar script */
        bcl_result_t res = bcl_eval_file(interp, script_file);

        if (res == BCL_ERROR) {
            fprintf(stderr, "Error: %s\n", bcl_get_error(interp));
            exit_code = 1;
        } else if (res == BCL_EXIT) {
            exit_code = interp->exit_code;
        }
    }

    /* Destruir intérprete */
    bcl_interp_destroy(interp);

    return exit_code;
}
