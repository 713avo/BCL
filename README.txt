================================================================================
 BCL - BASIC COMMAND LANGUAGE
================================================================================
Versión: 1.6.0
Fecha: Octubre 2025
Autor: Rafa
Licencia: (pendiente)
================================================================================

*** ¿QUÉ ES BCL? ***

BCL (Basic Command Language) es un lenguaje de scripting interpretado,
ligero y expresivo, inspirado en Tcl 8.x pero con una sintaxis más legible
tipo BASIC.

Diseñado para ser:
  - SIMPLE: Sintaxis clara, fácil de aprender
  - LIGERO: Sin dependencias externas, totalmente standalone
  - PORTABLE: Código C99 POSIX compatible
  - COMPLETO: 60 comandos cubriendo todas las necesidades básicas

Todo en BCL se maneja como texto (STRING). Las operaciones numéricas,
lógicas o de formato se evalúan dinámicamente.

*** CARACTERÍSTICAS PRINCIPALES ***

✓ 60 comandos implementados (100% del core)
✓ REPL interactivo con historial y navegación
✓ Sintaxis case-insensitive para comandos
✓ Sustitución de variables ($var) y comandos ([cmd])
✓ Expresiones matemáticas completas (30+ funciones)
✓ Control de flujo (IF, WHILE, FOR, FOREACH, SWITCH)
✓ Procedimientos con parámetros y recursión
✓ Listas y manipulación de strings
✓ Operaciones completas sobre archivos
✓ Expresiones regulares básicas (standalone)
✓ Manejo de tiempo y fechas (CLOCK)
✓ Interacción con el sistema (EXEC, ENV, ARGV)
✓ Manual completo de usuario (64 páginas PDF)

*** INSTALACIÓN ***

1. Requisitos:
   - Compilador GCC (C99)
   - Make
   - Sistema Unix/Linux/macOS

2. Compilación:

   make clean
   make release

3. El binario se genera en: bin/bcl

4. Ejecutar:

   # REPL interactivo
   ./bin/bcl

   # Ejecutar script
   ./bin/bcl mi_script.bcl

*** INICIO RÁPIDO ***

EJEMPLO 1: Hola Mundo
----------------------
PUTS "¡Hola, Mundo!"

EJEMPLO 2: Variables y Matemáticas
-----------------------------------
SET nombre "BCL"
PUTS "Bienvenido a $nombre"

SET a 10
SET b 20
SET suma [EXPR $a + $b]
PUTS "La suma es: $suma"

EJEMPLO 3: Bucles y Condiciones
--------------------------------
SET contador 1
WHILE [EXPR $contador <= 5] DO
    PUTS "Iteración $contador"
    INCR contador
END

IF [EXPR $suma > 25] THEN
    PUTS "La suma es mayor que 25"
ELSE
    PUTS "La suma es menor o igual a 25"
END

EJEMPLO 4: Procedimientos
--------------------------
PROC FACTORIAL WITH n DO
    IF [EXPR $n <= 1] THEN
        RETURN 1
    ELSE
        SET prev [FACTORIAL [EXPR $n - 1]]
        RETURN [EXPR $n * $prev]
    END
END

SET resultado [FACTORIAL 5]
PUTS "5! = $resultado"

EJEMPLO 5: Listas
-----------------
SET frutas [LIST "manzana" "naranja" "plátano"]
PUTS "Frutas: $frutas"
PUTS "Primera fruta: [LINDEX $frutas 0]"
PUTS "Número de frutas: [LLENGTH $frutas]"

LAPPEND frutas "uva"
PUTS "Ahora tenemos: $frutas"

EJEMPLO 6: Archivos
-------------------
# Escribir
SET fh [OPEN "test.txt" "W"]
PUTS $fh "Primera línea"
PUTS $fh "Segunda línea"
CLOSE $fh

# Leer
SET fh [OPEN "test.txt" "R"]
SET contenido [READ $fh]
CLOSE $fh
PUTS "Contenido: $contenido"

