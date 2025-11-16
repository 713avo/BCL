# Propuesta de Modificación: Sistema de Arrays Mejorado para BCL

## Análisis de Problemas Identificados

### Problema 1: GLOBAL no funciona con elementos de arrays

**Causa raíz:**
- En BCL, los arrays son simplemente variables con nombres especiales: `array(key)` es una variable llamada literalmente "array(key)"
- El comando `GLOBAL` marca variables individuales en el hash `global_refs` del scope
- Cuando ejecutas:
  ```bcl
  GLOBAL myarray
  SET myarray(foo) 42
  ```
- `GLOBAL` marca "myarray" en `global_refs`
- `SET` crea una variable local llamada "myarray(foo)" (diferente de "myarray")
- La verificación en `bcl_var_set()` (línea 217 de `bcl_interp.c`) busca exactamente "myarray(foo)" en `global_refs`, no encuentra coincidencia, y crea variable local

**Código problemático** (`src/bcl_interp.c`, líneas 214-223):
```c
if (scope) {
    /* Estamos en un scope local */
    /* Verificar si está en global_refs (declarada GLOBAL) */
    if (bcl_hash_exists(scope->global_refs, name)) {  // ← Búsqueda exacta
        /* Es una variable global */
        bcl_hash_set(interp->global_vars, name, val);
    } else {
        /* Variable local */
        bcl_hash_set(scope->vars, name, val);
    }
}
```

### Problema 2: No hay forma de marcar "todos los elementos de un array" como globales

**Causa raíz:**
- No existe un mecanismo de patrones en `global_refs`
- No puedes hacer `GLOBAL myarray(*)` para marcar todos los elementos
- Debes conocer de antemano todos los índices, lo cual es impráctico

### Problema 3: Arrays no son entidades de primera clase

**Implicaciones:**
- No hay forma eficiente de verificar "¿esta variable pertenece a un array?"
- Los comandos ARRAY tienen que iterar toda la tabla hash buscando patrones
- No hay forma de optimizar operaciones masivas sobre arrays

## Propuesta de Solución

### Opción A: Modificación Mínima - GLOBAL con Soporte de Prefijos (RECOMENDADA)

**Cambios mínimos con máximo impacto**

#### 1. Modificar estructura de scope para soportar prefijos globales

**Archivo:** `include/bcl.h`

```c
/* Estructura de scope local */
typedef struct bcl_scope {
    bcl_hash_table_t *vars;          /* Variables locales */
    bcl_hash_table_t *global_refs;   /* Referencias a variables globales exactas */
    bcl_hash_table_t *global_prefixes; /* NUEVO: Prefijos globales (para arrays) */
    struct bcl_scope *parent;
} bcl_scope_t;
```

#### 2. Actualizar creación y destrucción de scopes

**Archivo:** `src/bcl_interp.c`, línea 125

```c
static bcl_scope_t *scope_create(bcl_scope_t *parent) {
    bcl_scope_t *scope = malloc(sizeof(bcl_scope_t));
    if (!scope) return NULL;

    scope->vars = bcl_hash_create();
    scope->global_refs = bcl_hash_create();
    scope->global_prefixes = bcl_hash_create();  // NUEVO
    scope->parent = parent;

    if (!scope->vars || !scope->global_refs || !scope->global_prefixes) {
        if (scope->vars) bcl_hash_destroy(scope->vars);
        if (scope->global_refs) bcl_hash_destroy(scope->global_refs);
        if (scope->global_prefixes) bcl_hash_destroy(scope->global_prefixes);  // NUEVO
        free(scope);
        return NULL;
    }

    return scope;
}
```

**Actualizar destrucción** (línea 180):
```c
if (scope) {
    if (scope->vars) bcl_hash_destroy(scope->vars);
    if (scope->global_refs) bcl_hash_destroy(scope->global_refs);
    if (scope->global_prefixes) bcl_hash_destroy(scope->global_prefixes);  // NUEVO
    free(scope);
}
```

