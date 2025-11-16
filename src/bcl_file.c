/**
 * @file bcl_file.c
 * @brief Implementación de I/O de archivos con sistema de handles
 * @version 1.0
 *
 * Sistema de handles tipo file0, file1, etc.
 * Comandos: OPEN, CLOSE, READ, GETS, PUTS, PUTSN, TELL, SEEK, EOF
 */

#include "bcl.h"
#include "bcl_commands.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#ifndef BCL_NO_FILES

/* ========================================================================== */
/* UTILIDADES INTERNAS                                                       */
/* ========================================================================== */

/**
 * @brief Obtiene un handle de archivo desde la hash table
 */
static bcl_file_handle_t *get_file_handle(bcl_interp_t *interp, const char *handle_name) {
    if (!interp->file_handles) {
        return NULL;
    }

    bcl_value_t *val = bcl_hash_get(interp->file_handles, handle_name);
    if (!val) {
        return NULL;
    }

    /* El valor almacenado es un puntero codificado como string */
    bcl_file_handle_t *handle;
    sscanf(bcl_value_get(val), "%p", (void**)&handle);
    return handle;
}

/**
 * @brief Genera el nombre del próximo handle
 */
static char *generate_handle_name(bcl_interp_t *interp) {
    char *name = malloc(32);
    if (!name) return NULL;

    snprintf(name, 32, "file%zu", interp->next_handle_id++);
    return name;
}

/**
 * @brief Registra un handle en la tabla
 */
static void register_handle(bcl_interp_t *interp, const char *name, bcl_file_handle_t *handle) {
    /* Codificar puntero como string */
    char ptr_str[32];
    snprintf(ptr_str, sizeof(ptr_str), "%p", (void*)handle);

    bcl_hash_set(interp->file_handles, name, bcl_value_create(ptr_str));
}

/**
 * @brief Convierte string de modo a enum
 */
static bcl_file_mode_t parse_mode(const char *mode_str, const char **fopen_mode) {
    if (bcl_strcasecmp(mode_str, "R") == 0) {
        *fopen_mode = "r";
        return BCL_FILE_READ;
    } else if (bcl_strcasecmp(mode_str, "W") == 0) {
        *fopen_mode = "w";
        return BCL_FILE_WRITE;
    } else if (bcl_strcasecmp(mode_str, "A") == 0) {
        *fopen_mode = "a";
        return BCL_FILE_APPEND;
    } else if (bcl_strcasecmp(mode_str, "RW") == 0) {
        *fopen_mode = "r+";
        return BCL_FILE_READ_WRITE;
    }

    *fopen_mode = NULL;
    return BCL_FILE_READ; /* Default */
}

/* ========================================================================== */
/* OPEN - Abrir archivo                                                      */
/* ========================================================================== */

bcl_result_t bcl_cmd_open(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result) {
    if (argc < 1 || argc > 2) {
        bcl_set_error(interp, "wrong # args: should be \"OPEN path ?mode?\"");
        return BCL_ERROR;
    }

    const char *path = argv[0];
    const char *mode_str = (argc == 2) ? argv[1] : "R";

    /* Parsear modo */
    const char *fopen_mode;
    bcl_file_mode_t mode = parse_mode(mode_str, &fopen_mode);

    if (!fopen_mode) {
        bcl_set_error(interp, "invalid mode \"%s\": should be R, W, A, or RW", mode_str);
        return BCL_ERROR;
    }

    /* Abrir archivo */
    FILE *fp = fopen(path, fopen_mode);
    if (!fp) {
        bcl_set_error(interp, "couldn't open \"%s\": %s", path, strerror(errno));
        return BCL_ERROR;
    }

    /* Crear handle */
    bcl_file_handle_t *handle = malloc(sizeof(bcl_file_handle_t));
    if (!handle) {
        fclose(fp);
        bcl_set_error(interp, "out of memory");
        return BCL_ERROR;
    }

    handle->fp = fp;
    handle->mode = mode;
    handle->path = bcl_strdup(path);
    handle->eof_reached = false;

    /* Generar nombre de handle */
    char *handle_name = generate_handle_name(interp);
    if (!handle_name) {
        fclose(fp);
        free(handle->path);
        free(handle);
        bcl_set_error(interp, "out of memory");
        return BCL_ERROR;
    }

    /* Registrar handle */
    register_handle(interp, handle_name, handle);

    /* Retornar nombre del handle */
    if (result) {
        *result = bcl_value_create(handle_name);
    }

    free(handle_name);
    return BCL_OK;
}

/* ========================================================================== */
/* CLOSE - Cerrar archivo                                                    */
/* ========================================================================== */

