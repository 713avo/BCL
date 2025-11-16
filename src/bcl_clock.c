/**
 * @file bcl_clock.c
 * @brief Implementación del comando CLOCK
 * @version 1.0
 *
 * Subcomandos implementados:
 * - CLOCK SECONDS: Tiempo actual en segundos desde epoch
 * - CLOCK MILLISECONDS: Tiempo actual en milisegundos
 * - CLOCK MICROSECONDS: Tiempo actual en microsegundos
 * - CLOCK FORMAT: Formatear timestamp a string
 * - CLOCK SCAN: Parsear string a timestamp
 * - CLOCK ADD: Sumar intervalos de tiempo
 *
 * Nota: Versión básica sin soporte completo de timezone/locale/DST
 */

#include "bcl.h"
#include "bcl_commands.h"
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* ========================================================================== */
/* CLOCK SECONDS - Tiempo actual en segundos                                 */
/* ========================================================================== */

static bcl_result_t clock_seconds(bcl_interp_t *interp, bcl_value_t **result) {
    time_t now = time(NULL);
    char buf[64];
    snprintf(buf, sizeof(buf), "%ld", (long)now);
    *result = bcl_value_create(buf);
    return BCL_OK;
}

/* ========================================================================== */
/* CLOCK MILLISECONDS - Tiempo actual en milisegundos                        */
/* ========================================================================== */

static bcl_result_t clock_milliseconds(bcl_interp_t *interp, bcl_value_t **result) {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    long long ms = (long long)tv.tv_sec * 1000LL + tv.tv_usec / 1000;

    char buf[64];
    snprintf(buf, sizeof(buf), "%lld", ms);
    *result = bcl_value_create(buf);
    return BCL_OK;
}

/* ========================================================================== */
/* CLOCK MICROSECONDS - Tiempo actual en microsegundos                       */
/* ========================================================================== */

static bcl_result_t clock_microseconds(bcl_interp_t *interp, bcl_value_t **result) {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    long long us = (long long)tv.tv_sec * 1000000LL + tv.tv_usec;

    char buf[64];
    snprintf(buf, sizeof(buf), "%lld", us);
    *result = bcl_value_create(buf);
    return BCL_OK;
}

/* ========================================================================== */
/* CLOCK FORMAT - Formatear timestamp                                        */
/* ========================================================================== */

static bcl_result_t clock_format(bcl_interp_t *interp, int argc, char **argv,
                                  bcl_value_t **result) {
    if (argc < 1) {
        bcl_set_error(interp, "CLOCK FORMAT: wrong # args");
        return BCL_ERROR;
    }

    /* Parsear timestamp */
    bool ok;
    double timestamp_d = bcl_str_to_number(argv[0], &ok);
    if (!ok) {
        bcl_set_error(interp, "CLOCK FORMAT: invalid timestamp \"%s\"", argv[0]);
        return BCL_ERROR;
    }
    time_t timestamp = (time_t)timestamp_d;

    /* Buscar parámetro FORMAT */
    const char *format = "%a %b %d %H:%M:%S %Z %Y";  /* Default Tcl-style */
    bool use_gmt = false;
    bool format_specified = false;

    /* Si hay un segundo argumento y NO es una palabra clave, asumirlo como formato */
    if (argc >= 2 && argv[1][0] == '%') {
        format = argv[1];
        format_specified = true;
    }

    for (int i = (format_specified ? 2 : 1); i < argc; i++) {
        if (bcl_strcasecmp(argv[i], "FORMAT") == 0 && i + 1 < argc) {
            format = argv[i + 1];
            i++;
        } else if (bcl_strcasecmp(argv[i], "GMT") == 0) {
            use_gmt = true;
        }
        /* TIMEZONE y LOCALE se ignoran en esta implementación básica */
    }

    /* Convertir a struct tm */
    struct tm *tm_info;
    if (use_gmt) {
        tm_info = gmtime(&timestamp);
    } else {
        tm_info = localtime(&timestamp);
    }

    if (!tm_info) {
        bcl_set_error(interp, "CLOCK FORMAT: invalid timestamp");
        return BCL_ERROR;
    }

    /* Formatear */
    char buffer[1024];
    size_t len = strftime(buffer, sizeof(buffer), format, tm_info);

    if (len == 0) {
        bcl_set_error(interp, "CLOCK FORMAT: format error");
        return BCL_ERROR;
    }

    *result = bcl_value_create(buffer);
    return BCL_OK;
}

/* ========================================================================== */
/* CLOCK SCAN - Parsear string a timestamp                                   */
/* ========================================================================== */

