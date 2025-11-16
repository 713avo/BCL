/**
 * @file bcl_interp.c
 * @brief Implementación del intérprete principal
 */

#include "bcl.h"
#include "bcl_commands.h"
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

/* Forward declarations for extension system */
void bcl_extensions_init(bcl_interp_t *interp);
void bcl_extensions_cleanup(bcl_interp_t *interp);

/* ========================================================================== */
/* INTÉRPRETE - CREACIÓN Y DESTRUCCIÓN                                       */
/* ========================================================================== */

bcl_interp_t *bcl_interp_create(void) {
    bcl_interp_t *interp = malloc(sizeof(bcl_interp_t));
    if (!interp) return NULL;

    /* Inicializar variables globales */
    interp->global_vars = bcl_hash_create();
    if (!interp->global_vars) {
        free(interp);
        return NULL;
    }

    /* Inicializar procedimientos */
    interp->procedures = bcl_hash_create();
    if (!interp->procedures) {
        bcl_hash_destroy(interp->global_vars);
        free(interp);
        return NULL;
    }

    /* Inicializar handles de archivos */
    interp->file_handles = bcl_hash_create();
    if (!interp->file_handles) {
        bcl_hash_destroy(interp->global_vars);
        bcl_hash_destroy(interp->procedures);
        free(interp);
        return NULL;
    }
    interp->next_handle_id = 1;

    /* Inicializar stack de scopes */
    interp->scope_stack = malloc(sizeof(bcl_scope_t*) * BCL_MAX_SCOPE_DEPTH);
    if (!interp->scope_stack) {
        bcl_hash_destroy(interp->global_vars);
        bcl_hash_destroy(interp->procedures);
        bcl_hash_destroy(interp->file_handles);
        free(interp);
        return NULL;
    }
    interp->scope_depth = 0; /* Solo scope global (sin scopes locales) */

    /* Estado inicial */
    interp->flow_result = BCL_OK;
    interp->return_value = NULL;
    interp->exit_code = 0;
    interp->interactive = false;
    interp->recursion_depth = 0;

    /* Sin argumentos inicialmente */
    interp->argc = 0;
    interp->argv = NULL;

    /* Sin error inicial */
    interp->error_msg[0] = '\0';

    /* Inicializar sistema de extensiones */
    bcl_extensions_init(interp);

    return interp;
}

void bcl_interp_destroy(bcl_interp_t *interp) {
    if (!interp) return;

    /* Limpiar sistema de extensiones */
    bcl_extensions_cleanup(interp);

    bcl_hash_destroy(interp->global_vars);
    bcl_hash_destroy(interp->procedures);
    bcl_hash_destroy(interp->file_handles);

    /* Liberar stack de scopes */
    if (interp->scope_stack) {
        /* Pop todos los scopes que puedan quedar */
        while (interp->scope_depth > 0) {
            bcl_scope_t *scope = interp->scope_stack[interp->scope_depth - 1];
            if (scope) {
                if (scope->vars) bcl_hash_destroy(scope->vars);
                if (scope->global_refs) bcl_hash_destroy(scope->global_refs);
                if (scope->global_prefixes) bcl_hash_destroy(scope->global_prefixes);
                free(scope);
            }
            interp->scope_depth--;
        }
        free(interp->scope_stack);
    }

    if (interp->return_value) {
        bcl_value_destroy(interp->return_value);
    }

    free(interp);
}

/* ========================================================================== */
/* MANEJO DE ERRORES                                                         */
/* ========================================================================== */

const char *bcl_get_error(bcl_interp_t *interp) {
    return interp ? interp->error_msg : "No interpreter";
}

void bcl_set_error(bcl_interp_t *interp, const char *fmt, ...) {
    if (!interp) return;

    va_list args;
    va_start(args, fmt);
    vsnprintf(interp->error_msg, sizeof(interp->error_msg), fmt, args);
    va_end(args);
}

/* ========================================================================== */
/* MANEJO DE SCOPES                                                          */
/* ========================================================================== */

/**
 * @brief Crea un nuevo scope local
 */
static bcl_scope_t *scope_create(bcl_scope_t *parent) {
    bcl_scope_t *scope = malloc(sizeof(bcl_scope_t));
    if (!scope) return NULL;

    scope->vars = bcl_hash_create();
    scope->global_refs = bcl_hash_create();
    scope->global_prefixes = bcl_hash_create();
    scope->parent = parent;

    if (!scope->vars || !scope->global_refs || !scope->global_prefixes) {
        if (scope->vars) bcl_hash_destroy(scope->vars);
        if (scope->global_refs) bcl_hash_destroy(scope->global_refs);
        if (scope->global_prefixes) bcl_hash_destroy(scope->global_prefixes);
        free(scope);
        return NULL;
    }

    return scope;
}

/**
 * @brief Push de un nuevo scope al stack
 */
