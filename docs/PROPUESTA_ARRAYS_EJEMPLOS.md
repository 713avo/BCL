# Ejemplos Prácticos: Sistema de Arrays Mejorado

## Estado Actual vs Estado Propuesto

### Ejemplo 1: Librería MATRIX Simple

#### ❌ Problema Actual

```bcl
SOURCE "lib/MATRIX.BLB"

PROC MAT_ZEROS WITH name rows cols DO
    FOR 0 TO [EXPR $rows - 1] DO
        SET r $__FOR
        FOR 0 TO [EXPR $cols - 1] DO
            SET c $__FOR
            SET $name($r,$c) 0    # ← Crea variable LOCAL $name($r,$c)
        END
    END
END

# Uso:
MAT_ZEROS M 3 3
PUTS $M(0,0)    # ← ERROR: $M(0,0) no existe (fue creada localmente en el proc)
```

**Por qué falla:**
1. Dentro de `MAT_ZEROS`, `SET $name($r,$c) 0` expande a `SET M(0,0) 0`
2. Como estamos en scope local (dentro del proc), crea `M(0,0)` en variables locales
3. Al salir del proc, el scope local se destruye
4. `$M(0,0)` no existe en el scope global

#### ✅ Con la Modificación Propuesta

```bcl
SOURCE "lib/MATRIX.BLB"

PROC MAT_ZEROS WITH name rows cols DO
    GLOBAL M    # ← Marca M como global, incluye todos los M(*)
    FOR 0 TO [EXPR $rows - 1] DO
        SET r $__FOR
        FOR 0 TO [EXPR $cols - 1] DO
            SET c $__FOR
            SET $name($r,$c) 0    # ← Ahora crea variable GLOBAL
        END
    END
END

# Uso:
MAT_ZEROS M 3 3
PUTS $M(0,0)    # ← Imprime: 0 (existe globalmente)
```

**Cómo funciona:**
1. `GLOBAL M` detecta que no existe ningún `M` escalar
2. Como es posible que sea un array, marca el prefijo `"M("` en `global_prefixes`
3. Cuando `SET M(0,0) 0` se ejecuta, `is_global_var()` detecta que `"M("` está en prefijos
4. La variable `M(0,0)` se crea en `global_vars` en lugar de en el scope local
5. Al salir del proc, `M(0,0)` sigue existiendo

### Ejemplo 2: Librería WINDOW

#### ❌ Problema Actual

```bcl
SOURCE "lib/WINDOW.BLB"

PROC WIN_CREATE WITH id row col width height title DO
    SET win_id($id.row) $row        # ← Crea win_id(0.row) localmente
    SET win_id($id.col) $col        # ← Crea win_id(0.col) localmente
    SET win_id($id.width) $width
    SET win_id($id.height) $height
END

WIN_CREATE 0 10 20 40 15 "Test"
PUTS $win_id(0.row)    # ← ERROR: no existe (fue local)
```

#### ✅ Solución Actual (Workaround Manual)

```bcl
# Se debe crear el array ANTES de llamar a los procs
SET win_id(dummy) ""  # ← Forzar creación del array en global

SOURCE "lib/WINDOW.BLB"

PROC WIN_CREATE WITH id row col width height title DO
    # No hay GLOBAL, pero como win_id ya existe globalmente, funciona
    SET win_id($id.row) $row
    # ...
END

WIN_CREATE 0 10 20 40 15 "Test"
PUTS $win_id(0.row)    # Funciona, pero es un hack
```

#### ✅ Con la Modificación Propuesta

```bcl
SOURCE "lib/WINDOW.BLB"

PROC WIN_CREATE WITH id row col width height title DO
    GLOBAL win_id    # ← Marca win_id(*) como global automáticamente
    SET win_id($id.row) $row
    SET win_id($id.col) $col
    SET win_id($id.width) $width
    SET win_id($id.height) $height
END

WIN_CREATE 0 10 20 40 15 "Test"
PUTS $win_id(0.row)    # ← Imprime: 10 (perfecto!)
```

### Ejemplo 3: Múltiples Arrays en un Procedimiento

#### ❌ Problema Actual

```bcl
PROC MATRIX_MULTIPLY WITH A B C DO
    # Necesitamos acceder a arrays A, B y crear C
    # Sin GLOBAL para arrays, todos se crean localmente
    SET C(0,0) [EXPR $A(0,0) * $B(0,0)]    # ← Todos locales
END

SET A(0,0) 5
SET B(0,0) 3
MATRIX_MULTIPLY A B C
PUTS $C(0,0)    # ← ERROR: C(0,0) no existe
```

