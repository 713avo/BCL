# Manual de BCL (Español) - Estructura Modular en LaTeX

Este directorio contiene el código fuente LaTeX modular del Manual de Usuario de BCL en español.

## Estructura

El manual está organizado en archivos `.tex` separados para facilitar el mantenimiento:

### Archivos Principales
- `main.tex` - Documento principal que incluye todos los demás archivos
- `preamble.tex` - Configuración de paquetes y comandos personalizados
- `frontmatter.tex` - Página de título, copyright y tabla de contenidos

### Capítulos
- `ch01_introduccion.tex` - Introducción a BCL
- `ch02_fundamentos.tex` - Fundamentos de Programación
- `ch03_variables.tex` - Variables y Datos
- `ch04_expresiones.tex` - Expresiones y Matemáticas
- `ch05_control.tex` - Estructuras de Control
- `ch06_procedimientos.tex` - Procedimientos (Funciones)
- `ch07_listas.tex` - Listas
- `ch08_cadenas.tex` - Manipulación de Cadenas
- `ch09_archivos.tex` - Operaciones con Archivos
- `ch10_regexp.tex` - Expresiones Regulares
- `ch11_tiempo.tex` - Tiempo y Fecha
- `ch12_sistema.tex` - Interacción con el Sistema
- `ch13_introspeccion.tex` - Introspección
- `ch14_ejemplos.tex` - Ejemplos Completos
- `ch15_referencia.tex` - Referencia de Comandos

### Material Final
- `apendices.tex` - Instalación, Solución de Problemas, etc.

## Compilación

Para compilar el manual a PDF:

```bash
cd docs/manual-es
make         # Compilación completa con índice
make quick   # Compilación rápida (un solo pase)
make clean   # Limpiar archivos temporales
make view    # Compilar y abrir PDF
```

O manualmente:
```bash
cd docs/manual-es
pdflatex main.tex && makeindex main.idx && pdflatex main.tex && pdflatex main.tex
```

La salida será `main.pdf`.

## Ventajas de la Estructura Modular

1. **Fácil Mantenimiento**: Editar capítulos individuales sin tocar otros
2. **Control de Versiones**: Los diffs de git muestran claramente qué capítulos cambiaron
3. **Colaboración**: Múltiples autores pueden trabajar en diferentes capítulos
4. **Organización**: Separación lógica del contenido
5. **Reutilización**: Se pueden incluir/excluir capítulos según sea necesario

## Agregar un Nuevo Capítulo

1. Crear `chXX_nombre.tex` en este directorio
2. Agregar `\input{chXX_nombre.tex}` a `main.tex` en la ubicación apropiada
3. Recompilar el manual

## Notas

- Todos los archivos de capítulos usan rutas relativas
- El preámbulo define comandos personalizados como `\cmd{}`, `\var{}`, `\file{}`
- Cajas coloreadas personalizadas: `examplebox`, `tipbox`, `warningbox`, `notebox`
- El resaltado de sintaxis del lenguaje BCL está preconfigurado

## Traducción

Este manual es una traducción completa al español del manual original en inglés. Se ha mantenido:
- La estructura modular exacta
- El formato LaTeX
- Los ejemplos de código BCL (sin traducir)
- La terminología técnica apropiada en español
