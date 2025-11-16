/**
 * @file bcl_event.c
 * @brief BCL Event System - Core implementation
 * @version 2.0.0
 *
 * Sistema de eventos asíncrono similar a fileevent de TCL.
 * Soporta eventos de I/O (READABLE/WRITABLE) y timers.
 */

#include "bcl.h"
#include "bcl_commands.h"
#include <sys/select.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

/* ========================================================================== */
/* UTILIDADES DE TIEMPO                                                      */
/* ========================================================================== */

/**
 * @brief Obtiene el tiempo actual en milisegundos
 */
static uint64_t bcl_get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;
}

/* ========================================================================== */
/* CREACIÓN Y DESTRUCCIÓN DEL EVENT LOOP                                     */
/* ========================================================================== */

/**
 * @brief Crea un nuevo event loop
 */
bcl_event_loop_t *bcl_event_loop_create(void) {
    bcl_event_loop_t *loop = malloc(sizeof(bcl_event_loop_t));
    if (!loop) return NULL;

    loop->events = NULL;
    loop->running = false;
    loop->max_fd = -1;
    loop->event_count = 0;

    return loop;
}

/**
 * @brief Destruye el event loop y libera todos los eventos
 */
void bcl_event_loop_destroy(bcl_event_loop_t *loop) {
    if (!loop) return;

    /* Liberar todos los eventos */
    bcl_event_t *event = loop->events;
    while (event) {
        bcl_event_t *next = event->next;
        if (event->callback) free(event->callback);
        free(event);
        event = next;
    }

    free(loop);
}

/* ========================================================================== */
/* REGISTRO DE EVENTOS                                                       */
/* ========================================================================== */

/**
 * @brief Registra un evento de file descriptor
 */
int bcl_event_register_fd(bcl_interp_t *interp, int fd,
                         bcl_event_type_t types, const char *callback) {
    if (!interp || fd < 0 || !callback) return -1;

    /* Crear event loop si no existe */
    if (!interp->event_loop) {
        interp->event_loop = bcl_event_loop_create();
        if (!interp->event_loop) return -1;
    }

    /* Verificar si ya existe evento para este FD con estos tipos */
    bcl_event_t *event = interp->event_loop->events;
    while (event) {
        if (event->source == BCL_EVENT_SOURCE_FD &&
            event->fd_event.fd == fd &&
            (event->fd_event.types & types)) {
            /* Ya existe, actualizar callback */
            free(event->callback);
            event->callback = bcl_strdup(callback);
            event->fd_event.types |= types;  /* OR con nuevos tipos */
            return 0;
        }
        event = event->next;
    }

    /* Crear nuevo evento */
    event = malloc(sizeof(bcl_event_t));
    if (!event) return -1;

    event->source = BCL_EVENT_SOURCE_FD;
    event->fd_event.fd = fd;
    event->fd_event.types = types;
    event->callback = bcl_strdup(callback);
    event->next = interp->event_loop->events;

    interp->event_loop->events = event;
    interp->event_loop->event_count++;

    /* Actualizar max_fd */
    if (fd > interp->event_loop->max_fd) {
        interp->event_loop->max_fd = fd;
    }

    return 0;
}

/**
 * @brief Registra un timer
 */
int bcl_event_register_timer(bcl_interp_t *interp, uint32_t milliseconds,
                             const char *callback, bool repeat) {
    if (!interp || !callback) return -1;

    /* Crear event loop si no existe */
    if (!interp->event_loop) {
        interp->event_loop = bcl_event_loop_create();
        if (!interp->event_loop) return -1;
    }

    /* Crear nuevo timer */
    bcl_event_t *event = malloc(sizeof(bcl_event_t));
    if (!event) return -1;

    event->source = BCL_EVENT_SOURCE_TIMER;
    event->timer_event.expire_time_ms = bcl_get_time_ms() + milliseconds;
    event->timer_event.interval_ms = repeat ? milliseconds : 0;
    event->callback = bcl_strdup(callback);
    event->next = interp->event_loop->events;

    interp->event_loop->events = event;
    interp->event_loop->event_count++;

    return 0;
}

/**
 * @brief Elimina un evento de file descriptor
 */
