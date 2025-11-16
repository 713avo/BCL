#######################################################################
# Makefile - BCL Interpreter (Standalone, sin librerías externas)
#######################################################################

# Nombre del proyecto
PROJECT = bcl
VERSION = 1.5.1

# Directorios
SRC_DIR = src
INC_DIR = include
BIN_DIR = bin
EXAMPLE_DIR = examples

# Compilador y flags
CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -I$(INC_DIR)
CFLAGS += -D_POSIX_C_SOURCE=200112L
LDFLAGS = -lm -ldl

# Flags de optimización (por defecto)
OPT_FLAGS = -O2

# Flags de depuración
DEBUG_FLAGS = -g3 -O0 -DDEBUG

# Archivos fuente
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:.c=.o)
DEPS = $(OBJECTS:.o=.d)

# Ejecutable
TARGET = $(BIN_DIR)/$(PROJECT)

#######################################################################
# REGLAS PRINCIPALES
#######################################################################

.PHONY: all clean debug release test test-all run help dirs

# Por defecto: release
all: release

# Compilación optimizada
release: CFLAGS += $(OPT_FLAGS)
release: dirs $(TARGET)
	@echo "✓ Build completo (release): $(TARGET)"

# Compilación con debug
debug: CFLAGS += $(DEBUG_FLAGS)
debug: dirs $(TARGET)
	@echo "✓ Build completo (debug): $(TARGET)"

# Crear directorios
dirs:
	@mkdir -p $(BIN_DIR)

# Ejecutable principal
$(TARGET): $(OBJECTS)
	@echo "Enlazando $@..."
	@$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compilar archivos objeto (en src/)
$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compilando $<..."
	@$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# Incluir dependencias
-include $(DEPS)

#######################################################################
# TESTING
#######################################################################

# Ejecutar el intérprete
run: release
	@echo "Iniciando BCL REPL..."
	@$(TARGET)

# Ejecutar con script de ejemplo
run-example: release
	@if [ -f $(EXAMPLE_DIR)/demo.bcl ]; then \
		echo "Ejecutando $(EXAMPLE_DIR)/demo.bcl..."; \
		$(TARGET) $(EXAMPLE_DIR)/demo.bcl; \
	else \
		echo "Error: No existe $(EXAMPLE_DIR)/demo.bcl"; \
	fi

# Test simple
test: release
	@echo "════════════════════════════════════════════════════════════"
	@echo " Testing BCL Interpreter"
	@echo "════════════════════════════════════════════════════════════"
	@mkdir -p $(EXAMPLE_DIR)
	@echo 'SET nombre "Rafa"' > $(EXAMPLE_DIR)/_test.bcl
	@echo 'PUTS "Hola $$nombre"' >> $(EXAMPLE_DIR)/_test.bcl
	@echo 'SET a 5' >> $(EXAMPLE_DIR)/_test.bcl
	@echo 'SET b 10' >> $(EXAMPLE_DIR)/_test.bcl
	@echo 'INCR a 3' >> $(EXAMPLE_DIR)/_test.bcl
	@echo 'PUTS "a=$$a b=$$b"' >> $(EXAMPLE_DIR)/_test.bcl
	@echo 'APPEND msg "Hola" " " "mundo"' >> $(EXAMPLE_DIR)/_test.bcl
	@echo 'PUTS "msg=$$msg"' >> $(EXAMPLE_DIR)/_test.bcl
	@echo ""
	@echo "Ejecutando test básico..."
	@$(TARGET) $(EXAMPLE_DIR)/_test.bcl
	@rm -f $(EXAMPLE_DIR)/_test.bcl
	@echo ""
	@echo "✓ Test completado"

