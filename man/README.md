# BCL Manual Pages

This directory contains Unix-style manual pages (man pages) for the BCL interpreter and all its commands.

## Contents

### Main Manual
- **bcl.1** - BCL interpreter main manual page

### Command Categories
- **bcl-variables.1** - Variable commands (SET, UNSET, INCR, APPEND, GLOBAL)
- **bcl-expr.1** - Expression evaluation and math functions
- **bcl-control.1** - Control flow (IF, WHILE, FOR, FOREACH, SWITCH, BREAK, CONTINUE, RETURN, EXIT)
- **bcl-proc.1** - Procedure definition and invocation
- **bcl-list.1** - List manipulation commands
- **bcl-string.1** - String operations and manipulation
- **bcl-array.1** - Associative array operations
- **bcl-file.1** - File I/O commands (OPEN, CLOSE, READ, GETS, PUTS, etc.)
- **bcl-fileops.1** - File system operations (FILE, PWD, GLOB)
- **bcl-regexp.1** - Regular expression pattern matching
- **bcl-clock.1** - Time and date operations
- **bcl-system.1** - System interaction (EXEC, ENV, ARGV, EVAL, SOURCE, AFTER)
- **bcl-binary.1** - Binary data packing and unpacking
- **bcl-info.1** - Introspection commands
- **bcl-format.1** - String formatting (FORMAT, SCAN)

## Installation

### System-Wide Installation (Requires root)

```bash
sudo make install
```

This installs all man pages to `/usr/local/man/man1/`.

### User Installation

```bash
make install-user
```

This installs man pages to `~/.local/share/man/man1/` and adds it to your MANPATH.

### Temporary Use (No Installation)

```bash
man -M . bcl
man -M . bcl-variables
```

Or set MANPATH temporarily:

```bash
export MANPATH=$MANPATH:$(pwd)
man bcl
```

## Viewing Man Pages

After installation:

```bash
man bcl                 # Main BCL manual
man bcl-variables       # Variable commands
man bcl-expr            # Expression evaluation
man bcl-control         # Control flow
man bcl-list            # List commands
man bcl-string          # String commands
man bcl-array           # Array commands
man bcl-file            # File I/O
man bcl-fileops         # File operations
man bcl-regexp          # Regular expressions
man bcl-clock           # Time and date
man bcl-system          # System commands
man bcl-binary          # Binary data
man bcl-info            # Introspection
man bcl-format          # Formatting
man bcl-proc            # Procedures
```

## Converting to Other Formats

### Generate PDF

```bash
man -t bcl | ps2pdf - bcl.pdf
```

### Generate HTML

```bash
man2html bcl.1 > bcl.html
```

Or using groff:

```bash
groff -mandoc -Thtml bcl.1 > bcl.html
```

### Generate Plain Text

```bash
man bcl | col -b > bcl.txt
```

## Uninstallation

### System-Wide

```bash
sudo make uninstall
```

### User Installation

```bash
make uninstall-user
```

## Man Page Format

All man pages are written in groff/troff format with the following sections:

- **NAME** - Command name and brief description
- **SYNOPSIS** - Command syntax
- **DESCRIPTION** - Detailed description
- **OPTIONS/COMMANDS** - Command options or subcommands
- **EXAMPLES** - Usage examples
- **NOTES** - Important notes and limitations
- **SEE ALSO** - Related man pages

## Testing Man Pages

To test a man page before installation:

```bash
man -l bcl.1
```

Or:

```bash
groff -mandoc -Tascii bcl.1 | less
```

## Contributing

When adding or modifying man pages:

1. Follow the existing format and structure
2. Use groff/troff markup consistently
3. Include practical examples
4. Document all options and behaviors
5. Test rendering with `man -l` before committing
6. Update this README if adding new pages

## Man Page Sections

BCL man pages are in section 1 (user commands):
- Section 1: User commands
- `.1` extension indicates section 1

## License

Same license as BCL interpreter (pending).

## Authors

BCL Development Team
- Rafa
- Co-authored by Claude (Anthropic)
