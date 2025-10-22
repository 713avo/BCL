# ANSI.BLB - ANSI Escape Sequences Library

**Version**: 1.0.0
**Date**: 2025-10-22
**Author**: BCL Development Team
**Language**: Pure BCL (Basic Command Language)

## Overview

ANSI.BLB is a comprehensive library for terminal control using ANSI escape sequences. It provides a pure BCL implementation of functions for:

- Screen and line clearing
- Cursor positioning and control
- Text colors (foreground and background)
- Text styles (bold, underline, etc.)
- High-level drawing functions (boxes, progress bars, banners)

## Installation

Simply include the library in your BCL scripts:

```bcl
SOURCE "lib/ANSI.BLB"
```

## Features

✅ **Pure BCL** - No external dependencies
✅ **Comprehensive** - 40+ functions and constants
✅ **Easy to use** - Simple, intuitive API
✅ **Well documented** - Complete reference below
✅ **Cross-platform** - Works on any ANSI-compatible terminal

---

## Quick Start

```bcl
# Load the library
SOURCE "lib/ANSI.BLB"

# Clear screen and print colored text
ANSI_CLEAR
ANSI_PRINT_COLOR $ANSI_FG_GREEN "Hello, World!"

# Draw a box
ANSI_BOX 5 10 40 10 "My Box"

# Show a progress bar
ANSI_PROGRESS_BAR 20 10 50 75

# Cleanup
ANSI_CLEANUP
```

---

## API Reference

### Constants

#### Foreground Colors (Text)

| Constant | Color | Code |
|----------|-------|------|
| `ANSI_FG_BLACK` | Black | 30 |
| `ANSI_FG_RED` | Red | 31 |
| `ANSI_FG_GREEN` | Green | 32 |
| `ANSI_FG_YELLOW` | Yellow | 33 |
| `ANSI_FG_BLUE` | Blue | 34 |
| `ANSI_FG_MAGENTA` | Magenta | 35 |
| `ANSI_FG_CYAN` | Cyan | 36 |
| `ANSI_FG_WHITE` | White | 37 |
| `ANSI_FG_DEFAULT` | Default | 39 |

#### Bright Foreground Colors

| Constant | Color | Code |
|----------|-------|------|
| `ANSI_FG_BRIGHT_BLACK` | Bright Black (Gray) | 90 |
| `ANSI_FG_BRIGHT_RED` | Bright Red | 91 |
| `ANSI_FG_BRIGHT_GREEN` | Bright Green | 92 |
| `ANSI_FG_BRIGHT_YELLOW` | Bright Yellow | 93 |
| `ANSI_FG_BRIGHT_BLUE` | Bright Blue | 94 |
| `ANSI_FG_BRIGHT_MAGENTA` | Bright Magenta | 95 |
| `ANSI_FG_BRIGHT_CYAN` | Bright Cyan | 96 |
| `ANSI_FG_BRIGHT_WHITE` | Bright White | 97 |

#### Background Colors

| Constant | Color | Code |
|----------|-------|------|
| `ANSI_BG_BLACK` | Black | 40 |
| `ANSI_BG_RED` | Red | 41 |
| `ANSI_BG_GREEN` | Green | 42 |
| `ANSI_BG_YELLOW` | Yellow | 43 |
| `ANSI_BG_BLUE` | Blue | 44 |
| `ANSI_BG_MAGENTA` | Magenta | 45 |
| `ANSI_BG_CYAN` | Cyan | 46 |
| `ANSI_BG_WHITE` | White | 47 |
| `ANSI_BG_DEFAULT` | Default | 49 |

#### Bright Background Colors

| Constant | Color | Code |
|----------|-------|------|
| `ANSI_BG_BRIGHT_BLACK` | Bright Black | 100 |
| `ANSI_BG_BRIGHT_RED` | Bright Red | 101 |
| `ANSI_BG_BRIGHT_GREEN` | Bright Green | 102 |
| `ANSI_BG_BRIGHT_YELLOW` | Bright Yellow | 103 |
| `ANSI_BG_BRIGHT_BLUE` | Bright Blue | 104 |
| `ANSI_BG_BRIGHT_MAGENTA` | Bright Magenta | 105 |
| `ANSI_BG_BRIGHT_CYAN` | Bright Cyan | 106 |
| `ANSI_BG_BRIGHT_WHITE` | Bright White | 107 |

#### Text Styles

| Constant | Style | Code |
|----------|-------|------|
| `ANSI_RESET` | Reset all | 0 |
| `ANSI_BOLD` | Bold | 1 |
| `ANSI_DIM` | Dim | 2 |
| `ANSI_ITALIC` | Italic | 3 |
| `ANSI_UNDERLINE` | Underline | 4 |
| `ANSI_BLINK` | Blink | 5 |
| `ANSI_REVERSE` | Reverse video | 7 |
| `ANSI_HIDDEN` | Hidden | 8 |
| `ANSI_STRIKETHROUGH` | Strikethrough | 9 |

