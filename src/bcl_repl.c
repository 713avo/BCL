/**
 * @file bcl_repl.c
 * @brief Implementación del REPL (Read-Eval-Print Loop) mejorado
 * @note Standalone - sin dependencias de readline
 *
 * Características:
 * - Soporte para comandos multilínea (IF...END, PROC...END, etc.)
 * - Historial de 10 comandos con navegación por flechas
 * - Terminal raw mode para captura de teclas especiales
 */

#include "bcl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Platform-specific includes for terminal handling */
#ifndef _WIN32
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#else
#include <conio.h>
#endif

/* ========================================================================== */
/* CONFIGURACIÓN                                                             */
/* ========================================================================== */

#define HISTORY_SIZE 10
#define MAX_CMD_BUFFER 8192

/* ========================================================================== */
/* ESTRUCTURA DE HISTORIAL                                                   */
/* ========================================================================== */

typedef struct {
    char *commands[HISTORY_SIZE];   /* Buffer circular de comandos */
    int count;                       /* Número de comandos almacenados */
    int current;                     /* Índice actual en historial */
    int position;                    /* Posición de navegación */
} bcl_history_t;

/* ========================================================================== */
/* GESTIÓN DE TERMINAL                                                       */
/* ========================================================================== */

#ifndef _WIN32
static struct termios orig_termios;
static bool raw_mode_enabled = false;

/**
 * @brief Habilita raw mode del terminal (Unix/macOS)
 */
static void enable_raw_mode(void) {
    if (raw_mode_enabled) return;

    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) return;

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);  /* Deshabilitar echo y modo canónico */
    raw.c_cc[VMIN] = 1;               /* Leer 1 byte a la vez */
    raw.c_cc[VTIME] = 0;              /* Sin timeout */

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != -1) {
        raw_mode_enabled = true;
    }
}

/**
 * @brief Deshabilita raw mode del terminal (Unix/macOS)
 */
static void disable_raw_mode(void) {
    if (!raw_mode_enabled) return;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    raw_mode_enabled = false;
}

/**
 * @brief Lee un carácter del terminal en raw mode
 */
static int read_char(void) {
    char c;
    if (read(STDIN_FILENO, &c, 1) == 1) {
        return (unsigned char)c;
    }
    return -1;
}

#else
/* Windows: usar _getch() */
static void enable_raw_mode(void) { /* No-op en Windows */ }
static void disable_raw_mode(void) { /* No-op en Windows */ }
static int read_char(void) { return _getch(); }
#endif

/* ========================================================================== */
/* HISTORIAL DE COMANDOS                                                     */
/* ========================================================================== */

/**
 * @brief Inicializa el historial
 */
static void history_init(bcl_history_t *hist) {
    memset(hist, 0, sizeof(bcl_history_t));
}

/**
 * @brief Añade comando al historial
 */
static void history_add(bcl_history_t *hist, const char *cmd) {
    if (!cmd || !*cmd) return;

    /* No añadir duplicados consecutivos */
    if (hist->count > 0) {
        int last_idx = (hist->current - 1 + HISTORY_SIZE) % HISTORY_SIZE;
        if (hist->commands[last_idx] &&
            strcmp(hist->commands[last_idx], cmd) == 0) {
            return;
        }
    }

    /* Liberar comando antiguo si existe */
    if (hist->commands[hist->current]) {
        free(hist->commands[hist->current]);
    }

    /* Guardar nuevo comando */
    hist->commands[hist->current] = strdup(cmd);
    hist->current = (hist->current + 1) % HISTORY_SIZE;

    if (hist->count < HISTORY_SIZE) {
        hist->count++;
    }

    /* Resetear posición de navegación */
    hist->position = hist->current;
}

/**
 * @brief Navega hacia atrás en el historial (flecha arriba)
 */
static const char* history_prev(bcl_history_t *hist) {
    if (hist->count == 0) return NULL;

    /* Si position está en current (después del último), ir al último comando */
    if (hist->position == hist->current) {
        hist->position = (hist->current - 1 + HISTORY_SIZE) % HISTORY_SIZE;
        return hist->commands[hist->position];
    }

    /* Calcular el índice del comando más antiguo */
    int oldest = (hist->current - hist->count + HISTORY_SIZE) % HISTORY_SIZE;

    /* Si ya estamos en el más antiguo, quedarse ahí */
    if (hist->position == oldest) {
        return hist->commands[hist->position];
    }

    /* Ir un comando hacia atrás */
    int prev_pos = (hist->position - 1 + HISTORY_SIZE) % HISTORY_SIZE;

    /* Verificar que no pasemos el más antiguo */
    /* En un buffer circular, esto es complicado, así que usamos conteo simple */
    int steps_back = 0;
    int temp_pos = hist->current;
    while (temp_pos != hist->position) {
        temp_pos = (temp_pos - 1 + HISTORY_SIZE) % HISTORY_SIZE;
        steps_back++;
    }

    /* Si ya retrocedimos count-1 pasos, estamos en el más antiguo */
    if (steps_back >= hist->count - 1) {
        return hist->commands[hist->position];
    }

    hist->position = prev_pos;
    return hist->commands[hist->position];
}