EJEMPLO 7: Tiempo
-----------------
SET ahora [CLOCK SECONDS]
SET fecha [CLOCK FORMAT $ahora FORMAT "%d/%m/%Y %H:%M:%S"]
PUTS "Fecha actual: $fecha"

SET manana [CLOCK ADD $ahora 1 day]
PUTS "Mañana: [CLOCK FORMAT $manana FORMAT "%Y-%m-%d"]"

*** ESTRUCTURA DEL PROYECTO ***

BCL/
├── bin/              Binario compilado (bcl)
├── src/              Código fuente (20 archivos .c)
├── include/          Headers (.h)
├── def/              Especificaciones de comandos
├── docs/             Documentación
│   ├── BCL_Manual.tex   Manual LaTeX (2500+ líneas)
│   └── BCL_Manual.pdf   Manual PDF (64 páginas)
├── examples/         Ejemplos de código BCL
├── Makefile          Sistema de compilación
├── ChangeLog.txt     Historial de cambios
└── README.txt        Este archivo

*** COMANDOS IMPLEMENTADOS ***

CATEGORÍA: VARIABLES Y DATOS
-----------------------------
SET, UNSET, INCR, APPEND, GLOBAL

CATEGORÍA: I/O
--------------
PUTS, PUTSN, GETS

CATEGORÍA: EXPRESIONES
----------------------
EXPR (con 30+ funciones matemáticas)

CATEGORÍA: CONTROL DE FLUJO
----------------------------
IF, WHILE, FOR, FOREACH, SWITCH, BREAK, CONTINUE, RETURN, EXIT

CATEGORÍA: PROCEDIMIENTOS
--------------------------
PROC

CATEGORÍA: LISTAS
-----------------
LIST, SPLIT, JOIN, LINDEX, LRANGE, LLENGTH, LAPPEND, LINSERT,
LREPLACE, CONCAT, LSORT, LSEARCH

CATEGORÍA: STRINGS
------------------
STRING (20+ subcomandos)

CATEGORÍA: FORMATEO
-------------------
FORMAT, SCAN

CATEGORÍA: ARCHIVOS
-------------------
OPEN, CLOSE, READ, TELL, SEEK, EOF, PWD, FILE (4 subcomandos), GLOB

CATEGORÍA: EXPRESIONES REGULARES
---------------------------------
REGEXP, REGSUB (implementación básica standalone)

CATEGORÍA: TIEMPO
-----------------
CLOCK (6 subcomandos)

CATEGORÍA: SISTEMA
------------------
EXEC, ENV, ARGV, EVAL, SOURCE, AFTER

CATEGORÍA: INTROSPECCIÓN
-------------------------
INFO (7 subcomandos)

*** DOCUMENTACIÓN ***

Manual completo de usuario disponible en:
  docs/BCL_Manual.pdf (64 páginas)

Para generar el PDF desde LaTeX:
  cd docs
  lualatex BCL_Manual.tex
  lualatex BCL_Manual.tex
  lualatex BCL_Manual.tex