#### ✅ Con la Modificación Propuesta

```bcl
PROC MATRIX_MULTIPLY WITH A B C DO
    GLOBAL A B C    # ← Marca los tres arrays como globales
    SET C(0,0) [EXPR $A(0,0) * $B(0,0)]    # ← Ahora funcionan
END

SET A(0,0) 5
SET B(0,0) 3
MATRIX_MULTIPLY A B C
PUTS $C(0,0)    # ← Imprime: 15
```

## Casos Edge y Comportamiento Esperado

### Caso 1: GLOBAL de array no existente

```bcl
PROC CREATE_CONFIG DO
    GLOBAL config    # ← Array no existe aún
    SET config(host) "localhost"    # ← Crea globalmente
    SET config(port) 8080
END

CREATE_CONFIG
PUTS $config(host)    # ← Imprime: localhost
```

**Comportamiento:** Como `config` no existe como variable escalar, se asume que puede ser array y se marca el prefijo `"config("` como global.

### Caso 2: Conflicto entre variable escalar y array

```bcl
SET myvar "escalar"    # ← Variable escalar global

PROC TEST DO
    GLOBAL myvar
    SET myvar(key) "array"    # ← ¿Qué pasa?
END

TEST
```

**Comportamiento esperado:**
- `GLOBAL myvar` verifica si existe `myvar` escalar → SÍ existe
- Marca `myvar` (escalar) como global, NO marca prefijo `"myvar("`
- Dentro del proc, `SET myvar(key) "array"` crea `myvar(key)` LOCALMENTE
- Después del proc, `$myvar` sigue siendo "escalar", `$myvar(key)` no existe

**Razón:** Evitar ambigüedad. Si ya existe como escalar, no se trata como array.

### Caso 3: Array parcialmente creado

```bcl
SET data(x) 10    # ← Crea data(x) globalmente

PROC UPDATE DO
    GLOBAL data    # ← Detecta que data(...) existe
    SET data(y) 20    # ← Añade globalmente
    SET data(z) 30
END

UPDATE
PUTS "$data(x) $data(y) $data(z)"    # ← Imprime: 10 20 30
```

**Comportamiento:** `GLOBAL data` busca cualquier variable con patrón `data(*)`. Si encuentra al menos una, marca el prefijo como global.

### Caso 4: Nombres con puntos (como win_id)

```bcl
PROC CREATE DO
    GLOBAL win_id
    SET win_id(0.row) 10    # ← El índice contiene punto
END

CREATE
PUTS $win_id(0.row)    # ← Imprime: 10
```

**Comportamiento:** Funciona perfectamente. El prefijo es `"win_id("`, y `"0.row"` es simplemente el índice (puede contener cualquier carácter excepto `)`).

## Diagrama de Flujo: Decisión de Scope

```
┌─────────────────────────────────┐
│  SET varname value              │
└────────────┬────────────────────┘
             │
             v
    ┌────────────────┐
    │ En scope local?│
    └───┬────────┬───┘
        NO       YES
        │        │
        v        v
    Global   ┌──────────────────────┐
    Vars     │ is_global_var(name)? │
             └──┬────────────────┬──┘
                YES              NO
                │                │
                v                v
            Global Vars      Local Vars


is_global_var(scope, "myarray(foo)"):
  1. ¿Existe "myarray(foo)" en global_refs?
     NO → Continuar
  2. ¿El nombre contiene '('?
     SÍ → Extraer prefijo: "myarray("
  3. ¿Existe "myarray(" en global_prefixes?
     SÍ → RETURN true (ES GLOBAL)
     NO → RETURN false (ES LOCAL)
```

## Implementación del Algoritmo de Detección en GLOBAL

```
GLOBAL varname:
  1. Buscar en global_vars si existe varname(*)
     Método: Iterar hash, buscar claves que empiecen con "varname("

  2. SI se encuentra al menos una:
     → Marcar "varname(" en global_prefixes
     → (Esto hace que TODAS las futuras variables varname(*)
        se creen como globales)

  3. SI NO se encuentra ninguna:
     → Verificar si existe variable escalar "varname"
     → SI existe: Marcar "varname" en global_refs (variable escalar)
     → SI NO existe: Asumir que será array, marcar "varname(" en global_prefixes

  4. Esto permite tanto:
     - GLOBAL config   (antes de crear el array)
     - GLOBAL myarray  (después de que myarray(x) ya existe)
```

## Tests de Validación

### Test 1: Array creado después de GLOBAL