/**
 * @brief Navega hacia adelante en el historial (flecha abajo)
 */
static const char* history_next(bcl_history_t *hist) {
    if (hist->count == 0) return NULL;

    /* Si ya estamos en current (después del último), no hay nada más adelante */
    if (hist->position == hist->current) {
        return NULL;
    }

    /* Avanzar un comando */
    hist->position = (hist->position + 1) % HISTORY_SIZE;

    /* Si llegamos a current, retornar NULL (línea vacía) */
    if (hist->position == hist->current) {
        return NULL;
    }

    return hist->commands[hist->position];
}

/**
 * @brief Libera memoria del historial
 */
static void history_free(bcl_history_t *hist) {
    for (int i = 0; i < HISTORY_SIZE; i++) {
        if (hist->commands[i]) {
            free(hist->commands[i]);
            hist->commands[i] = NULL;
        }
    }
}

/* ========================================================================== */
/* DETECCIÓN DE COMANDOS ESTRUCTURADOS                                       */
/* ========================================================================== */

/**
 * @brief Verifica si una palabra es un comando estructurado que requiere END
 */
static bool is_structured_command(const char *word) {
    if (!word) return false;

    /* Comandos que requieren END */
    const char *structured[] = {
        "IF", "WHILE", "FOR", "FOREACH", "SWITCH", "PROC", NULL
    };

    for (int i = 0; structured[i]; i++) {
        if (bcl_strcasecmp(word, structured[i]) == 0) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Cuenta palabras clave de inicio/fin en una línea
 * @return > 0 si hay más inicios, < 0 si hay más END, 0 si están balanceados
 */
static int count_block_keywords(const char *line) {
    int balance = 0;
    char word[256];
    const char *p = line;

    while (*p) {
        /* Saltar espacios */
        while (*p && isspace(*p)) p++;
        if (!*p) break;

        /* Extraer palabra */
        int i = 0;
        while (*p && !isspace(*p) && i < 255) {
            word[i++] = *p++;
        }
        word[i] = '\0';

        /* Verificar si es comando estructurado */
        if (is_structured_command(word)) {
            balance++;
        } else if (bcl_strcasecmp(word, "END") == 0) {
            balance--;
        }
    }

    return balance;
}

/* ========================================================================== */
/* EDICIÓN DE LÍNEA CON HISTORIAL                                            */
/* ========================================================================== */

/**
 * @brief Lee una línea con soporte de historial y edición básica
 */
static int read_line_with_history(char *buffer, size_t size,
                                   bcl_history_t *hist, const char *prompt) {
    size_t pos = 0;
    buffer[0] = '\0';

    printf("%s", prompt);
    fflush(stdout);

    while (1) {
        int c = read_char();

        if (c == -1) {
            /* EOF */
            return -1;
        }

        /* Enter */
        if (c == '\n' || c == '\r') {
            printf("\n");
            buffer[pos] = '\0';
            return (int)pos;
        }

        /* Backspace */
        if (c == 127 || c == 8) {
            if (pos > 0) {
                pos--;
                printf("\b \b");  /* Borrar carácter en pantalla */
                fflush(stdout);
            }
            continue;
        }

        /* Ctrl+C */
        if (c == 3) {
            printf("^C\n");
            buffer[0] = '\0';
            return 0;
        }

        /* Ctrl+D */
        if (c == 4) {
            if (pos == 0) {
                printf("\n");
                return -1;  /* EOF */
            }
            continue;
        }

        /* Detectar secuencia de escape (flechas) */
        if (c == 27) {  /* ESC */
            int next1 = read_char();
            if (next1 == -1) continue;  /* Timeout o error */

            if (next1 == '[') {
                int next2 = read_char();
                if (next2 == -1) continue;  /* Timeout o error */

                /* Flecha arriba */
                if (next2 == 'A') {
                    const char *prev = history_prev(hist);
                    if (prev) {
                        /* Borrar línea actual */
                        while (pos > 0) {
                            printf("\b \b");
                            pos--;
                        }
                        /* Escribir comando del historial */
                        strncpy(buffer, prev, size - 1);
                        buffer[size - 1] = '\0';
                        pos = strlen(buffer);
                        printf("%s", buffer);
                        fflush(stdout);
                    }
                    continue;
                }

                /* Flecha abajo */
                if (next2 == 'B') {
                    const char *next = history_next(hist);
                    /* Borrar línea actual */
                    while (pos > 0) {
                        printf("\b \b");
                        pos--;
                    }
                    if (next) {
                        /* Escribir comando del historial */
                        strncpy(buffer, next, size - 1);
                        buffer[size - 1] = '\0';
                        pos = strlen(buffer);
                        printf("%s", buffer);
                    } else {
                        /* Línea vacía */
                        buffer[0] = '\0';
                        pos = 0;
                    }
                    fflush(stdout);
                    continue;
                }

                /* Otras flechas (izquierda/derecha) - ignorar por ahora */
                continue;
            } else {
                /* ESC seguido de algo que no es '[' - probablemente ESC solo */
                /* Ignorar el ESC y continuar con el siguiente carácter */
                continue;
            }
        }

        /* Carácter imprimible */
        if (c >= 32 && c < 127) {
            if (pos < size - 1) {
                buffer[pos++] = (char)c;
                putchar(c);
                fflush(stdout);
            }
        }
    }

    return (int)pos;
}

/* ========================================================================== */
/* REPL MEJORADO                                                             */
/* ========================================================================== */

int bcl_repl(bcl_interp_t *interp) {
    if (!interp) return 1;

    interp->interactive = true;

    printf("BCL Interpreter v%s\n", BCL_VERSION);
    printf("Type 'EXIT' to quit\n");
    printf("Multi-line commands (IF, PROC, etc.) supported\n");
    printf("Use ↑/↓ arrows for command history\n\n");

    /* Inicializar historial */
    bcl_history_t history;
    history_init(&history);

    /* Habilitar raw mode para captura de teclas */
    enable_raw_mode();

    /* Buffer para comandos multilínea */
    char cmd_buffer[MAX_CMD_BUFFER];
    cmd_buffer[0] = '\0';
    int block_depth = 0;  /* Contador de bloques anidados */

    while (1) {
        char line[BCL_MAX_LINE_LEN];

        /* Mostrar prompt diferente para continuación */
        const char *prompt = (block_depth > 0) ? "...> " : "BCL> ";

        /* Leer línea con historial */
        int len = read_line_with_history(line, sizeof(line), &history, prompt);

        if (len < 0) {
            /* EOF (Ctrl+D) */
            printf("\n");
            break;
        }

        /* Línea vacía */
        if (len == 0 && block_depth == 0) {
            continue;
        }

        /* Acumular línea en buffer */
        if (strlen(cmd_buffer) + strlen(line) + 2 < MAX_CMD_BUFFER) {
            if (cmd_buffer[0]) {
                strcat(cmd_buffer, "\n");
            }
            strcat(cmd_buffer, line);
        }

        /* Actualizar balance de bloques */
        block_depth += count_block_keywords(line);

        /* Si el comando no está completo, seguir leyendo */
        if (block_depth > 0) {
            continue;
        }

        /* Comando completo - añadir al historial */
        if (cmd_buffer[0]) {
            history_add(&history, cmd_buffer);
        }

        /* Evaluar comando */
        bcl_value_t *result = NULL;
        bcl_result_t res;

        /* Usar evaluador estructurado para comandos con bloques */
        if (strchr(cmd_buffer, '\n') != NULL) {
            /* Multilínea - usar eval_structured (no retorna valor) */
            res = bcl_eval_structured(interp, cmd_buffer);
        } else {
            /* Línea simple - usar eval normal que retorna valor */
            res = bcl_eval(interp, cmd_buffer, &result);
        }

        /* Limpiar buffer */
        cmd_buffer[0] = '\0';
        block_depth = 0;

        /* Manejar resultado */
        if (res == BCL_ERROR) {
            fprintf(stderr, "Error: %s\n", bcl_get_error(interp));
        } else if (res == BCL_EXIT) {
            if (result) bcl_value_destroy(result);
            break;
        } else if (res == BCL_OK) {
            /* En modo REPL, mostrar resultado si no es vacío */
            if (result) {
                const char *result_str = bcl_value_get(result);
                /* Solo mostrar si el resultado no es string vacío */
                if (result_str && result_str[0] != '\0') {
                    printf("%s\n", result_str);
                }
            }
        }

        if (result) {
            bcl_value_destroy(result);
        }
    }

    /* Limpiar */
    disable_raw_mode();
    history_free(&history);

    return interp->exit_code;
}
