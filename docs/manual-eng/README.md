# BCL Manual (English) - Modular LaTeX Structure

This directory contains the modular LaTeX source for the BCL User Manual in English.

## Structure

The manual is organized into separate `.tex` files for easier maintenance:

### Core Files
- `main.tex` - Main document that includes all other files
- `preamble.tex` - Package configuration and custom commands
- `frontmatter.tex` - Title page, copyright, and table of contents

### Chapters
- `ch01_introduction.tex` - Introduction to BCL
- `ch02_fundamentals.tex` - Programming Fundamentals
- `ch03_variables.tex` - Variables and Data
- `ch04_expressions.tex` - Expressions and Math
- `ch05_control.tex` - Control Structures
- `ch06_procedures.tex` - Procedures (Functions)
- `ch07_lists.tex` - Lists
- `ch08_strings.tex` - String Manipulation
- `ch09_files.tex` - File Operations
- `ch10_regexp.tex` - Regular Expressions
- `ch11_time.tex` - Time and Date
- `ch12_system.tex` - System Interaction
- `ch13_introspection.tex` - Introspection
- `ch14_examples.tex` - Complete Examples
- `ch15_reference.tex` - Command Reference

### Back Matter
- `appendices.tex` - Installation, Troubleshooting, etc.

## Compilation

To compile the manual to PDF:

```bash
cd docs/manual-eng
make         # Complete build with index
make quick   # Quick build (single pass)
make clean   # Clean temporary files
make view    # Build and open PDF
```

Or manually:
```bash
cd docs/manual-eng
pdflatex main.tex && makeindex main.idx && pdflatex main.tex && pdflatex main.tex
```

The output will be `main.pdf`.

## Advantages of Modular Structure

1. **Easy Maintenance**: Edit individual chapters without touching others
2. **Version Control**: Clear git diffs show which chapters changed
3. **Collaboration**: Multiple authors can work on different chapters
4. **Organization**: Logical separation of content
5. **Reusability**: Can include/exclude chapters as needed

## Adding a New Chapter

1. Create `chXX_name.tex` in this directory
2. Add `\input{chXX_name.tex}` to `main.tex` in the appropriate location
3. Recompile the manual

## Notes

- All chapter files use relative paths
- The preamble defines custom commands like `\cmd{}`, `\var{}`, `\file{}`
- Custom colored boxes: `examplebox`, `tipbox`, `warningbox`, `notebox`
- BCL language syntax highlighting is pre-configured