#### 3. Nueva función auxiliar para verificar si variable es global

**Archivo:** `src/bcl_interp.c`, después de línea 196

```c
/**
 * @brief Verifica si una variable debe tratarse como global
 * @param scope Scope actual (NULL = global)
 * @param name Nombre completo de la variable
 * @return true si debe ir en global_vars
 */
static bool is_global_var(bcl_scope_t *scope, const char *name) {
    if (!scope) return true;  /* Estamos en scope global */

    /* Verificar coincidencia exacta */
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
```

#### 4. Modificar bcl_var_set para usar la nueva función

**Archivo:** `src/bcl_interp.c`, línea 202

```c
bcl_result_t bcl_var_set(bcl_interp_t *interp, const char *name,
                         const char *value) {
    if (!interp || !name) return BCL_ERROR;

    bcl_value_t *val = bcl_value_create(value);
    if (!val) {
        bcl_set_error(interp, "Out of memory");
        return BCL_ERROR;
    }

    bcl_scope_t *scope = get_current_scope(interp);

    if (is_global_var(scope, name)) {  // MODIFICADO: usar nueva función
        /* Es una variable global */
        bcl_hash_set(interp->global_vars, name, val);
    } else {
        /* Variable local */
        bcl_hash_set(scope ? scope->vars : interp->global_vars, name, val);
    }

    return BCL_OK;
}
```

#### 5. Mejorar comando GLOBAL para detectar arrays

**Archivo:** `src/bcl_commands.c`, línea 278

```c
bcl_result_t bcl_cmd_global(bcl_interp_t *interp, int argc, char **argv,
                            bcl_value_t **result) {
    if (result) *result = NULL;

    if (argc < 1) {
        bcl_set_error(interp, "GLOBAL: wrong # args: should be \"GLOBAL varName ?varName ...?\"");
        return BCL_ERROR;
    }

    /* GLOBAL solo tiene efecto si estamos en un scope local */
    if (interp->scope_depth == 0) {
        if (result) *result = bcl_value_create("");
        return BCL_OK;
    }

    bcl_scope_t *scope = interp->scope_stack[interp->scope_depth - 1];
    if (!scope) return BCL_ERROR;

    /* Marcar cada variable como global */
    for (int i = 0; i < argc; i++) {
        const char *varname = argv[i];

        /* Verificar si ya existe como array en scope global */
        /* Buscar si existe alguna variable con patrón varname(...) */
        char array_pattern[256];
        snprintf(array_pattern, sizeof(array_pattern), "%s(", varname);

        bool is_array = false;
        for (size_t j = 0; j < BCL_HASH_TABLE_SIZE && !is_array; j++) {
            bcl_hash_entry_t *entry = interp->global_vars->buckets[j];
            while (entry) {
                if (strncmp(entry->key, array_pattern, strlen(array_pattern)) == 0) {
                    is_array = true;
                    break;
                }
                entry = entry->next;
            }
        }

        if (is_array) {
            /* Marcar como prefijo global (para arrays) */
            bcl_value_t *marker = bcl_value_create("1");
            bcl_hash_set(scope->global_prefixes, array_pattern, marker);
        } else {
            /* Variable escalar normal */
            bcl_value_t *marker = bcl_value_create("1");
            bcl_hash_set(scope->global_refs, varname, marker);
        }
    }

    if (result) *result = bcl_value_create("");
    return BCL_OK;
}
```

### Opción B: Modificación Profunda - Arrays como Entidades de Primera Clase

**Cambio arquitectónico mayor con beneficios a largo plazo**

#### 1. Nueva estructura para arrays

**Archivo:** `include/bcl.h`