bcl_result_t bcl_scope_push(bcl_interp_t *interp) {
    if (!interp) return BCL_ERROR;

    if (interp->scope_depth >= BCL_MAX_SCOPE_DEPTH) {
        bcl_set_error(interp, "Maximum scope depth exceeded");
        return BCL_ERROR;
    }

    bcl_scope_t *parent = (interp->scope_depth > 0) ?
                          interp->scope_stack[interp->scope_depth - 1] : NULL;

    bcl_scope_t *new_scope = scope_create(parent);
    if (!new_scope) {
        bcl_set_error(interp, "Out of memory creating scope");
        return BCL_ERROR;
    }

    interp->scope_stack[interp->scope_depth++] = new_scope;
    return BCL_OK;
}

/**
 * @brief Pop del scope actual
 */
bcl_result_t bcl_scope_pop(bcl_interp_t *interp) {
    if (!interp) return BCL_ERROR;

    if (interp->scope_depth == 0) {
        bcl_set_error(interp, "No scope to pop");
        return BCL_ERROR;
    }

    bcl_scope_t *scope = interp->scope_stack[--interp->scope_depth];

    if (scope) {
        if (scope->vars) bcl_hash_destroy(scope->vars);
        if (scope->global_refs) bcl_hash_destroy(scope->global_refs);
        if (scope->global_prefixes) bcl_hash_destroy(scope->global_prefixes);
        free(scope);
    }

    return BCL_OK;
}

/**
 * @brief Obtiene el scope actual (o NULL si solo hay scope global)
 */
static bcl_scope_t *get_current_scope(bcl_interp_t *interp) {
    if (!interp || interp->scope_depth == 0) return NULL;
    return interp->scope_stack[interp->scope_depth - 1];
}

/**
 * @brief Verifica si una variable debe tratarse como global
 * @param scope Scope actual (NULL = global)
 * @param name Nombre completo de la variable (puede incluir índice de array)
 * @return true si la variable debe ir en global_vars
 */
static bool is_global_var(bcl_scope_t *scope, const char *name) {
    if (!scope) return true;  /* Estamos en scope global */

    /* Verificar coincidencia exacta en global_refs */
    if (bcl_hash_exists(scope->global_refs, name)) {
        return true;
    }

    /* Verificar coincidencia por prefijo (para arrays) */
    /* Si name es "myarray(foo)", buscar si "myarray(" está en global_prefixes */
    const char *paren = strchr(name, '(');
    if (paren) {
        /* Construir prefijo: "myarray(" */
        size_t prefix_len = paren - name + 1;
        char prefix[256];
        if (prefix_len < sizeof(prefix)) {
            memcpy(prefix, name, prefix_len);
            prefix[prefix_len] = '\0';

            if (bcl_hash_exists(scope->global_prefixes, prefix)) {
                return true;
            }
        }
    }

    return false;
}

/* ========================================================================== */
/* VARIABLES                                                                 */
/* ========================================================================== */

bcl_result_t bcl_var_set(bcl_interp_t *interp, const char *name,
                         const char *value) {
    if (!interp || !name) return BCL_ERROR;

    bcl_value_t *val = bcl_value_create(value);
    if (!val) {
        bcl_set_error(interp, "Out of memory");
        return BCL_ERROR;
    }

    bcl_scope_t *scope = get_current_scope(interp);

    /* Usar is_global_var para verificar si debe ir en global_vars */
    if (is_global_var(scope, name)) {
        /* Es una variable global */
        bcl_hash_set(interp->global_vars, name, val);
    } else {
        /* Variable local */
        bcl_hash_set(scope->vars, name, val);
    }

    return BCL_OK;
}

bcl_value_t *bcl_var_get(bcl_interp_t *interp, const char *name) {
    if (!interp || !name) return NULL;

    bcl_scope_t *scope = get_current_scope(interp);

    if (scope) {
        /* Buscar en scope local primero */
        bcl_value_t *val = bcl_hash_get(scope->vars, name);
        if (val) return val;

        /* Si no está en local, buscar en global */
        /* (o si está declarada GLOBAL) */
    }

    /* Buscar en scope global */
    return bcl_hash_get(interp->global_vars, name);
}

bool bcl_var_exists(bcl_interp_t *interp, const char *name) {
    if (!interp || !name) return false;

    bcl_scope_t *scope = get_current_scope(interp);

    if (scope) {
        /* Buscar en scope local primero */
        if (bcl_hash_exists(scope->vars, name)) return true;
    }

    /* Buscar en scope global */
    return bcl_hash_exists(interp->global_vars, name);
}

bcl_result_t bcl_var_unset(bcl_interp_t *interp, const char *name) {
    if (!interp || !name) return BCL_ERROR;

    bcl_scope_t *scope = get_current_scope(interp);

    if (scope) {
        /* Remover de scope local si existe */
        if (bcl_hash_exists(scope->vars, name)) {
            bcl_hash_remove(scope->vars, name);
            return BCL_OK;
        }
    }

    /* Remover de scope global */
    bcl_hash_remove(interp->global_vars, name);

    return BCL_OK;
}