bcl_result_t bcl_cmd_close(bcl_interp_t *interp, int argc, char **argv,
                           bcl_value_t **result) {
    if (argc != 1) {
        bcl_set_error(interp, "wrong # args: should be \"CLOSE handle\"");
        return BCL_ERROR;
    }

    const char *handle_name = argv[0];

    /* Obtener handle */
    bcl_file_handle_t *handle = get_file_handle(interp, handle_name);
    if (!handle) {
        bcl_set_error(interp, "invalid file handle \"%s\"", handle_name);
        return BCL_ERROR;
    }

    /* Cerrar archivo */
    if (handle->fp) {
        fclose(handle->fp);
    }

    /* Liberar memoria */
    free(handle->path);
    free(handle);

    /* Eliminar de la tabla */
    bcl_hash_remove(interp->file_handles, handle_name);

    if (result) {
        *result = bcl_value_create("");
    }

    return BCL_OK;
}

/* ========================================================================== */
/* READ - Leer desde archivo                                                 */
/* ========================================================================== */

bcl_result_t bcl_cmd_read(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result) {
    if (argc < 1 || argc > 2) {
        bcl_set_error(interp, "wrong # args: should be \"READ handle ?numBytes?\"");
        return BCL_ERROR;
    }

    const char *handle_name = argv[0];

    /* Obtener handle */
    bcl_file_handle_t *handle = get_file_handle(interp, handle_name);
    if (!handle) {
        bcl_set_error(interp, "invalid file handle \"%s\"", handle_name);
        return BCL_ERROR;
    }

    /* Determinar cuántos bytes leer */
    size_t num_bytes;
    bool read_all = false;

    if (argc == 2) {
        bool ok;
        double n = bcl_str_to_number(argv[1], &ok);
        if (!ok || n < 0) {
            bcl_set_error(interp, "expected non-negative integer but got \"%s\"", argv[1]);
            return BCL_ERROR;
        }
        num_bytes = (size_t)n;
    } else {
        /* Sin argumento: leer todo el archivo */
        read_all = true;

        /* Obtener tamaño del archivo */
        long current = ftell(handle->fp);
        fseek(handle->fp, 0, SEEK_END);
        long size = ftell(handle->fp);
        fseek(handle->fp, current, SEEK_SET);

        num_bytes = (size - current);
    }

    /* Leer datos */
    char *buffer = malloc(num_bytes + 1);
    if (!buffer) {
        bcl_set_error(interp, "out of memory");
        return BCL_ERROR;
    }

    size_t bytes_read = fread(buffer, 1, num_bytes, handle->fp);
    buffer[bytes_read] = '\0';

    /* Verificar EOF */
    if (feof(handle->fp)) {
        handle->eof_reached = true;
    }

    /* Retornar datos leídos */
    if (result) {
        *result = bcl_value_create(buffer);
    }

    free(buffer);
    return BCL_OK;
}

/* ========================================================================== */
/* HELPERS PARA GETS/PUTS/PUTSN (usados desde bcl_commands.c)               */
/* ========================================================================== */

/**
 * @brief Lee línea desde un handle de archivo
 * @return BCL_OK si éxito, BCL_ERROR si error
 */
bcl_result_t bcl_file_gets(bcl_interp_t *interp, const char *handle_name,
                           bcl_value_t **result) {
    /* Obtener handle */
    bcl_file_handle_t *handle = get_file_handle(interp, handle_name);
    if (!handle) {
        bcl_set_error(interp, "invalid file handle \"%s\"", handle_name);
        return BCL_ERROR;
    }

    /* Leer línea */
    char line[BCL_MAX_LINE_LEN];
    if (fgets(line, sizeof(line), handle->fp)) {
        /* Eliminar newline final */
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        if (result) {
            *result = bcl_value_create(line);
        }
        return BCL_OK;
    } else {
        /* EOF o error */
        if (feof(handle->fp)) {
            handle->eof_reached = true;
        }

        if (result) {
            *result = bcl_value_create("");
        }
        return BCL_OK;
    }
}

/**
 * @brief Escribe a un handle de archivo con newline
 */
bcl_result_t bcl_file_puts(bcl_interp_t *interp, const char *handle_name,
                           const char *text, bcl_value_t **result) {
    /* Obtener handle */
    bcl_file_handle_t *handle = get_file_handle(interp, handle_name);
    if (!handle) {
        bcl_set_error(interp, "invalid file handle \"%s\"", handle_name);
        return BCL_ERROR;
    }

    /* Escribir texto con newline */
    if (fprintf(handle->fp, "%s\n", text) < 0) {
        bcl_set_error(interp, "error writing to file: %s", strerror(errno));
        return BCL_ERROR;
    }

    if (result) {
        *result = bcl_value_create("");
    }

    return BCL_OK;
}

/**
 * @brief Escribe a un handle de archivo sin newline
 */