```c
/* Array como entidad de primera clase */
typedef struct bcl_array {
    char *name;                  /* Nombre del array */
    bcl_hash_table_t *elements;  /* Tabla hash de elementos */
    bool is_global;              /* ¿Es global? */
} bcl_array_t;

/* Añadir al intérprete */
typedef struct bcl_interp {
    bcl_hash_table_t *global_vars;
    bcl_hash_table_t *procedures;
    bcl_hash_table_t *file_handles;
    bcl_hash_table_t *arrays;        /* NUEVO: Tabla de arrays */
    // ... resto de campos
} bcl_interp_t;
```

#### 2. Funciones de manejo de arrays

**Archivo:** `src/bcl_array.c` (nuevo o extender existente)

```c
/**
 * @brief Crea o obtiene un array
 */
bcl_array_t *bcl_array_get_or_create(bcl_interp_t *interp, const char *name) {
    /* Buscar en tabla de arrays */
    bcl_value_t *arr_val = bcl_hash_get(interp->arrays, name);

    if (arr_val) {
        /* Convertir string a puntero */
        bcl_array_t *arr;
        sscanf(bcl_value_get(arr_val), "%p", (void**)&arr);
        return arr;
    }

    /* Crear nuevo array */
    bcl_array_t *arr = malloc(sizeof(bcl_array_t));
    if (!arr) return NULL;

    arr->name = bcl_strdup(name);
    arr->elements = bcl_hash_create();
    arr->is_global = false;  /* Por defecto local */

    if (!arr->name || !arr->elements) {
        if (arr->name) free(arr->name);
        if (arr->elements) bcl_hash_destroy(arr->elements);
        free(arr);
        return NULL;
    }

    /* Guardar en tabla de arrays */
    char ptr_str[32];
    snprintf(ptr_str, sizeof(ptr_str), "%p", (void*)arr);
    bcl_value_t *val = bcl_value_create(ptr_str);
    bcl_hash_set(interp->arrays, name, val);

    return arr;
}

/**
 * @brief Establece elemento de array
 */
bcl_result_t bcl_array_set_element(bcl_interp_t *interp, const char *array_name,
                                   const char *key, const char *value) {
    bcl_array_t *arr = bcl_array_get_or_create(interp, array_name);
    if (!arr) return BCL_ERROR;

    bcl_value_t *val = bcl_value_create(value);
    if (!val) return BCL_ERROR;

    bcl_hash_set(arr->elements, key, val);
    return BCL_OK;
}

/**
 * @brief Obtiene elemento de array
 */
bcl_value_t *bcl_array_get_element(bcl_interp_t *interp, const char *array_name,
                                   const char *key) {
    bcl_value_t *arr_val = bcl_hash_get(interp->arrays, array_name);
    if (!arr_val) return NULL;

    bcl_array_t *arr;
    sscanf(bcl_value_get(arr_val), "%p", (void**)&arr);
    if (!arr) return NULL;

    return bcl_hash_get(arr->elements, key);
}

/**
 * @brief Marca array como global
 */
bcl_result_t bcl_array_mark_global(bcl_interp_t *interp, const char *array_name) {
    bcl_array_t *arr = bcl_array_get_or_create(interp, array_name);
    if (!arr) return BCL_ERROR;

    arr->is_global = true;
    return BCL_OK;
}
```

#### 3. Modificar parser para usar nuevas funciones

**Archivo:** `src/bcl_parser.c`, línea 258

```c
/* Dentro de bcl_expand_vars, cuando detecta array(...) */

/* Obtener valor del array usando nueva API */
bcl_value_t *val = bcl_array_get_element(interp, varname, expanded_index);
if (val) {
    bcl_string_append(result, bcl_value_get(val));
}
```

#### 4. Modificar comando SET

**Archivo:** `src/bcl_commands.c`, línea 34