static bcl_result_t clock_scan(bcl_interp_t *interp, int argc, char **argv,
                                bcl_value_t **result) {
    if (argc < 1) {
        bcl_set_error(interp, "CLOCK SCAN: wrong # args");
        return BCL_ERROR;
    }

    const char *text = argv[0];
    const char *format = NULL;
    bool use_gmt = false;
    time_t base_time = time(NULL);

    /* Parsear opciones */
    for (int i = 1; i < argc; i++) {
        if (bcl_strcasecmp(argv[i], "FORMAT") == 0 && i + 1 < argc) {
            format = argv[i + 1];
            i++;
        } else if (bcl_strcasecmp(argv[i], "GMT") == 0) {
            use_gmt = true;
        } else if (bcl_strcasecmp(argv[i], "BASE") == 0 && i + 1 < argc) {
            bool ok;
            double base_d = bcl_str_to_number(argv[i + 1], &ok);
            if (ok) {
                base_time = (time_t)base_d;
            }
            i++;
        }
    }

    struct tm tm_info;
    memset(&tm_info, 0, sizeof(tm_info));

    if (format) {
        /* Parsear con formato específico */
        char *ret = strptime(text, format, &tm_info);
        if (!ret) {
            bcl_set_error(interp, "CLOCK SCAN: unable to parse \"%s\" with format \"%s\"",
                         text, format);
            return BCL_ERROR;
        }

        /* strptime no establece tm_isdst, lo ponemos a -1 para que mktime lo determine */
        tm_info.tm_isdst = -1;
    } else {
        /* Intentar formatos comunes */
        /* Formato ISO: YYYY-MM-DD HH:MM:SS */
        if (sscanf(text, "%d-%d-%d %d:%d:%d",
                   &tm_info.tm_year, &tm_info.tm_mon, &tm_info.tm_mday,
                   &tm_info.tm_hour, &tm_info.tm_min, &tm_info.tm_sec) == 6) {
            tm_info.tm_year -= 1900;
            tm_info.tm_mon -= 1;
            tm_info.tm_isdst = -1;
        }
        /* Formato ISO fecha: YYYY-MM-DD */
        else if (sscanf(text, "%d-%d-%d",
                        &tm_info.tm_year, &tm_info.tm_mon, &tm_info.tm_mday) == 3) {
            tm_info.tm_year -= 1900;
            tm_info.tm_mon -= 1;
            tm_info.tm_hour = 0;
            tm_info.tm_min = 0;
            tm_info.tm_sec = 0;
            tm_info.tm_isdst = -1;
        }
        /* Palabra clave "now" */
        else if (bcl_strcasecmp(text, "now") == 0) {
            char buf[64];
            snprintf(buf, sizeof(buf), "%ld", (long)base_time);
            *result = bcl_value_create(buf);
            return BCL_OK;
        }
        else {
            bcl_set_error(interp, "CLOCK SCAN: unable to parse \"%s\" (use FORMAT option)", text);
            return BCL_ERROR;
        }
    }

    /* Convertir a timestamp */
    time_t timestamp;
    if (use_gmt) {
        /* timegm no está en POSIX estándar, usamos mktime con ajuste */
        #ifdef _WIN32
        timestamp = _mkgmtime(&tm_info);
        #else
        /* Guardar TZ, poner UTC, usar mktime, restaurar */
        char *old_tz = getenv("TZ");
        setenv("TZ", "UTC", 1);
        tzset();
        timestamp = mktime(&tm_info);
        if (old_tz) {
            setenv("TZ", old_tz, 1);
        } else {
            unsetenv("TZ");
        }
        tzset();
        #endif
    } else {
        timestamp = mktime(&tm_info);
    }

    if (timestamp == -1) {
        bcl_set_error(interp, "CLOCK SCAN: invalid date/time");
        return BCL_ERROR;
    }

    char buf[64];
    snprintf(buf, sizeof(buf), "%ld", (long)timestamp);
    *result = bcl_value_create(buf);
    return BCL_OK;
}

/* ========================================================================== */
/* CLOCK ADD - Sumar intervalos                                              */
/* ========================================================================== */