```bcl
#!/usr/bin/env bcl

PROC CREATE DO
    GLOBAL cfg
    SET cfg(a) 1
    SET cfg(b) 2
END

CREATE
SET result [EXPR $cfg(a) + $cfg(b)]
IF [EXPR $result == 3] THEN
    PUTS "PASS: Array global antes de creación"
ELSE
    PUTS "FAIL: Esperado 3, obtenido $result"
END
```

### Test 2: Array creado antes de GLOBAL

```bcl
#!/usr/bin/env bcl

SET arr(x) 10

PROC MODIFY DO
    GLOBAL arr
    SET arr(y) 20
    INCR arr(x)
END

MODIFY
IF [EXPR $arr(x) == 11 && $arr(y) == 20] THEN
    PUTS "PASS: GLOBAL detecta array existente"
ELSE
    PUTS "FAIL: arr(x)=$arr(x), arr(y)=$arr(y)"
END
```

### Test 3: Múltiples arrays

```bcl
#!/usr/bin/env bcl

PROC MULTI DO
    GLOBAL A B C
    SET A(0) 1
    SET B(0) 2
    SET C(0) [EXPR $A(0) + $B(0)]
END

MULTI
IF [EXPR $C(0) == 3] THEN
    PUTS "PASS: Múltiples arrays globales"
ELSE
    PUTS "FAIL: Esperado 3, obtenido $C(0)"
END
```

### Test 4: Anidamiento de procs

```bcl
#!/usr/bin/env bcl

PROC OUTER DO
    GLOBAL data
    SET data(outer) 1

    PROC INNER DO
        GLOBAL data
        SET data(inner) 2
        INCR data(outer)
    END

    INNER
END

OUTER
IF [EXPR $data(outer) == 2 && $data(inner) == 2] THEN
    PUTS "PASS: Arrays en procs anidados"
ELSE
    PUTS "FAIL: outer=$data(outer), inner=$data(inner)"
END
```

## Impacto en Librerías Existentes

### ANSI.BLB
**Sin cambios** - No usa arrays internos

### WINDOW.BLB
**Mejora significativa** - Requiere añadir `GLOBAL win_id` una vez al inicio

```diff
PROC WIN_CREATE WITH id row col width height title DO
+   GLOBAL win_id
    SET win_id($id.row) $row
    SET win_id($id.col) $col
    # ...
END
```

### Nueva MATRIX.BLB (ahora posible)

```bcl
################################################################################
# MATRIX.BLB - MATLAB-Style Matrix Operations
################################################################################

# Metadatos almacenados como: _mat(M.rows), _mat(M.cols)
# Elementos almacenados como: M(0,0), M(0,1), etc.

PROC MAT_CREATE WITH name rows cols DO
    GLOBAL _mat
    # Usar nombre como clave para metadatos
    SET _mat($name.rows) $rows
    SET _mat($name.cols) $cols
END

PROC MAT_ZEROS WITH name rows cols DO
    GLOBAL _mat
    EVAL "GLOBAL $name"    # ← Marcar el array de datos como global
    MAT_CREATE $name $rows $cols

    FOR 0 TO [EXPR $rows - 1] DO
        SET r $__FOR
        FOR 0 TO [EXPR $cols - 1] DO
            SET c $__FOR
            EVAL "SET ${name}($r,$c) 0"
        END
    END
END

PROC MAT_PRINT WITH name DO
    GLOBAL _mat
    EVAL "GLOBAL $name"

    SET rows $_mat($name.rows)
    SET cols $_mat($name.cols)

    PUTS "Matrix $name ($rows x $cols):"
    FOR 0 TO [EXPR $rows - 1] DO
        SET r $__FOR
        PUTS -NONEWLINE "  ["
        FOR 0 TO [EXPR $cols - 1] DO
            SET c $__FOR
            EVAL "SET val \$${name}($r,$c)"
            PUTS -NONEWLINE [FORMAT " %8.4f" $val]
        END
        PUTS " ]"
    END
END

# Uso:
MAT_ZEROS I 3 3
MAT_PRINT I
# Imprime:
# Matrix I (3 x 3):
#   [  0.0000  0.0000  0.0000 ]
#   [  0.0000  0.0000  0.0000 ]
#   [  0.0000  0.0000  0.0000 ]
```

## Conclusión

La modificación propuesta soluciona completamente los problemas identificados con un cambio mínimo en el código (aproximadamente 100 líneas) y sin romper compatibilidad con código existente.

**Beneficios clave:**
1. ✅ GLOBAL funciona con arrays
2. ✅ Librerías pueden usar arrays compartidos
3. ✅ No requiere workarounds ni hacks
4. ✅ Comportamiento intuitivo y predecible
5. ✅ 100% compatible con código existente