int bcl_event_unregister_fd(bcl_interp_t *interp, int fd,
                           bcl_event_type_t types) {
    if (!interp || !interp->event_loop) return -1;

    bcl_event_t **ptr = &interp->event_loop->events;
    bool found = false;

    while (*ptr) {
        bcl_event_t *event = *ptr;

        if (event->source == BCL_EVENT_SOURCE_FD &&
            event->fd_event.fd == fd) {

            if (types == 0) {
                /* Eliminar todos los tipos para este FD */
                *ptr = event->next;
                free(event->callback);
                free(event);
                interp->event_loop->event_count--;
                found = true;
                continue;
            } else {
                /* Eliminar solo los tipos especificados */
                event->fd_event.types &= ~types;
                if (event->fd_event.types == 0) {
                    /* No quedan tipos, eliminar el evento */
                    *ptr = event->next;
                    free(event->callback);
                    free(event);
                    interp->event_loop->event_count--;
                    found = true;
                    continue;
                }
            }
        }

        ptr = &(*ptr)->next;
    }

    /* Recalcular max_fd si es necesario */
    if (found) {
        interp->event_loop->max_fd = -1;
        bcl_event_t *event = interp->event_loop->events;
        while (event) {
            if (event->source == BCL_EVENT_SOURCE_FD) {
                if (event->fd_event.fd > interp->event_loop->max_fd) {
                    interp->event_loop->max_fd = event->fd_event.fd;
                }
            }
            event = event->next;
        }
    }

    return found ? 0 : -1;
}

/* ========================================================================== */
/* EVENT LOOP                                                                */
/* ========================================================================== */

/**
 * @brief Procesa eventos una vez con timeout
 * @param timeout_ms Timeout en milisegundos (-1 = infinito, 0 = non-blocking)
 * @return BCL_OK si procesó eventos, BCL_ERROR si error, BCL_BREAK si no hay eventos
 */
bcl_result_t bcl_event_process(bcl_interp_t *interp, int timeout_ms) {
    if (!interp || !interp->event_loop) return BCL_ERROR;

    if (interp->event_loop->event_count == 0) {
        return BCL_BREAK;  /* No hay eventos */
    }

    /* Preparar sets de file descriptors para select() */
    fd_set readfds, writefds, exceptfds;
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_ZERO(&exceptfds);

    int nfds = 0;
    uint64_t now = bcl_get_time_ms();
    uint64_t next_timer = UINT64_MAX;

    /* Registrar FDs y encontrar próximo timer */
    bcl_event_t *event = interp->event_loop->events;
    while (event) {
        if (event->source == BCL_EVENT_SOURCE_FD) {
            int fd = event->fd_event.fd;
            if (event->fd_event.types & BCL_EVENT_READABLE) {
                FD_SET(fd, &readfds);
            }
            if (event->fd_event.types & BCL_EVENT_WRITABLE) {
                FD_SET(fd, &writefds);
            }
            if (event->fd_event.types & BCL_EVENT_EXCEPTION) {
                FD_SET(fd, &exceptfds);
            }
            if (fd >= nfds) nfds = fd + 1;
        } else if (event->source == BCL_EVENT_SOURCE_TIMER) {
            if (event->timer_event.expire_time_ms < next_timer) {
                next_timer = event->timer_event.expire_time_ms;
            }
        }
        event = event->next;
    }

    /* Calcular timeout para select() */
    struct timeval tv, *tvp = NULL;

    if (next_timer != UINT64_MAX) {
        /* Hay timers, calcular tiempo hasta próximo timer */
        if (next_timer <= now) {
            /* Timer ya expiró, no bloquear */
            tv.tv_sec = 0;
            tv.tv_usec = 0;
            tvp = &tv;
        } else {
            uint64_t diff_ms = next_timer - now;
            if (timeout_ms >= 0 && (uint64_t)timeout_ms < diff_ms) {
                /* Timeout del usuario es menor */
                diff_ms = timeout_ms;
            }
            tv.tv_sec = diff_ms / 1000;
            tv.tv_usec = (diff_ms % 1000) * 1000;
            tvp = &tv;
        }
    } else if (timeout_ms >= 0) {
        /* No hay timers, usar timeout del usuario */
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        tvp = &tv;
    }
    /* Si timeout_ms < 0 y no hay timers, tvp = NULL (bloquear indefinidamente) */

    /* Llamar a select() */
    int ready = select(nfds, &readfds, &writefds, &exceptfds, tvp);

    if (ready < 0) {
        if (errno == EINTR) {
            return BCL_OK;  /* Interrumpido por señal, no es error */
        }
        bcl_set_error(interp, "EVENT: select() failed: %s", strerror(errno));
        return BCL_ERROR;
    }

    /* Procesar eventos de I/O */
    if (ready > 0) {
        event = interp->event_loop->events;
        while (event) {
            if (event->source == BCL_EVENT_SOURCE_FD) {
                int fd = event->fd_event.fd;
                bool triggered = false;

                if ((event->fd_event.types & BCL_EVENT_READABLE) && FD_ISSET(fd, &readfds)) {
                    triggered = true;
                }
                if ((event->fd_event.types & BCL_EVENT_WRITABLE) && FD_ISSET(fd, &writefds)) {
                    triggered = true;
                }
                if ((event->fd_event.types & BCL_EVENT_EXCEPTION) && FD_ISSET(fd, &exceptfds)) {
                    triggered = true;
                }

                if (triggered) {
                    /* Llamar al callback con el FD como parámetro */
                    char fd_str[32];
                    snprintf(fd_str, sizeof(fd_str), "%d", fd);

                    char *argv[] = { (char*)fd_str };
                    bcl_value_t *callback_result = NULL;

                    bcl_result_t res = bcl_dispatch_command(interp, event->callback,
                                                           1, argv, &callback_result);

                    if (callback_result) {
                        bcl_value_destroy(callback_result);
                    }

                    if (res != BCL_OK && res != BCL_RETURN) {
                        /* Error en callback */
                        return res;
                    }
                }
            }
            event = event->next;
        }
    }

    /* Procesar timers expirados */
    now = bcl_get_time_ms();
    bcl_event_t **ptr = &interp->event_loop->events;

    while (*ptr) {
        event = *ptr;

        if (event->source == BCL_EVENT_SOURCE_TIMER &&
            event->timer_event.expire_time_ms <= now) {

            /* Timer expirado, llamar callback */
            bcl_value_t *callback_result = NULL;
            bcl_result_t res = bcl_dispatch_command(interp, event->callback,
                                                   0, NULL, &callback_result);

            if (callback_result) {
                bcl_value_destroy(callback_result);
            }

            if (res != BCL_OK && res != BCL_RETURN) {
                return res;
            }

            /* Si es repetitivo, reprogramar */
            if (event->timer_event.interval_ms > 0) {
                event->timer_event.expire_time_ms = now + event->timer_event.interval_ms;
                ptr = &event->next;
            } else {
                /* One-shot, eliminar */
                *ptr = event->next;
                free(event->callback);
                free(event);
                interp->event_loop->event_count--;
            }
        } else {
            ptr = &event->next;
        }
    }

    return BCL_OK;
}