# Comprehensive test suite
test-all: release
	@echo "════════════════════════════════════════════════════════════"
	@echo " BCL Comprehensive Test Suite"
	@echo "════════════════════════════════════════════════════════════"
	@echo ""
	@echo "Running test_variables.bcl..."
	@$(TARGET) tests/test_variables.bcl
	@echo ""
	@echo "Running test_control_flow.bcl..."
	@$(TARGET) tests/test_control_flow.bcl
	@echo ""
	@echo "Running test_expressions.bcl..."
	@$(TARGET) tests/test_expressions.bcl
	@echo ""
	@echo "Running test_lists.bcl..."
	@$(TARGET) tests/test_lists.bcl
	@echo ""
	@echo "Running test_strings.bcl..."
	@$(TARGET) tests/test_strings.bcl
	@echo ""
	@echo "Running test_arrays.bcl..."
	@$(TARGET) tests/test_arrays.bcl
	@echo ""
	@echo "Running test_procedures.bcl..."
	@$(TARGET) tests/test_procedures.bcl
	@echo ""
	@echo "Running test_binary.bcl..."
	@$(TARGET) tests/test_binary.bcl
	@echo ""
	@echo "Running test_format_scan.bcl..."
	@$(TARGET) tests/test_format_scan.bcl
	@echo ""
	@echo "Running test_regexp.bcl..."
	@$(TARGET) tests/test_regexp.bcl
	@echo ""
	@echo "Running test_info_clock.bcl..."
	@$(TARGET) tests/test_info_clock.bcl
	@echo ""
	@echo "Running test_system.bcl..."
	@$(TARGET) tests/test_system.bcl
	@echo ""
	@echo "Running test_files.bcl..."
	@$(TARGET) tests/test_files.bcl
	@echo ""
	@echo "════════════════════════════════════════════════════════════"
	@echo " All test suites completed!"
	@echo "════════════════════════════════════════════════════════════"

#######################################################################
# LIMPIEZA
#######################################################################

clean:
	@echo "Limpiando..."
	@rm -f $(SRC_DIR)/*.o $(SRC_DIR)/*.d
	@rm -rf $(BIN_DIR)
	@rm -f $(EXAMPLE_DIR)/_test.bcl
	@echo "✓ Limpieza completa"

#######################################################################
# INFORMACIÓN
#######################################################################

info:
	@echo "════════════════════════════════════════════════════════════"
	@echo " Proyecto: $(PROJECT) v$(VERSION)"
	@echo "════════════════════════════════════════════════════════════"
	@echo " Compilador:    $(CC)"
	@echo " CFLAGS:        $(CFLAGS)"
	@echo " LDFLAGS:       $(LDFLAGS)"
	@echo " Fuentes:       $(words $(SOURCES)) archivos"
	@echo " Target:        $(TARGET)"
	@echo " Standalone:    SIN librerías externas (readline, pcre, utf8proc)"
	@echo "════════════════════════════════════════════════════════════"

help:
	@echo "════════════════════════════════════════════════════════════"
	@echo " BCL Interpreter - Makefile"
	@echo "════════════════════════════════════════════════════════════"
	@echo ""
	@echo "Comandos principales:"
	@echo "  make              - Compilar (release)"
	@echo "  make release      - Compilar optimizado"
	@echo "  make debug        - Compilar con símbolos debug"
	@echo "  make clean        - Limpiar archivos compilados"
	@echo ""
	@echo "Testing:"
	@echo "  make test         - Ejecutar test básico"
	@echo "  make test-all     - Ejecutar suite completa de tests"
	@echo "  make run          - Ejecutar REPL"
	@echo "  make run-example  - Ejecutar script demo"
	@echo ""
	@echo "Información:"
	@echo "  make info         - Mostrar info del proyecto"
	@echo "  make help         - Mostrar esta ayuda"
	@echo ""
	@echo "Nota: Este build NO requiere librerías externas"
	@echo "      (readline, pcre, utf8proc, etc.)"
	@echo ""
	@echo "════════════════════════════════════════════════════════════"

.DEFAULT_GOAL := help
