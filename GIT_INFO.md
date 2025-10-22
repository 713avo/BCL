# BCL Interpreter - Git Repository Information

## Repository Details

**Location:** `/home/moneyland/woark/Program/BCL/claude`
**Initial Commit:** `a5d07a6` - Initial commit: BCL Interpreter v1.5.1
**Date:** Wed Oct 22 14:09:17 2025 +0200
**Branch:** master

## Project Statistics

- **Total Files:** 142
- **Total Lines:** 40,904
- **Source Code Files:** 23 (`.c` files)
- **Header Files:** 2 (`.h` files)
- **Documentation Files:** 42 (LaTeX `.tex` files)
- **Specification Files:** 17 (`.txt` files)

## Repository Structure

```
claude/
├── .git/                      # Git repository
├── .gitignore                 # Git ignore rules
├── src/                       # Source code (23 files)
│   ├── bcl_*.c               # BCL implementation modules
│   └── main.c                # Main entry point
├── include/                   # Headers (2 files)
│   ├── bcl.h                 # Core definitions
│   └── bcl_commands.h        # Command declarations
├── docs/                      # Documentation
│   ├── manual-eng/           # English manual (131 pages)
│   └── manual-es/            # Spanish manual (132 pages)
├── def/                       # Specifications (17 files)
├── examples/                  # Example scripts
├── Makefile                   # Build system
└── README.txt                # Project README
```

## Documentation

### Manuals (PDF)
- **English:** `docs/manual-eng/main.pdf` (131 pages, 17 chapters)
- **Spanish:** `docs/manual-es/main.pdf` (132 pages, 17 chapters)

### Specifications
Complete specifications available in `def/` directory:
- BCL_Variables_v1.4.txt
- BCL_Control_v1.1.txt
- BCL_Procedimientos_v1.0.txt
- BCL_Listas_v1.1.txt
- BCL_STRING_Referencia_v1.1_PC.txt
- BCL_ARRAY_v1.0.txt
- BCL_BINARY_v1.0.txt
- And more...

## Key Features Committed

### Core Language
- Variables (SET, UNSET, INCR, APPEND, GLOBAL)
- Control flow (IF, WHILE, FOR, FOREACH, SWITCH)
- Procedures (PROC with parameters, local/global scope)
- Expressions (EXPR with full operator support)

### Data Structures
- Lists (12+ commands: LIST, SPLIT, JOIN, LINDEX, etc.)
- Strings (20+ subcommands via STRING)
- **Arrays** (associative arrays with Tcl syntax)
- **Binary data** (BINARY FORMAT/SCAN)

### System Integration
- File I/O (OPEN, CLOSE, READ, FILE)
- Regular expressions (REGEXP, REGSUB)
- Time operations (CLOCK)
- Introspection (INFO)
- System commands (EVAL, SOURCE, EXEC)

## Build System

```bash
# Build release version
make release

# Build debug version
make debug

# Run tests
make test

# Clean build
make clean
```

## Version Control Commands

```bash
# View commit history
git log

# View file changes
git diff

# View specific file history
git log --follow src/bcl_array.c

# View commit statistics
git show --stat HEAD
```

## Recent Implementations

The initial commit includes two major new features:

### 1. ARRAY Command (Chapter 16)
- ARRAY EXISTS, SIZE, NAMES, GET, SET, UNSET
- Tcl-compatible associative array syntax
- Pattern matching with glob wildcards
- Full documentation in both languages

### 2. BINARY Command (Chapter 17)
- BINARY FORMAT - pack binary data
- BINARY SCAN - unpack binary data
- Support for multiple data types (8/16/32-bit integers, strings, hex)
- Little-endian and big-endian byte ordering
- Complete documentation with practical examples

## Contributors

- Primary Author: BCL Development Team
- Co-Authored-By: Claude (Anthropic)

## License

(To be determined by project owner)

## Next Steps

To continue development:

1. Clone or pull the repository
2. Make changes in a new branch
3. Test thoroughly
4. Commit with descriptive messages
5. Merge back to master

## Contact

For questions or contributions, contact the project maintainer.

---

*Generated: October 22, 2025*
*BCL Interpreter Version: 1.5.1*
