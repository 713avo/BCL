/**
 * @file bcl_extensions.c
 * @brief BCL - Sistema de extensiones dinámicas
 * @version 2.0.0
 *
 * Permite cargar módulos .so que añaden nuevos comandos a BCL en tiempo de ejecución.
 */

#include "bcl.h"
#include "bcl_commands.h"
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>

/* ========================================================================== */
/* REGISTRO DE COMANDOS DESDE EXTENSIONES                                    */
/* ========================================================================== */

/**
 * @brief Wrapper para comandos de extensión
 *
 * Esta estructura almacena el puntero a la función de la extensión
 * como un valor BCL para poder almacenarlo en la hash table.
 */
typedef struct {
    bcl_extension_cmd_func_t func;
} bcl_extension_cmd_wrapper_t;

/**
 * @brief Registra un nuevo comando desde una extensión
 */
static int bcl_extension_register_command(bcl_interp_t *interp, const char *name,
                                         bcl_extension_cmd_func_t func) {
    if (!interp || !name || !func) return -1;

    /* Crear wrapper para la función */
    bcl_extension_cmd_wrapper_t *wrapper = malloc(sizeof(bcl_extension_cmd_wrapper_t));
    if (!wrapper) return -1;

    wrapper->func = func;

    /* Convertir el puntero a string para almacenar en hash table */
    char ptr_str[32];
    snprintf(ptr_str, sizeof(ptr_str), "%p", (void*)wrapper);

    /* Almacenar en tabla de comandos de extensiones */
    bcl_value_t *val = bcl_value_create(ptr_str);
    if (!val) {
        free(wrapper);
        return -1;
    }

    bcl_hash_set(interp->extension_cmds, name, val);
    return 0;
}

/* ========================================================================== */
/* API PARA EXTENSIONES                                                      */
/* ========================================================================== */

/**
 * @brief Crea la estructura de API para pasar a extensiones
 */
static bcl_extension_api_t *bcl_create_extension_api(bcl_interp_t *interp) {
    bcl_extension_api_t *api = malloc(sizeof(bcl_extension_api_t));
    if (!api) return NULL;

    api->version = BCL_EXTENSION_API_VERSION;
    api->interp = interp;
    api->register_command = bcl_extension_register_command;
    api->set_error = bcl_set_error;
    api->value_create = bcl_value_create;
    api->value_destroy = bcl_value_destroy;
    api->value_get = bcl_value_get;
    api->var_set = bcl_var_set;
    api->var_get = bcl_var_get;

    return api;
}

/* ========================================================================== */
/* COMANDO LOAD                                                              */
/* ========================================================================== */

/**
 * LOAD ruta_extension
 *
 * Carga una extensión dinámica (.so) y ejecuta su función de inicialización.
 *
 * Ejemplo:
 *   LOAD "extensions/socket.so"
 *   # Ahora el comando SOCKET está disponible
 *   SOCKET server 8080
 */
bcl_result_t bcl_cmd_load(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result) {
    if (argc != 1) {
        bcl_set_error(interp, "LOAD: wrong # args: should be \"LOAD path\"");
        return BCL_ERROR;
    }

    const char *path = argv[0];

    /* Verificar si ya está cargada */
    for (bcl_extension_t *ext = interp->extensions; ext != NULL; ext = ext->next) {
        if (strcmp(ext->path, path) == 0) {
            bcl_set_error(interp, "LOAD: extension \"%s\" already loaded", path);
            return BCL_ERROR;
        }
    }

    /* Cargar biblioteca dinámica */
    void *handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        bcl_set_error(interp, "LOAD: cannot load \"%s\": %s", path, dlerror());
        return BCL_ERROR;
    }

    /* Buscar función de inicialización */
    bcl_extension_init_func_t init_func =
        (bcl_extension_init_func_t)dlsym(handle, "bcl_extension_init");

    if (!init_func) {
        bcl_set_error(interp, "LOAD: \"%s\" does not export bcl_extension_init", path);
        dlclose(handle);
        return BCL_ERROR;
    }

    /* Crear API para la extensión */
    bcl_extension_api_t *api = bcl_create_extension_api(interp);
    if (!api) {
        bcl_set_error(interp, "LOAD: out of memory");
        dlclose(handle);
        return BCL_ERROR;
    }

    /* Llamar a la función de inicialización */
    int init_result = init_func(api);
    free(api);

    if (init_result != 0) {
        bcl_set_error(interp, "LOAD: initialization of \"%s\" failed", path);
        dlclose(handle);
        return BCL_ERROR;
    }

    /* Agregar a lista de extensiones cargadas */
    bcl_extension_t *ext = malloc(sizeof(bcl_extension_t));
    if (!ext) {
        bcl_set_error(interp, "LOAD: out of memory");
        dlclose(handle);
        return BCL_ERROR;
    }

    ext->dl_handle = handle;
    ext->path = bcl_strdup(path);
    ext->name = bcl_strdup(path); /* Simplificado - podría extraer nombre del path */
    ext->next = interp->extensions;
    interp->extensions = ext;

    *result = bcl_value_create("");
    return BCL_OK;
}

