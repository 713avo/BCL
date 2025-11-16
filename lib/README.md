# BCL Standard Libraries

This directory contains the standard BCL libraries. Each library is self-documented with comprehensive headers and examples.

## Available Libraries

### ANSI.BLB - Terminal Control & Unicode Graphics

Terminal control with 60+ Unicode character constants.

```bcl
SOURCE "lib/ANSI.BLB"

ANSI_CLEAR                                    # Clear screen
ANSI_CURSOR_GOTO 10 20                       # Move cursor
ANSI_SET_COLOR $ANSI_FG_BRIGHT_CYAN $ANSI_BG_BLUE
```

**Features:** 16 colors, text styles, box drawing, Unicode symbols, cursor control

### WINDOW.BLB - Advanced Window Management

Terminal window management with global array persistence.

**Features:** Multiple windows, borders, colors, menus, progress bars, scrolling

### MATRIX.BLB - MATLAB-Style Matrix Operations

Complete matrix library with 21 functions.

**Functions:** Creation, arithmetic, statistics, utilities

### CALCULUS.BLB - Numerical Calculus

Complete numerical analysis library.

**Functions:** Derivatives, integration, root finding, differential equations, optimization

### ROMAN.BLB - Roman Numeral Conversion

Bidirectional Roman numeral conversion (1-3999).

**Features:** Conversion, validation, arithmetic, comparison, formatting

---

## Usage

```bcl
SOURCE "lib/LIBRARYNAME.BLB"
```

## Documentation

- Header comments in each .BLB file
- Man pages: `man 3 LIBRARYNAME.BLB`
- Tests: `tests/test_*.bcl`
- Examples: `examples/*_demo.bcl`

See individual .BLB files for complete documentation.