bcl_result_t bcl_file_putsn(bcl_interp_t *interp, const char *handle_name,
                            const char *text, bcl_value_t **result) {
    /* Obtener handle */
    bcl_file_handle_t *handle = get_file_handle(interp, handle_name);
    if (!handle) {
        bcl_set_error(interp, "invalid file handle \"%s\"", handle_name);
        return BCL_ERROR;
    }

    /* Escribir texto sin newline */
    if (fprintf(handle->fp, "%s", text) < 0) {
        bcl_set_error(interp, "error writing to file: %s", strerror(errno));
        return BCL_ERROR;
    }

    fflush(handle->fp);

    if (result) {
        *result = bcl_value_create("");
    }

    return BCL_OK;
}

/**
 * @brief Verifica si un string es un handle de archivo válido
 */
bool bcl_is_file_handle(bcl_interp_t *interp, const char *name) {
    return (get_file_handle(interp, name) != NULL);
}

/* ========================================================================== */
/* TELL - Posición actual                                                    */
/* ========================================================================== */

bcl_result_t bcl_cmd_tell(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result) {
    if (argc != 1) {
        bcl_set_error(interp, "wrong # args: should be \"TELL handle\"");
        return BCL_ERROR;
    }

    const char *handle_name = argv[0];

    /* Obtener handle */
    bcl_file_handle_t *handle = get_file_handle(interp, handle_name);
    if (!handle) {
        bcl_set_error(interp, "invalid file handle \"%s\"", handle_name);
        return BCL_ERROR;
    }

    /* Obtener posición */
    long pos = ftell(handle->fp);
    if (pos < 0) {
        bcl_set_error(interp, "error getting file position: %s", strerror(errno));
        return BCL_ERROR;
    }

    /* Retornar posición */
    char buf[32];
    snprintf(buf, sizeof(buf), "%ld", pos);

    if (result) {
        *result = bcl_value_create(buf);
    }

    return BCL_OK;
}

/* ========================================================================== */
/* SEEK - Mover puntero                                                      */
/* ========================================================================== */

bcl_result_t bcl_cmd_seek(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result) {
    if (argc != 3) {
        bcl_set_error(interp, "wrong # args: should be \"SEEK handle offset whence\"");
        return BCL_ERROR;
    }

    const char *handle_name = argv[0];
    const char *offset_str = argv[1];
    const char *whence_str = argv[2];

    /* Obtener handle */
    bcl_file_handle_t *handle = get_file_handle(interp, handle_name);
    if (!handle) {
        bcl_set_error(interp, "invalid file handle \"%s\"", handle_name);
        return BCL_ERROR;
    }

    /* Parsear offset */
    bool ok;
    double offset_d = bcl_str_to_number(offset_str, &ok);
    if (!ok) {
        bcl_set_error(interp, "expected integer but got \"%s\"", offset_str);
        return BCL_ERROR;
    }
    long offset = (long)offset_d;

    /* Parsear whence */
    int whence;
    if (bcl_strcasecmp(whence_str, "SET") == 0 || bcl_strcasecmp(whence_str, "START") == 0) {
        whence = SEEK_SET;
    } else if (bcl_strcasecmp(whence_str, "CUR") == 0 || bcl_strcasecmp(whence_str, "CURRENT") == 0) {
        whence = SEEK_CUR;
    } else if (bcl_strcasecmp(whence_str, "END") == 0) {
        whence = SEEK_END;
    } else {
        bcl_set_error(interp, "invalid whence \"%s\": should be START/SET, CUR/CURRENT, or END", whence_str);
        return BCL_ERROR;
    }

    /* Mover puntero */
    if (fseek(handle->fp, offset, whence) != 0) {
        bcl_set_error(interp, "error seeking: %s", strerror(errno));
        return BCL_ERROR;
    }

    /* Limpiar EOF flag si nos movimos */
    handle->eof_reached = false;

    if (result) {
        *result = bcl_value_create("");
    }

    return BCL_OK;
}

/* ========================================================================== */
/* EOF - Verificar fin de archivo                                            */
/* ========================================================================== */

bcl_result_t bcl_cmd_eof(bcl_interp_t *interp, int argc, char **argv,
                         bcl_value_t **result) {
    if (argc != 1) {
        bcl_set_error(interp, "wrong # args: should be \"EOF handle\"");
        return BCL_ERROR;
    }

    const char *handle_name = argv[0];

    /* Obtener handle */
    bcl_file_handle_t *handle = get_file_handle(interp, handle_name);
    if (!handle) {
        bcl_set_error(interp, "invalid file handle \"%s\"", handle_name);
        return BCL_ERROR;
    }

    /* Verificar EOF */
    int is_eof = feof(handle->fp) || handle->eof_reached;

    if (result) {
        *result = bcl_value_create(is_eof ? "1" : "0");
    }

    return BCL_OK;
}

#endif /* BCL_NO_FILES */
