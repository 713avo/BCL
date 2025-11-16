# Changelog

All notable changes to the BCL (Basic Control Language) project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.0.0] - 2025-11-16

### Added

#### Core Features
- **Event System**: Complete event-driven programming system with `EVENT` command
  - `EVENT CREATE`: Register I/O events (READABLE, WRITABLE, EXCEPTION)
  - `EVENT DELETE`: Unregister specific events
  - `EVENT TIMER`: Create timer events with millisecond precision
  - `EVENT PROCESS`: Process events once with timeout
  - `EVENT LOOP`: Run event loop indefinitely
  - `EVENT STOP`: Stop event loop gracefully
  - `EVENT INFO`: List all registered events
  - Hybrid architecture: POSIX select() core with extensible backends
  - Callbacks receive FD/handle as parameters for clean API
  - Integration with SOCKET extension for network programming

- **Dynamic Extension System**: Load compiled extensions at runtime
  - `LOAD` command: Load shared object (.so) extensions
  - Extension API for C developers
  - Standardized extension initialization and command registration
  - SOCKET extension as reference implementation

- **Array System Improvements**:
  - Global variable prefix system for array persistence across procedures
  - Improved GLOBAL declarations for arrays
  - Better array access patterns in libraries

#### Standard Libraries

- **ROMAN.BLB**: Complete Roman numeral conversion library
  - `DECIMAL_TO_ROMAN`: Convert decimal (1-3999) to Roman numerals
  - `ROMAN_TO_DECIMAL`: Convert Roman numerals to decimal
  - `ROMAN_VALIDATE`: Validate canonical Roman numeral format
  - `ROMAN_ADD`, `ROMAN_SUB`, `ROMAN_MULTIPLY`, `ROMAN_DIVIDE`: Arithmetic operations
  - `ROMAN_COMPARE`: Compare Roman numerals
  - `ROMAN_RANGE`: Generate Roman numeral sequences
  - `ROMAN_FORMAT`: Format with upper/lower/title case
  - `ROMAN_TABLE`: Display conversion tables
  - Supports all subtraction rules (IV, IX, XL, XC, CD, CM)
  - Range: I to MMMCMXCIX (1 to 3999)

- **CALCULUS.BLB**: Numerical analysis and calculus library
  - Derivatives: Forward, backward, and central difference methods
  - Integration: Trapezoidal, Simpson's rule, and adaptive quadrature
  - Root finding: Bisection, Newton-Raphson, secant methods
  - Differential equations: Euler, RK4 (Runge-Kutta 4th order)
  - Polynomial operations: Evaluation, differentiation
  - Numerical optimization: Golden section search

- **ANSI.BLB v2.0.0**: Enhanced terminal control
  - 60+ Unicode character constants (box drawing, arrows, symbols)
  - 16 foreground and 16 background colors
  - Text styles (bold, italic, underline, blink, etc.)
  - Cursor control and screen management
  - Progress bars and spinners
  - Full UTF-8 support

- **WINDOW.BLB**: Advanced terminal window management
  - Multiple window support with z-ordering
  - Three border styles (single, double, rounded)
  - Configurable colors for content and borders
  - Window operations: create, destroy, show, hide, move
  - Content: text printing, centering, progress bars
  - Menus and buttons
  - Scrollable content buffering
  - Message boxes and dialogs

- **MATRIX.BLB**: MATLAB-style matrix operations
  - 21 matrix functions
  - Matrix creation: zeros, ones, eye, random, from list
  - Operations: add, subtract, multiply, element-wise multiply
  - Utilities: transpose, trace, determinant (2x2)
  - Statistics: sum, mean, min, max
  - Row and column extraction
  - Pretty printing with alignment

#### Extensions

- **SOCKET Extension**: Network programming support
  - `SOCKET CREATE`: Create TCP/UDP sockets
  - `SOCKET BIND`: Bind socket to address/port
  - `SOCKET LISTEN`: Listen for connections
  - `SOCKET ACCEPT`: Accept incoming connections
  - `SOCKET CONNECT`: Connect to remote host
  - `SOCKET SEND`: Send data
  - `SOCKET RECV`: Receive data
  - `SOCKET CLOSE`: Close socket
  - `SOCKET GETOPT`, `SOCKET SETOPT`: Socket options
  - Integrated with EVENT system for non-blocking I/O
  - IPv4 and IPv6 support