static bcl_result_t clock_add(bcl_interp_t *interp, int argc, char **argv,
                               bcl_value_t **result) {
    if (argc < 3 || argc % 2 == 0) {
        bcl_set_error(interp, "CLOCK ADD: wrong # args: should be \"CLOCK ADD timestamp value unit ?value unit ...? ?options?\"");
        return BCL_ERROR;
    }

    /* Parsear timestamp inicial */
    bool ok;
    double timestamp_d = bcl_str_to_number(argv[0], &ok);
    if (!ok) {
        bcl_set_error(interp, "CLOCK ADD: invalid timestamp \"%s\"", argv[0]);
        return BCL_ERROR;
    }
    time_t timestamp = (time_t)timestamp_d;

    bool use_gmt = false;

    /* Convertir a struct tm */
    struct tm tm_info;
    struct tm *tm_ptr;

    if (use_gmt) {
        tm_ptr = gmtime(&timestamp);
    } else {
        tm_ptr = localtime(&timestamp);
    }

    if (!tm_ptr) {
        bcl_set_error(interp, "CLOCK ADD: invalid timestamp");
        return BCL_ERROR;
    }

    tm_info = *tm_ptr;

    /* Procesar pares cantidad/unidad */
    int i = 1;
    while (i < argc) {
        /* Verificar si es una opción */
        if (bcl_strcasecmp(argv[i], "GMT") == 0) {
            use_gmt = true;
            i++;
            continue;
        }
        if (bcl_strcasecmp(argv[i], "TIMEZONE") == 0) {
            i += 2;  /* Ignorar en esta implementación básica */
            continue;
        }

        /* Debe ser cantidad unidad */
        if (i + 1 >= argc) break;

        double cantidad_d = bcl_str_to_number(argv[i], &ok);
        if (!ok) {
            bcl_set_error(interp, "CLOCK ADD: invalid quantity \"%s\"", argv[i]);
            return BCL_ERROR;
        }
        int cantidad = (int)cantidad_d;
        const char *unidad = argv[i + 1];

        /* Aplicar según unidad */
        if (bcl_strcasecmp(unidad, "seconds") == 0 || bcl_strcasecmp(unidad, "second") == 0) {
            tm_info.tm_sec += cantidad;
        }
        else if (bcl_strcasecmp(unidad, "minutes") == 0 || bcl_strcasecmp(unidad, "minute") == 0) {
            tm_info.tm_min += cantidad;
        }
        else if (bcl_strcasecmp(unidad, "hours") == 0 || bcl_strcasecmp(unidad, "hour") == 0) {
            tm_info.tm_hour += cantidad;
        }
        else if (bcl_strcasecmp(unidad, "days") == 0 || bcl_strcasecmp(unidad, "day") == 0) {
            tm_info.tm_mday += cantidad;
        }
        else if (bcl_strcasecmp(unidad, "weeks") == 0 || bcl_strcasecmp(unidad, "week") == 0) {
            tm_info.tm_mday += cantidad * 7;
        }
        else if (bcl_strcasecmp(unidad, "months") == 0 || bcl_strcasecmp(unidad, "month") == 0) {
            tm_info.tm_mon += cantidad;
        }
        else if (bcl_strcasecmp(unidad, "years") == 0 || bcl_strcasecmp(unidad, "year") == 0) {
            tm_info.tm_year += cantidad;
        }
        else {
            bcl_set_error(interp, "CLOCK ADD: unknown unit \"%s\"", unidad);
            return BCL_ERROR;
        }

        i += 2;
    }

    /* Normalizar y convertir de vuelta a timestamp */
    tm_info.tm_isdst = -1;
    time_t new_timestamp = mktime(&tm_info);

    if (new_timestamp == -1) {
        bcl_set_error(interp, "CLOCK ADD: result out of range");
        return BCL_ERROR;
    }

    char buf[64];
    snprintf(buf, sizeof(buf), "%ld", (long)new_timestamp);
    *result = bcl_value_create(buf);
    return BCL_OK;
}

/* ========================================================================== */
/* CLOCK - Comando principal                                                 */
/* ========================================================================== */

bcl_result_t bcl_cmd_clock(bcl_interp_t *interp, int argc, char **argv,
                           bcl_value_t **result) {
    if (argc < 1) {
        bcl_set_error(interp, "CLOCK: wrong # args: should be \"CLOCK subcommand ?args?\"");
        return BCL_ERROR;
    }

    const char *subcmd = argv[0];

    if (bcl_strcasecmp(subcmd, "SECONDS") == 0) {
        return clock_seconds(interp, result);
    }
    else if (bcl_strcasecmp(subcmd, "MILLISECONDS") == 0) {
        return clock_milliseconds(interp, result);
    }
    else if (bcl_strcasecmp(subcmd, "MICROSECONDS") == 0) {
        return clock_microseconds(interp, result);
    }
    else if (bcl_strcasecmp(subcmd, "FORMAT") == 0) {
        return clock_format(interp, argc - 1, argv + 1, result);
    }
    else if (bcl_strcasecmp(subcmd, "SCAN") == 0) {
        return clock_scan(interp, argc - 1, argv + 1, result);
    }
    else if (bcl_strcasecmp(subcmd, "ADD") == 0) {
        return clock_add(interp, argc - 1, argv + 1, result);
    }
    else {
        bcl_set_error(interp, "CLOCK: unknown subcommand \"%s\": should be SECONDS, MILLISECONDS, MICROSECONDS, FORMAT, SCAN, or ADD", subcmd);
        return BCL_ERROR;
    }
}