/**
 * @brief Event loop infinito
 * @return BCL_OK si salió normalmente, BCL_EXIT si EXIT, BCL_ERROR si error
 */
bcl_result_t bcl_event_loop_run(bcl_interp_t *interp) {
    if (!interp || !interp->event_loop) {
        bcl_set_error(interp, "EVENT LOOP: no event loop initialized");
        return BCL_ERROR;
    }

    interp->event_loop->running = true;

    while (interp->event_loop->running) {
        bcl_result_t res = bcl_event_process(interp, -1);  /* Bloquear indefinidamente */

        if (res == BCL_BREAK) {
            /* No hay más eventos */
            break;
        } else if (res == BCL_ERROR) {
            interp->event_loop->running = false;
            return BCL_ERROR;
        } else if (res == BCL_EXIT) {
            interp->event_loop->running = false;
            return BCL_EXIT;
        }
    }

    return BCL_OK;
}

/**
 * @brief Detiene el event loop
 */
void bcl_event_loop_stop(bcl_interp_t *interp) {
    if (interp && interp->event_loop) {
        interp->event_loop->running = false;
    }
}

/* ========================================================================== */
/* COMANDO EVENT                                                             */
/* ========================================================================== */

/**
 * @brief Convierte string de tipo a bcl_event_type_t
 */
static bcl_event_type_t parse_event_type(const char *type_str) {
    if (bcl_strcasecmp(type_str, "READABLE") == 0) {
        return BCL_EVENT_READABLE;
    } else if (bcl_strcasecmp(type_str, "WRITABLE") == 0) {
        return BCL_EVENT_WRITABLE;
    } else if (bcl_strcasecmp(type_str, "EXCEPTION") == 0) {
        return BCL_EVENT_EXCEPTION;
    }
    return 0;
}

/**
 * @brief Convierte handle string a file descriptor
 * Soporta: número directo, "stdin", "stdout", "stderr", "sock<N>"
 */