---

### Screen Control Functions

#### `ANSI_CLEAR`
Clear the entire screen.

```bcl
ANSI_CLEAR
```

#### `ANSI_CLEAR_LINE`
Clear the current line.

```bcl
ANSI_CLEAR_LINE
```

#### `ANSI_CLEAR_TO_END`
Clear from cursor position to end of screen.

```bcl
ANSI_CLEAR_TO_END
```

#### `ANSI_CLEAR_TO_START`
Clear from cursor position to start of screen.

```bcl
ANSI_CLEAR_TO_START
```

#### `ANSI_CLEAR_LINE_TO_END`
Clear from cursor position to end of line.

```bcl
ANSI_CLEAR_LINE_TO_END
```

#### `ANSI_CLEAR_LINE_TO_START`
Clear from cursor position to start of line.

```bcl
ANSI_CLEAR_LINE_TO_START
```

---

### Cursor Control Functions

#### `ANSI_CURSOR_HOME`
Move cursor to home position (1,1).

```bcl
ANSI_CURSOR_HOME
```

#### `ANSI_CURSOR_GOTO row col`
Move cursor to specific row and column (1-based).

```bcl
ANSI_CURSOR_GOTO 10 20
```

**Parameters:**
- `row` - Row number (1-based)
- `col` - Column number (1-based)

#### `ANSI_CURSOR_UP n`
Move cursor up N lines.

```bcl
ANSI_CURSOR_UP 5
```

#### `ANSI_CURSOR_DOWN n`
Move cursor down N lines.

```bcl
ANSI_CURSOR_DOWN 3
```

#### `ANSI_CURSOR_RIGHT n`
Move cursor right N columns.

```bcl
ANSI_CURSOR_RIGHT 10
```

#### `ANSI_CURSOR_LEFT n`
Move cursor left N columns.

```bcl
ANSI_CURSOR_LEFT 5
```

#### `ANSI_CURSOR_SAVE`
Save current cursor position.

```bcl
ANSI_CURSOR_SAVE
```

#### `ANSI_CURSOR_RESTORE`
Restore previously saved cursor position.

```bcl
ANSI_CURSOR_RESTORE
```

#### `ANSI_CURSOR_HIDE`
Hide the cursor.

```bcl
ANSI_CURSOR_HIDE
```

#### `ANSI_CURSOR_SHOW`
Show the cursor.

```bcl
ANSI_CURSOR_SHOW
```

---

### Color and Style Functions

#### `ANSI_RESET`
Reset all text attributes to default.

```bcl
ANSI_RESET
```

#### `ANSI_SET_FG color`
Set foreground (text) color.

```bcl
ANSI_SET_FG $ANSI_FG_RED
PUTS "Red text"
ANSI_RESET
```

**Parameters:**
- `color` - Color code (use ANSI_FG_* constants)

#### `ANSI_SET_BG color`
Set background color.

```bcl
ANSI_SET_BG $ANSI_BG_BLUE
PUTS "Blue background"
ANSI_RESET
```

**Parameters:**
- `color` - Color code (use ANSI_BG_* constants)

#### `ANSI_SET_STYLE style`
Set text style.

```bcl
ANSI_SET_STYLE $ANSI_BOLD
PUTS "Bold text"
ANSI_RESET
```

**Parameters:**
- `style` - Style code (use ANSI_* style constants)

#### `ANSI_SET_COLOR fg bg`
Set both foreground and background colors.

```bcl
ANSI_SET_COLOR $ANSI_FG_WHITE $ANSI_BG_RED
PUTS "White on red"
ANSI_RESET
```

**Parameters:**
- `fg` - Foreground color code
- `bg` - Background color code

#### `ANSI_SET_RGB_FG r g b`
Set foreground color using RGB values (256-color mode).

```bcl
ANSI_SET_RGB_FG 255 100 50
PUTS "Custom RGB color"
ANSI_RESET
```

**Parameters:**
- `r` - Red value (0-255)
- `g` - Green value (0-255)
- `b` - Blue value (0-255)

#### `ANSI_SET_RGB_BG r g b`
Set background color using RGB values (256-color mode).

```bcl
ANSI_SET_RGB_BG 50 100 150
PUTS "Custom background"
ANSI_RESET
```

**Parameters:**
- `r` - Red value (0-255)
- `g` - Green value (0-255)
- `b` - Blue value (0-255)

---

### High-Level Convenience Functions

#### `ANSI_PRINT_COLOR color text`
Print text with specified foreground color (auto-reset).

```bcl
ANSI_PRINT_COLOR $ANSI_FG_GREEN "Success!"
```

**Parameters:**
- `color` - Foreground color code
- `text` - Text to print

#### `ANSI_PRINT_COLORED fg bg text`
Print text with specified foreground and background colors (auto-reset).

