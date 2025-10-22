# Makefile for BCL Manual

.PHONY: all clean pdf view help

# Default target
all: pdf

# Build the PDF (full compilation with index)
pdf:
	@echo "Building BCL Manual..."
	@pdflatex -interaction=nonstopmode main.tex > /dev/null
	@makeindex main.idx > /dev/null 2>&1
	@pdflatex -interaction=nonstopmode main.tex > /dev/null
	@pdflatex -interaction=nonstopmode main.tex > /dev/null
	@echo "Done! PDF created: main.pdf"

# Quick build (single pass, no index)
quick:
	@echo "Quick build..."
	@pdflatex -interaction=nonstopmode main.tex > /dev/null
	@echo "Done! PDF created: main.pdf"

# View the PDF
view: pdf
	@if command -v xdg-open > /dev/null; then \
		xdg-open main.pdf; \
	elif command -v open > /dev/null; then \
		open main.pdf; \
	else \
		echo "Please open main.pdf manually"; \
	fi

# Clean temporary files
clean:
	@echo "Cleaning temporary files..."
	@rm -f *.aux *.idx *.ilg *.ind *.log *.lol *.lot *.out *.toc
	@echo "Done!"

# Clean everything including PDF
distclean: clean
	@echo "Removing PDF..."
	@rm -f main.pdf
	@echo "Done!"

# Show help
help:
	@echo "BCL Manual Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all        - Build the complete manual (default)"
	@echo "  pdf        - Same as 'all'"
	@echo "  quick      - Quick build (single pass, no index)"
	@echo "  view       - Build and open the PDF"
	@echo "  clean      - Remove temporary files"
	@echo "  distclean  - Remove temporary files and PDF"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Usage:"
	@echo "  make           # Build the manual"
	@echo "  make clean     # Clean temporary files"
	@echo "  make view      # Build and view"
