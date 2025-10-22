/**
 * @file bcl_file_ops.c
 * @brief Operaciones de filesystem (PWD, FILE, GLOB)
 * @version 1.0
 *
 * Comandos: PWD, FILE (EXISTS/SIZE/DELETE/RENAME), GLOB
 */

#include "bcl.h"
#include "bcl_commands.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#ifndef BCL_NO_FILES

#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fnmatch.h>

/* ========================================================================== */
/* PWD - Directorio actual                                                   */
/* ========================================================================== */

bcl_result_t bcl_cmd_pwd(bcl_interp_t *interp, int argc, char **argv,
                         bcl_value_t **result) {
    BCL_UNUSED(argv);

    if (argc != 0) {
        bcl_set_error(interp, "wrong # args: should be \"PWD\"");
        return BCL_ERROR;
    }

    char cwd[4096];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        bcl_set_error(interp, "couldn't get current directory: %s", strerror(errno));
        return BCL_ERROR;
    }

    if (result) {
        *result = bcl_value_create(cwd);
    }

    return BCL_OK;
}

/* ========================================================================== */
/* FILE - Operaciones de filesystem                                          */
/* ========================================================================== */

/**
 * @brief FILE EXISTS - Verificar existencia de archivo/directorio
 */
static bcl_result_t file_exists(bcl_interp_t *interp, int argc, char **argv,
                                bcl_value_t **result) {
    if (argc != 1) {
        bcl_set_error(interp, "wrong # args: should be \"FILE EXISTS path\"");
        return BCL_ERROR;
    }

    const char *path = argv[0];
    struct stat st;

    int exists = (stat(path, &st) == 0) ? 1 : 0;

    if (result) {
        *result = bcl_value_create(exists ? "1" : "0");
    }

    return BCL_OK;
}

/**
 * @brief FILE SIZE - Obtener tamaño de archivo
 */
static bcl_result_t file_size(bcl_interp_t *interp, int argc, char **argv,
                              bcl_value_t **result) {
    if (argc != 1) {
        bcl_set_error(interp, "wrong # args: should be \"FILE SIZE path\"");
        return BCL_ERROR;
    }

    const char *path = argv[0];
    struct stat st;

    if (stat(path, &st) != 0) {
        bcl_set_error(interp, "couldn't stat \"%s\": %s", path, strerror(errno));
        return BCL_ERROR;
    }

    /* Retornar tamaño en bytes */
    char buf[64];
    snprintf(buf, sizeof(buf), "%lld", (long long)st.st_size);

    if (result) {
        *result = bcl_value_create(buf);
    }

    return BCL_OK;
}

/**
 * @brief FILE DELETE - Eliminar archivo
 */
static bcl_result_t file_delete(bcl_interp_t *interp, int argc, char **argv,
                                bcl_value_t **result) {
    if (argc != 1) {
        bcl_set_error(interp, "wrong # args: should be \"FILE DELETE path\"");
        return BCL_ERROR;
    }

    const char *path = argv[0];

    if (unlink(path) != 0) {
        /* Si falla unlink, podría ser un directorio */
        if (rmdir(path) != 0) {
            bcl_set_error(interp, "couldn't delete \"%s\": %s", path, strerror(errno));
            return BCL_ERROR;
        }
    }

    if (result) {
        *result = bcl_value_create("");
    }

    return BCL_OK;
}

/**
 * @brief FILE RENAME - Renombrar/mover archivo
 */
static bcl_result_t file_rename(bcl_interp_t *interp, int argc, char **argv,
                                bcl_value_t **result) {
    if (argc != 2) {
        bcl_set_error(interp, "wrong # args: should be \"FILE RENAME source dest\"");
        return BCL_ERROR;
    }

    const char *source = argv[0];
    const char *dest = argv[1];

    if (rename(source, dest) != 0) {
        bcl_set_error(interp, "couldn't rename \"%s\" to \"%s\": %s",
                     source, dest, strerror(errno));
        return BCL_ERROR;
    }

    if (result) {
        *result = bcl_value_create("");
    }

    return BCL_OK;
}

/**
 * @brief FILE - Dispatcher de subcomandos
 */
bcl_result_t bcl_cmd_file(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result) {
    if (argc < 1) {
        bcl_set_error(interp, "wrong # args: should be \"FILE subcommand ?arg ...?\"");
        return BCL_ERROR;
    }

    const char *subcmd = argv[0];

    /* Dispatcher de subcomandos */
    if (bcl_strcasecmp(subcmd, "EXISTS") == 0) {
        return file_exists(interp, argc - 1, argv + 1, result);
    } else if (bcl_strcasecmp(subcmd, "SIZE") == 0) {
        return file_size(interp, argc - 1, argv + 1, result);
    } else if (bcl_strcasecmp(subcmd, "DELETE") == 0) {
        return file_delete(interp, argc - 1, argv + 1, result);
    } else if (bcl_strcasecmp(subcmd, "RENAME") == 0) {
        return file_rename(interp, argc - 1, argv + 1, result);
    } else {
        bcl_set_error(interp, "unknown subcommand \"%s\": should be EXISTS, SIZE, DELETE, or RENAME", subcmd);
        return BCL_ERROR;
    }
}

/* ========================================================================== */
/* GLOB - Expansión de patrones                                              */
/* ========================================================================== */