```bcl
ANSI_PRINT_COLORED $ANSI_FG_BLACK $ANSI_BG_YELLOW "Warning!"
```

**Parameters:**
- `fg` - Foreground color code
- `bg` - Background color code
- `text` - Text to print

#### `ANSI_PRINT_BOLD text`
Print bold text (auto-reset).

```bcl
ANSI_PRINT_BOLD "Important message"
```

**Parameters:**
- `text` - Text to print

#### `ANSI_PRINT_UNDERLINE text`
Print underlined text (auto-reset).

```bcl
ANSI_PRINT_UNDERLINE "Underlined text"
```

**Parameters:**
- `text` - Text to print

#### `ANSI_BOX row col width height title`
Draw a box at specified position with optional title.

```bcl
ANSI_BOX 5 10 40 8 "Status"
```

**Parameters:**
- `row` - Starting row (1-based)
- `col` - Starting column (1-based)
- `width` - Box width in characters
- `height` - Box height in characters
- `title` - Optional title (empty string for no title)

#### `ANSI_PROGRESS_BAR row col width percent`
Draw a progress bar.

```bcl
ANSI_PROGRESS_BAR 10 5 50 65
```

**Parameters:**
- `row` - Row position (1-based)
- `col` - Column position (1-based)
- `width` - Total width of progress bar
- `percent` - Progress percentage (0-100)

#### `ANSI_BANNER text`
Print a centered banner with styling.

```bcl
ANSI_BANNER "Welcome to BCL"
```

**Parameters:**
- `text` - Banner text

---

### Utility Functions

#### `ANSI_SLEEP ms`
Simple delay (uses AFTER command).

```bcl
ANSI_SLEEP 1000
```

**Parameters:**
- `ms` - Milliseconds to sleep

#### `ANSI_INIT`
Initialize screen for ANSI drawing (clear, hide cursor).

```bcl
ANSI_INIT
```

#### `ANSI_CLEANUP`
Cleanup and restore normal terminal (reset, show cursor).

```bcl
ANSI_CLEANUP
```

---

## Complete Examples

### Example 1: Colored Messages

```bcl
SOURCE "lib/ANSI.BLB"

ANSI_PRINT_COLOR $ANSI_FG_GREEN "✓ Success: Operation completed"
ANSI_PRINT_COLOR $ANSI_FG_YELLOW "⚠ Warning: Low disk space"
ANSI_PRINT_COLOR $ANSI_FG_RED "✗ Error: Connection failed"
ANSI_PRINT_BOLD "Important: Read the documentation"
```

### Example 2: Progress Bar Animation

```bcl
SOURCE "lib/ANSI.BLB"

ANSI_CLEAR
ANSI_BANNER "Installing BCL Libraries"

SET percent 0
WHILE [EXPR $percent <= 100] DO
    ANSI_PROGRESS_BAR 10 10 60 $percent
    ANSI_SLEEP 50
    SET percent [EXPR $percent + 2]
END

ANSI_CURSOR_GOTO 12 10
ANSI_PRINT_COLOR $ANSI_FG_GREEN "Installation complete!"
```

### Example 3: Dashboard Layout

```bcl
SOURCE "lib/ANSI.BLB"

ANSI_INIT

# Header
ANSI_BANNER "System Monitor"

# Boxes
ANSI_BOX 5 5 35 8 "CPU Usage"
ANSI_BOX 5 45 35 8 "Memory"
ANSI_BOX 15 5 75 10 "Process List"

# Content
ANSI_CURSOR_GOTO 7 10
ANSI_PRINT_COLOR $ANSI_FG_GREEN "CPU: 45%"

ANSI_CURSOR_GOTO 7 50
ANSI_PRINT_COLOR $ANSI_FG_YELLOW "RAM: 8.2 / 16 GB"

ANSI_CURSOR_GOTO 30 1
ANSI_CLEANUP
```

---

## Compatibility

This library works with any ANSI-compatible terminal:

- ✅ Linux terminals (xterm, gnome-terminal, konsole)
- ✅ macOS Terminal.app and iTerm2
- ✅ Windows Terminal, PowerShell 7+
- ✅ VSCode integrated terminal
- ⚠️ Windows Command Prompt (limited support, enable virtual terminal processing)

---

## Notes

- All row and column positions are **1-based** (first position is 1, not 0)
- Always call `ANSI_RESET` or `ANSI_CLEANUP` when done to restore terminal state
- Some terminals may not support all features (RGB colors, styles)
- The library uses `PUTSN` (no newline) for most output to allow precise positioning

---

## License

Part of BCL (Basic Command Language) project.
License: Pending

---

## Version History

- **1.0.0** (2025-10-22) - Initial release
  - 40+ functions and constants
  - Screen, cursor, color control
  - High-level drawing functions
  - Complete documentation

---

**For more BCL libraries and documentation, visit the BCL repository.**
