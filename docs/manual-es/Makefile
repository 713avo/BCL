# Makefile para el Manual de BCL (Español)

.PHONY: all clean pdf view help

# Objetivo por defecto
all: pdf

# Construir el PDF (compilación completa con índice)
pdf:
	@echo "Compilando Manual de BCL (Español)..."
	@pdflatex -interaction=nonstopmode main.tex > /dev/null
	@makeindex main.idx > /dev/null 2>&1
	@pdflatex -interaction=nonstopmode main.tex > /dev/null
	@pdflatex -interaction=nonstopmode main.tex > /dev/null
	@echo "¡Listo! PDF creado: main.pdf"

# Compilación rápida (un solo pase, sin índice)
quick:
	@echo "Compilación rápida..."
	@pdflatex -interaction=nonstopmode main.tex > /dev/null
	@echo "¡Listo! PDF creado: main.pdf"

# Ver el PDF
view: pdf
	@if command -v xdg-open > /dev/null; then \
		xdg-open main.pdf; \
	elif command -v open > /dev/null; then \
		open main.pdf; \
	else \
		echo "Por favor abre main.pdf manualmente"; \
	fi

# Limpiar archivos temporales
clean:
	@echo "Limpiando archivos temporales..."
	@rm -f *.aux *.idx *.ilg *.ind *.log *.lol *.lot *.out *.toc
	@echo "¡Listo!"

# Limpiar todo incluyendo el PDF
distclean: clean
	@echo "Eliminando PDF..."
	@rm -f main.pdf
	@echo "¡Listo!"

# Mostrar ayuda
help:
	@echo "Sistema de Compilación del Manual de BCL"
	@echo ""
	@echo "Objetivos:"
	@echo "  all        - Compilar el manual completo (por defecto)"
	@echo "  pdf        - Igual que 'all'"
	@echo "  quick      - Compilación rápida (un pase, sin índice)"
	@echo "  view       - Compilar y abrir el PDF"
	@echo "  clean      - Eliminar archivos temporales"
	@echo "  distclean  - Eliminar archivos temporales y PDF"
	@echo "  help       - Mostrar este mensaje de ayuda"
	@echo ""
	@echo "Uso:"
	@echo "  make           # Compilar el manual"
	@echo "  make clean     # Limpiar archivos temporales"
	@echo "  make view      # Compilar y ver"
