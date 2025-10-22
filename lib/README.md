# BCL Standard Library

This directory contains standard libraries for BCL (Basic Command Language) written in pure BCL.

## Available Libraries

### ANSI.BLB - ANSI Terminal Control

Complete library for terminal control using ANSI escape sequences.

**Features:**
- Screen and line clearing
- Cursor positioning and control (move, save/restore, show/hide)
- Text colors (16 standard + 16 bright colors)
- Text styles (bold, dim, italic, underline, blink, reverse, etc.)
- RGB colors (256-color mode)
- High-level functions (boxes, progress bars, banners)

**Usage:**
```bcl
SOURCE "lib/ANSI.BLB"

ANSI_CLEAR
ANSI_PRINT_COLOR $ANSI_FG_GREEN "Success!"
ANSI_BOX 5 10 40 8 "My Box"
ANSI_PROGRESS_BAR 20 10 50 75
ANSI_CLEANUP
```

**Documentation:** See [ANSI.md](ANSI.md) for complete API reference.

**Examples:**
- `examples/color_demo.bcl` - Demonstrates all colors and styles
- `examples/dashboard.bcl` - Interactive system dashboard

---

## Using Libraries

To use a library in your BCL scripts, use the `SOURCE` command:

```bcl
SOURCE "lib/LIBRARY_NAME.BLB"
```

The `.BLB` extension stands for "BCL Library".

## Creating Your Own Libraries

Libraries should:
1. Be written in pure BCL (no external dependencies)
2. Use `.BLB` extension
3. Include header comments with version, description, usage
4. Define procedures with `PROC ... END`
5. Use descriptive, UPPERCASE names for procedures
6. Provide accompanying `.md` documentation

Example library structure:

```bcl
################################################################################
# MY_LIB.BLB - My Custom Library
################################################################################
# Version: 1.0.0
# Description: Brief description of what the library does
# Usage: SOURCE "lib/MY_LIB.BLB"
################################################################################

# Constants
SET MY_CONSTANT "value"

# Procedures
PROC MY_FUNCTION WITH param1 param2 DO
    # Implementation
    RETURN $result
END

################################################################################
# END OF MY_LIB.BLB
################################################################################
```

---

## License

Part of BCL (Basic Command Language) project.