Especificaciones de comandos en:
  def/*.txt (14 archivos de especificación)

*** EJEMPLOS ***

Ejemplos completos en el directorio examples/:
  - demo.bcl              Demostración general
  - demo_clock.bcl        Demostración de CLOCK
  - BCL_file_test.bcl     Test de operaciones de archivos

*** DESARROLLO ***

COMPILACIÓN EN MODO DEBUG:
--------------------------
make debug

LIMPIEZA:
---------
make clean

INFORMACIÓN DEL BUILD:
----------------------
make info

TEST BÁSICO:
------------
make test

*** DIFERENCIAS CON TCL ***

BCL está inspirado en Tcl pero tiene diferencias importantes:

SIMILITUDES:
- Todo es string
- Sustitución de variables con $
- Sustitución de comandos con []
- Listas como strings separadas por espacios

DIFERENCIAS:
- Sintaxis tipo BASIC (IF...THEN...END en lugar de if {...})
- Case-insensitive para comandos
- Bloques cerrados con END (no ENDIF, ENDWHILE, etc.)
- Sin namespaces
- Sin upvar/uplevel
- Sin manejo avanzado de errores (catch)
- Regexp básico (sin PCRE2)
- Sin soporte completo de timezone en CLOCK

*** ESTADÍSTICAS ***

- Líneas de código C: ~12,000
- Archivos fuente: 20
- Comandos: 60
- Funciones matemáticas: 30+
- Ejemplos en manual: 50+
- Páginas de documentación: 64
- Tamaño del binario: ~180 KB (optimizado)
- Sin dependencias externas: 100% standalone

*** RENDIMIENTO ***

BCL está diseñado para ser ligero y eficiente:
- Arranque instantáneo
- Consumo mínimo de memoria
- Adecuado para scripts pequeños a medianos
- REPL interactivo responsivo

*** CASOS DE USO ***

BCL es ideal para:
✓ Scripts de automatización
✓ Procesamiento de archivos de texto
✓ Prototipos rápidos
✓ Herramientas de línea de comandos
✓ Educación en programación (sintaxis simple)
✓ Sistemas embebidos (sin dependencias)
✓ Calculadora avanzada con scripting

NO recomendado para:
✗ Aplicaciones de alto rendimiento
✗ Procesamiento masivo de datos
✗ Aplicaciones web complejas
✗ Parsing complejo con regex (usar PCRE2)

*** VERSIONES ***

v1.6.0 (2025-10-21) - REGEXP/REGSUB standalone, Manual completo
v1.5.1 (2025-10-21) - CLOCK completo
v1.5.0 (2025-10-21) - REPL reescrito, comandos de sistema
v1.0.0 (2025-10-20) - Implementación inicial

Ver ChangeLog.txt para detalles completos.

*** FUTURO (BCL 2.0) ***

Características consideradas para futuras versiones:
- REGEXP/REGSUB con PCRE2 completo
- Soporte completo de timezone (IANA database)
- Módulo de red (sockets)
- Serialización JSON/XML
- Soporte Unicode/UTF-8 completo
- Diccionarios nativos
- Manejo de excepciones (try/catch)

*** CONTRIBUIR ***

(Información pendiente sobre cómo contribuir al proyecto)

*** LICENCIA ***

(Información de licencia pendiente)

*** CONTACTO ***

Autor: Rafa
Proyecto: BCL - Basic Command Language
Fecha: Octubre 2025

*** AGRADECIMIENTOS ***

- Inspirado en Tcl 8.x de John Ousterhout
- Sintaxis influenciada por BASIC
- Filosofía de simplicidad de Unix

================================================================================
 COMANDOS RÁPIDOS DE REFERENCIA
================================================================================

# Variables
SET var valor             # Asignar variable
PUTS $var                 # Mostrar variable
INCR var [incremento]     # Incrementar
APPEND var texto          # Concatenar

# Matemáticas
EXPR 2 + 2                # Calcular: 4
EXPR sqrt(16)             # Raíz cuadrada: 4
EXPR sin(3.14159)         # Seno: ≈0

# Control
IF condición THEN ... END
WHILE condición DO ... END
FOR var FROM inicio TO fin DO ... END
FOREACH var IN lista DO ... END

# Listas
SET lista [LIST a b c]
LINDEX $lista 0           # Primer elemento
LLENGTH $lista            # Longitud
LAPPEND lista d           # Añadir elemento

# Archivos
SET f [OPEN "file.txt" "R"]
SET data [READ $f]
CLOSE $f

# Tiempo
CLOCK SECONDS             # Timestamp actual
CLOCK FORMAT [CLOCK SECONDS] FORMAT "%Y-%m-%d"

# Sistema
EXEC "ls -la"             # Ejecutar comando
ENV HOME                  # Variable de entorno
SOURCE "script.bcl"       # Cargar script

================================================================================
¡Gracias por usar BCL!
================================================================================