#### Documentation

- **Event System Documentation** (docs/EVENT_SYSTEM.md):
  - Complete architecture explanation
  - All EVENT subcommands with examples
  - 4 practical examples (echo server, interactive client, timers, file monitoring)
  - Integration guides for libraries
  - Extension development guide
  - Troubleshooting section

- **Extension System Documentation** (docs/EXTENSION_SYSTEM.pdf):
  - Complete LaTeX manual for extension development
  - API reference with all hooks and functions
  - Step-by-step extension creation guide
  - Best practices and patterns
  - SOCKET extension as case study

- **Language Reference** (docs/BCL_LANGUAGE_REFERENCE.md):
  - LLM-optimized comprehensive reference
  - All 64+ commands documented
  - Complete syntax and examples
  - Variable scoping rules
  - Expression evaluation

- **Manual Pages**: Unix-style man pages for all commands
  - Section 1: User commands and BCL interpreter
  - Section 3: Library functions (ANSI, MATRIX, WINDOW, CALCULUS, ROMAN)
  - Section 7: BCL language overview
  - Installed in standard man page format

- **LaTeX Documentation**:
  - Complete BCL system manual
  - Extension development guide
  - PDF generation with proper UTF-8 support
  - Professional formatting

#### Testing

- **Event System Tests**:
  - test_events_simple.bcl: 7 basic tests (85%+ passing)
  - test_events_comprehensive.bcl: 17 exhaustive tests
  - Covers timers, I/O events, global variables, precision, error handling

- **ROMAN Library Tests**:
  - test_roman.bcl: 10 test suites, 37 assertions
  - 100% pass rate
  - Tests conversions, validation, arithmetic, formatting
  - Bidirectional conversion verification (100 round-trips)

#### Examples

- examples/roman_demo.bcl: Complete ROMAN library demonstration
- Multiple event system examples in documentation
- Socket client/server examples

### Changed

- **Procedure Parameters**: All standard libraries now use correct `PROC name WITH params DO` syntax
  - ANSI.BLB: All procedures verified
  - CALCULUS.BLB: All procedures verified
  - MATRIX.BLB: All procedures verified
  - WINDOW.BLB: All procedures verified

- **README.md**: Major restructuring
  - Added event system features
  - Added dynamic extensions section
  - Updated statistics (13,500 LOC, 200KB binary)
  - Updated project structure (26 .c files, extensions directory)
  - Comprehensive examples for all major features

### Fixed

- **CLOCK FORMAT**: Fixed strftime syntax compatibility
- **CLOCK SCAN**: Fixed %s format specifier for Unix timestamps
- **CLOCK ADD**: Added unit requirement for time arithmetic
- **INFO LOCALS**: Implemented local variable introspection
- **Array Access**: Fixed global array persistence in procedures
- **Unicode Support**: Complete UTF-8 handling in all libraries
- **LaTeX Compilation**: Fixed encoding issues in PDF generation

### Removed

- Proposal files for implemented features (PROPOSAL_EVENT_SYSTEM.md)
- Obsolete test files and debugging scripts

### Technical Notes

#### Known BCL EXPR Limitations (Workarounds Applied)

1. **Negative Number Addition**: `EXPR -1 + 5` returns 6 (incorrect)
   - Workaround: Use reversed order `EXPR 5 + -1` (correct: 4)
   - Applied in: ROMAN_TO_DECIMAL

2. **Negative Number Comparison**: `EXPR $a < 0` fails when $a is negative
   - Workaround: Reverse comparison `EXPR 0 > $a`
   - Applied in: test_roman.bcl

#### Performance Notes

- Event system uses efficient select() polling
- Timer precision: millisecond accuracy (±10% variance acceptable)
- Array operations optimized with global prefix system
- Extension loading has minimal overhead (<1ms)

## [1.0.0] - Initial Release

### Added
- Basic BCL interpreter
- Core command set
- List and string operations
- File I/O
- Process control
- Basic networking (deprecated in v2.0, use SOCKET extension)

---

For more details about specific features, see:
- Event System: docs/EVENT_SYSTEM.md
- Extension Development: docs/EXTENSION_SYSTEM.pdf
- Language Reference: docs/BCL_LANGUAGE_REFERENCE.md
- Library Documentation: lib/*.BLB header comments