/**
 * @brief Estructura para almacenar opciones de GLOB
 */
typedef struct {
    char *directory;        /* Directorio base */
    bool tails;            /* Solo nombres, no rutas completas */
    bool nocomplain;       /* No error si no hay matches */
    char types;            /* 'f' files, 'd' dirs, '\0' ambos */
} glob_options_t;

/**
 * @brief Verifica si un archivo coincide con el tipo especificado
 */
static bool match_type(const char *path, char type) {
    if (type == '\0') {
        return true;  /* Aceptar todos */
    }

    struct stat st;
    if (stat(path, &st) != 0) {
        return false;
    }

    if (type == 'f') {
        return S_ISREG(st.st_mode);
    } else if (type == 'd') {
        return S_ISDIR(st.st_mode);
    }

    return false;
}

/**
 * @brief Añade una ruta al resultado si coincide con el patrón
 */
static void add_match(bcl_string_t *result, const char *path, bool tails) {
    if (bcl_string_cstr(result)[0] != '\0') {
        bcl_string_append(result, " ");
    }

    if (tails) {
        /* Solo el nombre del archivo */
        const char *slash = strrchr(path, '/');
        bcl_string_append(result, slash ? slash + 1 : path);
    } else {
        /* Ruta completa */
        bcl_string_append(result, path);
    }
}

/**
 * @brief Busca archivos que coincidan con el patrón
 */
static int glob_search(const char *directory, const char *pattern,
                       glob_options_t *opts, bcl_string_t *result) {
    DIR *dir = opendir(directory);
    if (!dir) {
        return -1;
    }

    int match_count = 0;
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        /* Saltar . y .. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        /* Verificar si coincide con el patrón */
        if (fnmatch(pattern, entry->d_name, 0) == 0) {
            /* Construir ruta completa */
            char full_path[4096];
            snprintf(full_path, sizeof(full_path), "%s/%s", directory, entry->d_name);

            /* Verificar tipo si se especificó */
            if (match_type(full_path, opts->types)) {
                add_match(result, full_path, opts->tails);
                match_count++;
            }
        }
    }

    closedir(dir);
    return match_count;
}

/**
 * @brief GLOB - Expansión de patrones de archivos
 */
bcl_result_t bcl_cmd_glob(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result) {
    if (argc < 1) {
        bcl_set_error(interp, "wrong # args: should be \"GLOB pattern ?options?\"");
        return BCL_ERROR;
    }

    const char *pattern = argv[0];

    /* Opciones por defecto */
    glob_options_t opts = {
        .directory = NULL,
        .tails = false,
        .nocomplain = false,
        .types = '\0'
    };

    /* Parsear opciones */
    for (int i = 1; i < argc; i++) {
        if (bcl_strcasecmp(argv[i], "DIRECTORY") == 0) {
            if (i + 1 >= argc) {
                bcl_set_error(interp, "DIRECTORY option requires argument");
                return BCL_ERROR;
            }
            opts.directory = argv[++i];
        } else if (bcl_strcasecmp(argv[i], "TAILS") == 0) {
            opts.tails = true;
        } else if (bcl_strcasecmp(argv[i], "NOCOMPLAIN") == 0) {
            opts.nocomplain = true;
        } else if (bcl_strcasecmp(argv[i], "TYPES") == 0) {
            if (i + 1 >= argc) {
                bcl_set_error(interp, "TYPES option requires argument");
                return BCL_ERROR;
            }
            const char *types_str = argv[++i];
            if (strlen(types_str) > 0) {
                opts.types = types_str[0];  /* f o d */
            }
        } else {
            bcl_set_error(interp, "unknown option \"%s\"", argv[i]);
            return BCL_ERROR;
        }
    }

    /* Determinar directorio base */
    const char *search_dir = opts.directory ? opts.directory : ".";

    /* Separar directorio y patrón si el patrón incluye / */
    char dir_buf[4096];
    const char *pattern_only = pattern;

    const char *last_slash = strrchr(pattern, '/');
    if (last_slash) {
        /* El patrón incluye directorio */
        size_t dir_len = last_slash - pattern;
        if (dir_len >= sizeof(dir_buf)) {
            bcl_set_error(interp, "path too long");
            return BCL_ERROR;
        }

        strncpy(dir_buf, pattern, dir_len);
        dir_buf[dir_len] = '\0';
        search_dir = dir_buf;
        pattern_only = last_slash + 1;
    }

    /* Buscar coincidencias */
    bcl_string_t *matches = bcl_string_create("");
    int match_count = glob_search(search_dir, pattern_only, &opts, matches);

    if (match_count < 0) {
        bcl_string_destroy(matches);
        bcl_set_error(interp, "couldn't read directory \"%s\": %s",
                     search_dir, strerror(errno));
        return BCL_ERROR;
    }

    if (match_count == 0 && !opts.nocomplain) {
        bcl_string_destroy(matches);
        bcl_set_error(interp, "no files matched glob pattern \"%s\"", pattern);
        return BCL_ERROR;
    }

    if (result) {
        *result = bcl_value_create(bcl_string_cstr(matches));
    }

    bcl_string_destroy(matches);
    return BCL_OK;
}

#endif /* BCL_NO_FILES */