/* ========================================================================== */
/* PROCEDIMIENTOS                                                            */
/* ========================================================================== */

/**
 * @brief Crea un nuevo procedimiento
 */
static bcl_procedure_t *proc_create(const char *name, bcl_param_t *params,
                                    size_t param_count, bcl_block_t *body_block) {
    bcl_procedure_t *proc = malloc(sizeof(bcl_procedure_t));
    if (!proc) return NULL;

    proc->name = bcl_strdup(name);
    proc->param_count = param_count;
    proc->body_block = body_block; /* Store block directly */

    /* Copiar parámetros */
    if (param_count > 0) {
        proc->params = malloc(sizeof(bcl_param_t) * param_count);
        if (!proc->params) {
            free(proc->name);
            free(proc);
            return NULL;
        }

        for (size_t i = 0; i < param_count; i++) {
            proc->params[i].name = bcl_strdup(params[i].name);
            proc->params[i].optional = params[i].optional;
        }
    } else {
        proc->params = NULL;
    }

    return proc;
}

/**
 * @brief Destruye un procedimiento
 */
static void proc_destroy(bcl_procedure_t *proc) {
    if (!proc) return;

    if (proc->name) free(proc->name);

    if (proc->params) {
        for (size_t i = 0; i < proc->param_count; i++) {
            if (proc->params[i].name) free(proc->params[i].name);
        }
        free(proc->params);
    }

    if (proc->body_block) {
        bcl_block_free(proc->body_block);
    }

    free(proc);
}

bcl_result_t bcl_proc_define(bcl_interp_t *interp, const char *name,
                             bcl_param_t *params, size_t param_count,
                             bcl_block_t *body_block) {
    if (!interp || !name) return BCL_ERROR;

    /* Crear procedimiento */
    bcl_procedure_t *proc = proc_create(name, params, param_count, body_block);
    if (!proc) {
        bcl_set_error(interp, "Out of memory creating procedure");
        return BCL_ERROR;
    }

    /* Guardar en hash como string con el puntero */
    /* Convertir puntero a string */
    char ptr_str[32];
    snprintf(ptr_str, sizeof(ptr_str), "%p", (void*)proc);

    bcl_value_t *val = bcl_value_create(ptr_str);
    bcl_hash_set(interp->procedures, name, val);

    return BCL_OK;
}

bcl_result_t bcl_proc_call(bcl_interp_t *interp, const char *name,
                           bcl_value_t **args, size_t arg_count,
                           bcl_value_t **result) {
    if (!interp || !name) return BCL_ERROR;

    /* Buscar procedimiento */
    bcl_value_t *proc_val = bcl_hash_get(interp->procedures, name);
    if (!proc_val) {
        bcl_set_error(interp, "invalid command name \"%s\"", name);
        return BCL_ERROR;
    }

    /* Convertir string a puntero */
    bcl_procedure_t *proc;
    sscanf(bcl_value_get(proc_val), "%p", (void**)&proc);

    if (!proc) {
        bcl_set_error(interp, "corrupted procedure \"%s\"", name);
        return BCL_ERROR;
    }

    /* Verificar número de argumentos */
    size_t required_args = 0;
    for (size_t i = 0; i < proc->param_count; i++) {
        if (!proc->params[i].optional) required_args++;
    }

    if (arg_count < required_args) {
        bcl_set_error(interp, "wrong # args: should be \"%s %s\"",
                     name, "param ...");
        return BCL_ERROR;
    }

    /* Crear nuevo scope local */
    bcl_result_t res = bcl_scope_push(interp);
    if (res != BCL_OK) return res;

    /* Asignar argumentos a parámetros */
    for (size_t i = 0; i < proc->param_count && i < arg_count; i++) {
        const char *param_name = proc->params[i].name;
        const char *param_value = bcl_value_get(args[i]);
        bcl_var_set(interp, param_name, param_value);
    }

    /* Ejecutar cuerpo del procedimiento directamente (ya parseado) */
    res = BCL_OK;
    if (proc->body_block) {
        res = bcl_exec_block(interp, proc->body_block);
    }

    bcl_value_t *proc_result = NULL;

    /* Manejar RETURN */
    if (res == BCL_RETURN) {
        /* RETURN es normal en procedimientos */
        res = BCL_OK;
        if (interp->return_value) {
            proc_result = interp->return_value;
            interp->return_value = NULL;
        }
    }

    /* Pop scope local */
    bcl_scope_pop(interp);

    /* Retornar resultado */
    if (result) {
        *result = proc_result ? proc_result : bcl_value_create("");
    } else if (proc_result) {
        bcl_value_destroy(proc_result);
    }

    return res;
}

bool bcl_proc_exists(bcl_interp_t *interp, const char *name) {
    if (!interp || !name) return false;
    return bcl_hash_exists(interp->procedures, name);
}