/* ========================================================================== */
/* DESPACHO DE COMANDOS DE EXTENSIONES                                       */
/* ========================================================================== */

/**
 * @brief Verifica si un comando está registrado por una extensión
 */
bool bcl_is_extension_command(bcl_interp_t *interp, const char *name) {
    if (!interp || !interp->extension_cmds || !name) return false;
    return bcl_hash_exists(interp->extension_cmds, name);
}

/**
 * @brief Ejecuta un comando de extensión
 */
bcl_result_t bcl_call_extension_command(bcl_interp_t *interp, const char *name,
                                       int argc, char **argv, bcl_value_t **result) {
    if (!interp || !name) return BCL_ERROR;

    bcl_value_t *cmd_val = bcl_hash_get(interp->extension_cmds, name);
    if (!cmd_val) {
        bcl_set_error(interp, "unknown command \"%s\"", name);
        return BCL_ERROR;
    }

    /* Convertir string de vuelta a puntero */
    void *wrapper_ptr = NULL;
    sscanf(bcl_value_get(cmd_val), "%p", &wrapper_ptr);

    if (!wrapper_ptr) {
        bcl_set_error(interp, "invalid extension command \"%s\"", name);
        return BCL_ERROR;
    }

    bcl_extension_cmd_wrapper_t *wrapper = (bcl_extension_cmd_wrapper_t*)wrapper_ptr;

    /* Llamar a la función de la extensión */
    return wrapper->func(interp, argc, argv, result);
}

/* ========================================================================== */
/* INICIALIZACIÓN Y LIMPIEZA                                                 */
/* ========================================================================== */

/**
 * @brief Inicializa el sistema de extensiones (llamado desde bcl_interp_create)
 */
void bcl_extensions_init(bcl_interp_t *interp) {
    if (!interp) return;

    interp->extensions = NULL;
    interp->extension_cmds = bcl_hash_create();
}

/**
 * @brief Limpia el sistema de extensiones (llamado desde bcl_interp_destroy)
 */
void bcl_extensions_cleanup(bcl_interp_t *interp) {
    if (!interp) return;

    /* Cerrar todas las extensiones */
    bcl_extension_t *ext = interp->extensions;
    while (ext) {
        bcl_extension_t *next = ext->next;

        if (ext->dl_handle) {
            dlclose(ext->dl_handle);
        }
        free(ext->path);
        free(ext->name);
        free(ext);

        ext = next;
    }
    interp->extensions = NULL;

    /* Liberar wrappers de comandos */
    if (interp->extension_cmds) {
        size_t count;
        char **keys = bcl_hash_keys(interp->extension_cmds, &count);

        for (size_t i = 0; i < count; i++) {
            bcl_value_t *val = bcl_hash_get(interp->extension_cmds, keys[i]);
            if (val) {
                void *wrapper_ptr = NULL;
                sscanf(bcl_value_get(val), "%p", &wrapper_ptr);
                if (wrapper_ptr) {
                    free(wrapper_ptr);
                }
            }
        }

        if (keys) {
            for (size_t i = 0; i < count; i++) {
                free(keys[i]);
            }
            free(keys);
        }

        bcl_hash_destroy(interp->extension_cmds);
        interp->extension_cmds = NULL;
    }
}
