# BCL Documentation

This directory contains documentation for the BCL (Basic Command Language) project.

## Contents

### manual-eng/
English version of the BCL User Manual in modular LaTeX format.

- **Format:** LaTeX (modular structure with separate chapter files)
- **Output:** PDF (main.pdf)
- **Pages:** 113
- **Version:** 1.5.1

### manual-es/
Spanish version of the BCL User Manual (complete translation).

- **Format:** LaTeX (modular structure with separate chapter files)
- **Output:** PDF (main.pdf)
- **Pages:** 114
- **Version:** 1.5.1

The manual is organized into 15 chapters covering:
1. Introduction to BCL
2. Programming Fundamentals
3. Variables and Data
4. Expressions and Math
5. Control Structures
6. Procedures (Functions)
7. Lists
8. String Manipulation
9. File Operations
10. Regular Expressions
11. Time and Date
12. System Interaction
13. Introspection
14. Complete Examples
15. Command Reference

Plus appendices for installation, troubleshooting, and more.

**To build the English manual:**
```bash
cd manual-eng
make
```

**To build the Spanish manual:**
```bash
cd manual-es
make
```

See `manual-eng/README.md` and `manual-es/README.md` for detailed documentation on the modular structure and compilation options.

## Future Plans

- **api/** - API reference documentation (planned)
- **tutorials/** - Step-by-step tutorials (planned)

## Contributing

When adding documentation:
1. Follow the existing modular structure
2. Use meaningful chapter/section names
3. Include practical examples
4. Test all code examples
5. Update relevant README files

## Build Requirements

To build the LaTeX manuals, you need:
- pdflatex (TeX Live or similar)
- makeindex (for index generation)
- Standard LaTeX packages (see manual-eng/preamble.tex)

On Debian/Ubuntu:
```bash
sudo apt-get install texlive-latex-base texlive-latex-extra texlive-fonts-recommended
```

On Arch Linux:
```bash
sudo pacman -S texlive-core texlive-latexextra
```