```c
else {
    /* Asignar valor */
    const char *value = argv[1];

    /* Verificar si es array: varname(key) */
    const char *paren = strchr(varname, '(');
    if (paren) {
        /* Extraer nombre de array y clave */
        size_t name_len = paren - varname;
        char array_name[256];
        memcpy(array_name, varname, name_len);
        array_name[name_len] = '\0';

        /* Extraer clave */
        const char *key_start = paren + 1;
        const char *key_end = strchr(key_start, ')');
        if (key_end) {
            size_t key_len = key_end - key_start;
            char key[256];
            memcpy(key, key_start, key_len);
            key[key_len] = '\0';

            /* Usar API de arrays */
            if (bcl_array_set_element(interp, array_name, key, value) != BCL_OK) {
                return BCL_ERROR;
            }
        }
    } else {
        /* Variable normal */
        if (bcl_var_set(interp, varname, value) != BCL_OK) {
            return BCL_ERROR;
        }
    }

    *result = bcl_value_create(value);
}
```

## Comparación de Opciones

| Aspecto | Opción A (Prefijos) | Opción B (Primera Clase) |
|---------|-------------------|-------------------------|
| Complejidad | Baja (50-100 líneas) | Alta (300-500 líneas) |
| Compatibilidad | 100% compatible | Requiere migración |
| Rendimiento | Igual o mejor | Mejor para arrays grandes |
| Mantenibilidad | Simple | Más robusto |
| Riesgo | Bajo | Medio |

## Recomendación

**Implementar Opción A (Prefijos)** por las siguientes razones:

1. **Mínima invasividad**: Solo modifica el sistema de scopes sin cambiar arquitectura fundamental
2. **Compatibilidad total**: Todo el código existente sigue funcionando
3. **Soluciona el problema principal**: GLOBAL funciona correctamente con arrays
4. **Bajo riesgo**: Cambios localizados y fáciles de probar
5. **Implementación rápida**: 1-2 horas de desarrollo

La Opción B sería recomendable para una versión 2.0 de BCL donde se puedan hacer cambios disruptivos.

## Plan de Implementación (Opción A)

### Fase 1: Modificación de estructuras (15 min)
- Añadir `global_prefixes` a `bcl_scope_t`
- Actualizar `scope_create()` y destrucción

### Fase 2: Función auxiliar (20 min)
- Implementar `is_global_var()`
- Añadir tests unitarios

### Fase 3: Actualizar bcl_var_set (10 min)
- Usar `is_global_var()` en lugar de verificación directa
- Validar con tests existentes

### Fase 4: Mejorar GLOBAL (30 min)
- Detectar si variable es array
- Marcar prefijo en lugar de nombre exacto
- Tests exhaustivos

### Fase 5: Testing (30 min)
- Ejecutar suite de tests completa
- Añadir tests específicos para arrays globales
- Validar casos edge

### Fase 6: Documentación (15 min)
- Actualizar man pages
- Añadir ejemplos a docs/man_llm.md

**Tiempo total estimado: 2 horas**

## Tests Propuestos

```bcl
#!/usr/bin/env bcl
# test_global_arrays.bcl

PROC TEST_ARRAY DO
    GLOBAL myarray
    SET myarray(foo) 42
    SET myarray(bar) 99
    INCR myarray(foo)
END

SET myarray(foo) 10
SET myarray(bar) 20

TEST_ARRAY

# Después del proc, los valores deben ser:
# myarray(foo) = 43 (10 + 1 desde el proc)
# myarray(bar) = 99 (sobrescrito en el proc)

IF [EXPR $myarray(foo) == 43] THEN
    PUTS "PASS: GLOBAL array modification works"
ELSE
    PUTS "FAIL: Expected 43, got $myarray(foo)"
END

IF [EXPR $myarray(bar) == 99] THEN
    PUTS "PASS: GLOBAL array element setting works"
ELSE
    PUTS "FAIL: Expected 99, got $myarray(bar)"
END
```

## Conclusión

La implementación de la Opción A resolverá los problemas identificados de forma eficiente y segura, permitiendo que librerías como MATRIX funcionen correctamente con arrays compartidos entre procedimientos.