static int parse_handle(const char *handle_str) {
    if (bcl_strcasecmp(handle_str, "stdin") == 0) {
        return STDIN_FILENO;
    } else if (bcl_strcasecmp(handle_str, "stdout") == 0) {
        return STDOUT_FILENO;
    } else if (bcl_strcasecmp(handle_str, "stderr") == 0) {
        return STDERR_FILENO;
    } else if (strncasecmp(handle_str, "sock", 4) == 0) {
        /* Socket handle desde extensión SOCKET */
        int sock_id;
        if (sscanf(handle_str, "sock%d", &sock_id) == 1) {
            /* Aquí deberíamos consultar la extensión SOCKET para obtener el FD real
             * Por ahora, simplemente retornamos un FD inválido para que la extensión
             * maneje esto */
            return -1;  /* TODO: Integrar con extensión SOCKET */
        }
    }
    /* Intentar parsear como número directo */
    return atoi(handle_str);
}

/**
 * EVENT CREATE handle type callback
 * Registra un evento de I/O
 */
static bcl_result_t event_create(bcl_interp_t *interp, int argc, char **argv,
                                bcl_value_t **result) {
    if (argc != 3) {
        bcl_set_error(interp, "EVENT CREATE: wrong # args: should be \"EVENT CREATE handle type callback\"");
        return BCL_ERROR;
    }

    int fd = parse_handle(argv[0]);
    if (fd < 0) {
        bcl_set_error(interp, "EVENT CREATE: invalid handle \"%s\"", argv[0]);
        return BCL_ERROR;
    }

    bcl_event_type_t type = parse_event_type(argv[1]);
    if (type == 0) {
        bcl_set_error(interp, "EVENT CREATE: invalid type \"%s\": must be READABLE, WRITABLE, or EXCEPTION", argv[1]);
        return BCL_ERROR;
    }

    const char *callback = argv[2];

    /* Verificar que el callback existe */
    if (!bcl_proc_exists(interp, callback)) {
        bcl_set_error(interp, "EVENT CREATE: procedure \"%s\" not found", callback);
        return BCL_ERROR;
    }

    /* Registrar evento */
    if (bcl_event_register_fd(interp, fd, type, callback) != 0) {
        bcl_set_error(interp, "EVENT CREATE: failed to register event");
        return BCL_ERROR;
    }

    *result = bcl_value_create("");
    return BCL_OK;
}

/**
 * EVENT DELETE handle [type]
 * Elimina un evento de I/O
 */
static bcl_result_t event_delete(bcl_interp_t *interp, int argc, char **argv,
                                bcl_value_t **result) {
    if (argc < 1 || argc > 2) {
        bcl_set_error(interp, "EVENT DELETE: wrong # args: should be \"EVENT DELETE handle ?type?\"");
        return BCL_ERROR;
    }

    int fd = parse_handle(argv[0]);
    if (fd < 0) {
        bcl_set_error(interp, "EVENT DELETE: invalid handle \"%s\"", argv[0]);
        return BCL_ERROR;
    }

    bcl_event_type_t type = 0;
    if (argc == 2) {
        type = parse_event_type(argv[1]);
        if (type == 0) {
            bcl_set_error(interp, "EVENT DELETE: invalid type \"%s\"", argv[1]);
            return BCL_ERROR;
        }
    }

    /* Eliminar evento */
    if (bcl_event_unregister_fd(interp, fd, type) != 0) {
        bcl_set_error(interp, "EVENT DELETE: no event found for handle %d", fd);
        return BCL_ERROR;
    }

    *result = bcl_value_create("");
    return BCL_OK;
}

/**
 * EVENT TIMER milliseconds callback
 * Registra un timer one-shot
 */
static bcl_result_t event_timer(bcl_interp_t *interp, int argc, char **argv,
                               bcl_value_t **result) {
    if (argc != 2) {
        bcl_set_error(interp, "EVENT TIMER: wrong # args: should be \"EVENT TIMER milliseconds callback\"");
        return BCL_ERROR;
    }

    int milliseconds = atoi(argv[0]);
    if (milliseconds < 0) {
        bcl_set_error(interp, "EVENT TIMER: invalid milliseconds \"%s\"", argv[0]);
        return BCL_ERROR;
    }

    const char *callback = argv[1];

    /* Verificar que el callback existe */
    if (!bcl_proc_exists(interp, callback)) {
        bcl_set_error(interp, "EVENT TIMER: procedure \"%s\" not found", callback);
        return BCL_ERROR;
    }

    /* Registrar timer */
    if (bcl_event_register_timer(interp, milliseconds, callback, false) != 0) {
        bcl_set_error(interp, "EVENT TIMER: failed to register timer");
        return BCL_ERROR;
    }

    *result = bcl_value_create("");
    return BCL_OK;
}

/**
 * EVENT PROCESS [timeout]
 * Procesa eventos una vez
 */
