# Makefile for BCL manual pages

# Installation directories
PREFIX ?= /usr/local
MANDIR = $(PREFIX)/man/man1
USER_MANDIR = $(HOME)/.local/share/man/man1

# Man pages to install
MAN_PAGES = bcl.1 \
            bcl-variables.1 \
            bcl-expr.1 \
            bcl-control.1 \
            bcl-proc.1 \
            bcl-list.1 \
            bcl-string.1 \
            bcl-array.1 \
            bcl-file.1 \
            bcl-fileops.1 \
            bcl-regexp.1 \
            bcl-clock.1 \
            bcl-system.1 \
            bcl-binary.1 \
            bcl-info.1 \
            bcl-format.1

.PHONY: all install install-user uninstall uninstall-user test clean help

all: help

help:
	@echo "BCL Manual Pages Installation"
	@echo "=============================="
	@echo ""
	@echo "Available targets:"
	@echo "  make install        - Install man pages system-wide (requires root)"
	@echo "  make install-user   - Install man pages for current user"
	@echo "  make uninstall      - Uninstall system-wide man pages"
	@echo "  make uninstall-user - Uninstall user man pages"
	@echo "  make test           - Test man page rendering"
	@echo "  make clean          - Clean generated files"
	@echo ""
	@echo "View man pages:"
	@echo "  man -M . bcl        - View without installing"
	@echo "  man bcl             - View after installation"
	@echo ""

# System-wide installation (requires root)
install:
	@echo "Installing BCL man pages to $(MANDIR)..."
	@mkdir -p $(MANDIR)
	@for man in $(MAN_PAGES); do \
		echo "  Installing $$man"; \
		install -m 644 $$man $(MANDIR)/$$man; \
	done
	@echo "Updating man database..."
	@if command -v mandb >/dev/null 2>&1; then \
		mandb -q || true; \
	fi
	@echo "✓ Installation complete"
	@echo ""
	@echo "Try: man bcl"

# User installation (no root required)
install-user:
	@echo "Installing BCL man pages to $(USER_MANDIR)..."
	@mkdir -p $(USER_MANDIR)
	@for man in $(MAN_PAGES); do \
		echo "  Installing $$man"; \
		install -m 644 $$man $(USER_MANDIR)/$$man; \
	done
	@echo "✓ Installation complete"
	@echo ""
	@echo "Add to your shell rc file (~/.bashrc or ~/.zshrc):"
	@echo "  export MANPATH=\"\$$MANPATH:$(HOME)/.local/share/man\""
	@echo ""
	@echo "Then: man bcl"

# System-wide uninstall
uninstall:
	@echo "Uninstalling BCL man pages from $(MANDIR)..."
	@for man in $(MAN_PAGES); do \
		if [ -f $(MANDIR)/$$man ]; then \
			echo "  Removing $$man"; \
			rm -f $(MANDIR)/$$man; \
		fi \
	done
	@echo "Updating man database..."
	@if command -v mandb >/dev/null 2>&1; then \
		mandb -q || true; \
	fi
	@echo "✓ Uninstallation complete"

# User uninstall
uninstall-user:
	@echo "Uninstalling BCL man pages from $(USER_MANDIR)..."
	@for man in $(MAN_PAGES); do \
		if [ -f $(USER_MANDIR)/$$man ]; then \
			echo "  Removing $$man"; \
			rm -f $(USER_MANDIR)/$$man; \
		fi \
	done
	@echo "✓ Uninstallation complete"

# Test man pages
test:
	@echo "Testing man page rendering..."
	@echo ""
	@for man in $(MAN_PAGES); do \
		echo "Testing $$man..."; \
		if groff -mandoc -Tascii $$man >/dev/null 2>&1; then \
			echo "  ✓ $$man is valid"; \
		else \
			echo "  ✗ $$man has errors"; \
		fi \
	done
	@echo ""
	@echo "View a specific page with: man -l bcl.1"

# Clean generated files (if any)
clean:
	@echo "Cleaning generated files..."
	@rm -f *.pdf *.html *.txt
	@echo "✓ Clean complete"

# Generate PDF versions (requires ps2pdf)
pdf:
	@echo "Generating PDF versions..."
	@mkdir -p pdf
	@for man in $(MAN_PAGES); do \
		base=$${man%.1}; \
		echo "  Generating $$base.pdf"; \
		man -t ./$$man | ps2pdf - pdf/$$base.pdf; \
	done
	@echo "✓ PDFs generated in pdf/ directory"

# Generate HTML versions (requires groff)
html:
	@echo "Generating HTML versions..."
	@mkdir -p html
	@for man in $(MAN_PAGES); do \
		base=$${man%.1}; \
		echo "  Generating $$base.html"; \
		groff -mandoc -Thtml $$man > html/$$base.html; \
	done
	@echo "✓ HTML files generated in html/ directory"

# Generate plain text versions
txt:
	@echo "Generating plain text versions..."
	@mkdir -p txt
	@for man in $(MAN_PAGES); do \
		base=$${man%.1}; \
		echo "  Generating $$base.txt"; \
		man -l $$man | col -b > txt/$$base.txt; \
	done
	@echo "✓ Text files generated in txt/ directory"

# List all man pages
list:
	@echo "BCL Manual Pages:"
	@echo ""
	@for man in $(MAN_PAGES); do \
		desc=$$(grep -A1 "^\.SH NAME" $$man | tail -n1 | sed 's/^[^ ]* \\- //'); \
		printf "  %-20s - %s\n" "$$man" "$$desc"; \
	done