static bcl_result_t event_process(bcl_interp_t *interp, int argc, char **argv,
                                 bcl_value_t **result) {
    if (argc > 1) {
        bcl_set_error(interp, "EVENT PROCESS: wrong # args: should be \"EVENT PROCESS ?timeout?\"");
        return BCL_ERROR;
    }

    int timeout_ms = -1;  /* Por defecto, bloquear indefinidamente */
    if (argc == 1) {
        timeout_ms = atoi(argv[0]);
    }

    bcl_result_t res = bcl_event_process(interp, timeout_ms);

    if (res == BCL_BREAK) {
        /* No hay eventos */
        *result = bcl_value_create("0");
    } else if (res == BCL_OK) {
        *result = bcl_value_create("1");
    } else {
        return res;  /* Error */
    }

    return BCL_OK;
}

/**
 * EVENT LOOP
 * Event loop infinito
 */
static bcl_result_t event_loop(bcl_interp_t *interp, int argc, char **argv,
                              bcl_value_t **result) {
    BCL_UNUSED(argc);
    BCL_UNUSED(argv);

    bcl_result_t res = bcl_event_loop_run(interp);

    *result = bcl_value_create("");
    return res;
}

/**
 * EVENT INFO
 * Lista eventos registrados
 */
static bcl_result_t event_info(bcl_interp_t *interp, int argc, char **argv,
                              bcl_value_t **result) {
    BCL_UNUSED(argc);
    BCL_UNUSED(argv);

    if (!interp->event_loop) {
        *result = bcl_value_create("");
        return BCL_OK;
    }

    bcl_string_t *output = bcl_string_create("");
    bcl_event_t *event = interp->event_loop->events;

    while (event) {
        if (event->source == BCL_EVENT_SOURCE_FD) {
            char buf[256];
            snprintf(buf, sizeof(buf), "FD %d (%s%s%s) -> %s\n",
                    event->fd_event.fd,
                    (event->fd_event.types & BCL_EVENT_READABLE) ? "R" : "",
                    (event->fd_event.types & BCL_EVENT_WRITABLE) ? "W" : "",
                    (event->fd_event.types & BCL_EVENT_EXCEPTION) ? "E" : "",
                    event->callback);
            bcl_string_append(output, buf);
        } else if (event->source == BCL_EVENT_SOURCE_TIMER) {
            char buf[256];
            uint64_t now = bcl_get_time_ms();
            int64_t remaining = (int64_t)event->timer_event.expire_time_ms - (int64_t)now;
            snprintf(buf, sizeof(buf), "TIMER in %ldms%s -> %s\n",
                    (long)remaining,
                    event->timer_event.interval_ms > 0 ? " (repeat)" : "",
                    event->callback);
            bcl_string_append(output, buf);
        }
        event = event->next;
    }

    *result = bcl_value_create(bcl_string_cstr(output));
    bcl_string_destroy(output);
    return BCL_OK;
}

/**
 * EVENT - Comando principal
 *
 * Subcomandos:
 *   EVENT CREATE handle type callback
 *   EVENT DELETE handle [type]
 *   EVENT TIMER milliseconds callback
 *   EVENT PROCESS [timeout]
 *   EVENT LOOP
 *   EVENT INFO
 */
bcl_result_t bcl_cmd_event(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result) {
    if (argc < 1) {
        bcl_set_error(interp, "EVENT: wrong # args: should be \"EVENT subcommand ?args?\"");
        return BCL_ERROR;
    }

    const char *subcmd = argv[0];

    if (bcl_strcasecmp(subcmd, "CREATE") == 0) {
        return event_create(interp, argc - 1, argv + 1, result);
    } else if (bcl_strcasecmp(subcmd, "DELETE") == 0) {
        return event_delete(interp, argc - 1, argv + 1, result);
    } else if (bcl_strcasecmp(subcmd, "TIMER") == 0) {
        return event_timer(interp, argc - 1, argv + 1, result);
    } else if (bcl_strcasecmp(subcmd, "PROCESS") == 0) {
        return event_process(interp, argc - 1, argv + 1, result);
    } else if (bcl_strcasecmp(subcmd, "LOOP") == 0) {
        return event_loop(interp, argc - 1, argv + 1, result);
    } else if (bcl_strcasecmp(subcmd, "INFO") == 0) {
        return event_info(interp, argc - 1, argv + 1, result);
    } else {
        bcl_set_error(interp, "EVENT: unknown subcommand \"%s\": must be CREATE, DELETE, TIMER, PROCESS, LOOP, or INFO", subcmd);
        return BCL_ERROR;
    }
}
