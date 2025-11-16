# BCL - Basic Command Language

[![Version](https://img.shields.io/badge/version-2.0.0-blue.svg)](https://github.com/yourusername/bcl)
[![Language](https://img.shields.io/badge/language-C99-green.svg)](https://en.wikipedia.org/wiki/C99)
[![License](https://img.shields.io/badge/license-pending-lightgrey.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS-lightgrey.svg)](README.md)

**BCL (Basic Command Language)** is a lightweight, standalone scripting language interpreter inspired by Tcl 8.x but with a more readable BASIC-like syntax. Designed to be simple, portable, and complete with zero external dependencies.

## What's New in v2.0

🎉 **Array Global Support** - Arrays now work correctly with GLOBAL in procedures
🎨 **Full Unicode Support** - UTF-8 escape sequences (`\uXXXX`, `\UXXXXXXXX`) for box drawing and symbols
📊 **MATRIX Library** - New MATLAB-style matrix operations library with 21 functions
🖼️ **Enhanced WINDOW Library** - Complete rewrite with Unicode borders, menus, progress bars, and scrolling
✨ **Enhanced ANSI Library** - 60+ Unicode character constants for terminal graphics
⚡ **Event System** - Asynchronous I/O and timers with TCL-style fileevent support
🔌 **Dynamic Extensions** - Load binary modules at runtime with LOAD command
🌐 **SOCKET Extension** - TCP client/server networking (loadable extension)

## Features

✅ **Simple & Readable** - BASIC-like syntax, easy to learn
✅ **Standalone** - No external dependencies, fully self-contained
✅ **Portable** - Pure C99 POSIX-compatible code
✅ **Complete** - 64+ built-in commands covering all basic needs
✅ **Interactive REPL** - Command history and multi-line editing
✅ **Case-Insensitive** - Commands work in any case
✅ **Dynamic** - Everything is a string, evaluated dynamically
✅ **Unicode Support** - Full UTF-8 with escape sequences for symbols and box drawing (v2.0+)
✅ **Array Global System** - Arrays persist correctly across procedure boundaries (v2.0+)
✅ **Event-Driven** - Asynchronous I/O, timers, and event loop (v2.0+)
✅ **Extensible** - Dynamic module loading with stable API (v2.0+)
✅ **Standard Libraries** - MATRIX, WINDOW, ANSI, and CALCULUS libraries (v2.0+)
✅ **Documentation** - Complete manuals, man pages, and LLM-optimized reference

## Quick Start

### Installation

```bash
# Clone the repository
git clone https://github.com/yourusername/bcl.git
cd bcl

# Build the interpreter
make clean
make release

# Run the REPL
./bin/bcl

# Run a script
./bin/bcl script.bcl
```

### Requirements

- GCC (C99 compatible)
- Make
- Unix/Linux/macOS system
- No external libraries required

### Hello World

```bcl
PUTS "Hello, World!"
```

### Variables and Math

```bcl
SET name "BCL"
PUTS "Welcome to $name"

SET a 10
SET b 20
SET sum [EXPR $a + $b]
PUTS "Sum: $sum"
```

### Control Flow

```bcl
SET counter 1
WHILE [EXPR $counter <= 5] DO
    PUTS "Iteration $counter"
    INCR counter
END

IF [EXPR $sum > 25] THEN
    PUTS "Sum is greater than 25"
ELSE
    PUTS "Sum is less or equal to 25"
END
```

### Procedures

```bcl
PROC FACTORIAL WITH n DO
    IF [EXPR $n <= 1] THEN
        RETURN 1
    ELSE
        SET prev [FACTORIAL [EXPR $n - 1]]
        RETURN [EXPR $n * $prev]
    END
END

SET result [FACTORIAL 5]
PUTS "5! = $result"
```

### Associative Arrays

```bcl
SET config(host) "localhost"
SET config(port) 8080
SET config(timeout) 30

PUTS "Server: $config(host):$config(port)"
PUTS "Array size: [ARRAY SIZE config]"

FOREACH key IN [ARRAY NAMES config] DO
    PUTS "  $key = $config($key)"
END
```

### Binary Data

```bcl
# Pack binary data
SET data [BINARY FORMAT ccH8 65 66 "deadbeef"]
PUTS "Packed data length: [STRING LENGTH $data]"

# Unpack binary data
BINARY SCAN $data ccH8 byte1 byte2 hex
PUTS "Byte1: $byte1, Byte2: $byte2, Hex: $hex"
```

## Language Features

### Variables & Data
- `SET`, `UNSET`, `INCR`, `APPEND`, `GLOBAL`
- Variable substitution: `$var`, `${var}`
- Command substitution: `[command]`

### Control Flow
- `IF...THEN...ELSE...END`
- `WHILE...DO...END`
- `FOR...FROM...TO...DO...END`
- `FOREACH...IN...DO...END`
- `SWITCH...CASE...DEFAULT...END`
- `BREAK`, `CONTINUE`, `RETURN`, `EXIT`

### Expressions
- `EXPR` with 30+ math functions
- Operators: `+`, `-`, `*`, `/`, `%`, `**` (power)
- Comparisons: `==`, `!=`, `<`, `>`, `<=`, `>=`
- Logic: `&&`, `||`, `!`
- Functions: `sin`, `cos`, `sqrt`, `log`, `abs`, `min`, `max`, `rand`, etc.

### Lists
- `LIST`, `SPLIT`, `JOIN`
- `LINDEX`, `LRANGE`, `LLENGTH`
- `LAPPEND`, `LINSERT`, `LREPLACE`
- `CONCAT`, `LSORT`, `LSEARCH`

### Strings
- `STRING` with 20+ subcommands
- `LENGTH`, `INDEX`, `RANGE`, `MATCH`, `COMPARE`
- `TOUPPER`, `TOLOWER`, `TRIM`, `TRIMLEFT`, `TRIMRIGHT`
- `REPLACE`, `REPEAT`, `REVERSE`, `MAP`, etc.
- `FORMAT`, `SCAN` (printf/scanf style)

### Arrays (Associative)
- `ARRAY EXISTS`, `SIZE`, `NAMES`, `GET`, `SET`, `UNSET`
- Tcl-compatible syntax: `$array(index)`
- Pattern matching with glob wildcards

### Binary Data
- `BINARY FORMAT` - Pack data into binary format
- `BINARY SCAN` - Unpack binary data
- Format codes: `a`, `A`, `c`, `s`, `S`, `i`, `I`, `H`, `h`, `x`, `X`, `@`
- Endianness support (little/big endian)

### File I/O
- `OPEN`, `CLOSE`, `READ`, `PUTS`, `GETS`
- `TELL`, `SEEK`, `EOF`, `FLUSH`
- `FILE` operations: `exists`, `delete`, `rename`, `copy`, `size`, `type`, `mtime`
- `PWD`, `GLOB` (file pattern matching)

### Regular Expressions
- `REGEXP` - Basic pattern matching
- `REGSUB` - Pattern-based substitution
- Patterns: `.`, `*`, `+`, `?`, `^`, `$`
- Character classes: `\d`, `\w`, `\s` (and negated)
- Options: `NOCASE`, `ALL`, `MATCH`, `COUNT`

### Time & Date
- `CLOCK SECONDS` - Unix timestamp
- `CLOCK MILLISECONDS` - High precision time
- `CLOCK MICROSECONDS` - Microsecond precision
- `CLOCK FORMAT` - Format timestamps (strftime)
- `CLOCK SCAN` - Parse date strings (strptime)
- `CLOCK ADD` - Time arithmetic (add/subtract intervals)

### System Integration
- `EXEC` - Execute system commands
- `ENV` - Access environment variables
- `ARGV` - Command-line arguments
- `EVAL` - Dynamic code evaluation
- `SOURCE` - Load external scripts
- `AFTER` - Millisecond delays
- `LOAD` - Load dynamic extensions (.so files)

### Event System
- `EVENT CREATE` - Register I/O events (READABLE, WRITABLE, EXCEPTION)
- `EVENT DELETE` - Unregister events
- `EVENT TIMER` - Create timer events (millisecond precision)
- `EVENT PROCESS` - Process events once (with timeout)
- `EVENT LOOP` - Run event loop indefinitely
- `EVENT STOP` - Stop event loop
- `EVENT INFO` - List registered events

### Introspection
- `INFO EXISTS`, `COMMANDS`, `PROCS`, `VARS`, `LOCALS`, `GLOBALS`, `BODY`

## Project Structure

```
BCL/
├── src/                    # Source code (26 .c files)
│   ├── main.c             # Main entry point
│   ├── bcl_event.c        # Event system implementation (v2.0)
│   ├── bcl_extensions.c   # Dynamic extension loader (v2.0)
│   ├── bcl_*.c            # Core implementation modules
│   └── *.o, *.d           # Build artifacts (generated)
├── include/                # Header files
│   ├── bcl.h              # Core definitions
│   └── bcl_commands.h     # Command declarations
├── bin/                    # Compiled binary
│   └── bcl                # BCL interpreter (generated)
├── lib/                    # Standard libraries (v2.0+)
│   ├── ANSI.BLB           # Terminal control & Unicode graphics
│   ├── WINDOW.BLB         # Window management system
│   ├── MATRIX.BLB         # MATLAB-style matrix operations
│   ├── CALCULUS.BLB       # Numerical calculus library
│   └── ROMAN.BLB          # Roman numeral conversion (v2.0)
├── extensions/             # Dynamic extensions (v2.0+)
│   ├── socket.c           # SOCKET extension source
│   ├── socket.so          # Compiled extension (generated)
│   └── Makefile           # Extension build system
├── docs/                   # Documentation
│   ├── manual-eng/        # English manual (131 pages)
│   ├── manual-es/         # Spanish manual (132 pages)
│   ├── man_llm.md         # LLM-optimized reference (v2.0)
│   ├── EVENT_SYSTEM.md    # Event system guide (v2.0)
│   ├── extensions/        # Extension system guide
│   ├── PROPOSAL_*.md      # Design proposals
│   └── LICENSE.txt        # License information
├── man/                    # Unix manual pages (v1.6+)
│   ├── bcl.1              # Main manual page
│   └── bcl-*.1            # Command category pages
├── tests/                  # Test suite
│   ├── test_*.bcl         # Unit tests
│   ├── test_events*.bcl   # Event system tests (v2.0)
│   └── run_tests.sh       # Test runner
├── examples/               # Example BCL scripts
│   ├── matrix_demo.bcl    # MATRIX library demo (v2.0)
│   ├── window_demo_v2.bcl # WINDOW library demo (v2.0)
│   ├── socket_server.bcl  # TCP server example (v2.0)
│   ├── socket_client.bcl  # TCP client example (v2.0)
│   └── *.bcl              # Other examples
├── Makefile               # Build system
├── ChangeLog.txt          # Version history
├── README.md              # This file
└── .gitignore             # Git ignore rules
```

## Documentation

### Manuals (PDF)
- **English**: [docs/manual-eng/main.pdf](docs/manual-eng/main.pdf) - 131 pages, 17 chapters
- **Spanish**: [docs/manual-es/main.pdf](docs/manual-es/main.pdf) - 132 pages, 17 chapters

Both manuals include:
- Complete command reference
- 50+ practical examples
- Beginner-friendly tutorials
- Advanced topics (arrays, binary data, regex)

### Unix Man Pages
BCL includes comprehensive Unix-style manual pages:
```bash
man -l man/bcl.1              # Main BCL manual
man -l man/bcl-variables.1    # Variables and data
man -l man/bcl-control.1      # Control flow
# ... 15 category pages total
```

### LLM-Optimized Reference
For AI-assisted code generation:
- **[docs/man_llm.md](docs/man_llm.md)** - RAG-optimized complete language reference
- Includes all 62 commands with examples
- Standard libraries documentation (MATRIX, WINDOW, ANSI)
- Array global system explained
- Unicode escape sequences reference

### Building Manuals from Source

```bash
cd docs/manual-eng
make
# or manually:
pdflatex main.tex
makeindex main.idx
pdflatex main.tex
pdflatex main.tex
```

## Building

### Release Build (Optimized)
```bash
make release
```

### Debug Build
```bash
make debug
```

### Clean Build Artifacts
```bash
make clean
```

### Quick Build (Skip Dependencies)
```bash
make quick
```

### Run Tests
```bash
make test
```

## Performance & Statistics

- **Lines of code**: ~13,500
- **Source files**: 26 (.c files)
- **Commands**: 64+ (core) + extensible
- **Math functions**: 30+
- **Binary size**: ~200 KB (optimized)
- **Dependencies**: Zero external libraries
- **Startup time**: Instant
- **Memory footprint**: Minimal
- **Extensions**: Dynamic loading via LOAD

## Differences from Tcl

### Similarities
- Everything is a string
- Variable substitution with `$`
- Command substitution with `[]`
- Lists as space-separated strings

### Differences
- **BASIC-like syntax**: `IF...THEN...END` instead of `if {...}`
- **Case-insensitive commands**: `PUTS`, `puts`, `Puts` all work
- **Unified block endings**: All blocks end with `END`
- **No namespaces**: Simpler scope model
- **Standalone regex**: No PCRE dependency (limited features)
- **No advanced scope manipulation**: No `upvar`/`uplevel`
- **No exception handling**: No `catch`/`try`

## Use Cases

### Ideal For
✅ Automation scripts
✅ Text file processing
✅ Quick prototypes
✅ Command-line tools
✅ Educational purposes (simple syntax)
✅ Embedded systems (no dependencies)
✅ Advanced calculator with scripting
✅ Event-driven network servers
✅ Real-time data processing
✅ System monitoring and logging

### Not Recommended For
❌ High-performance applications
❌ Massive data processing
❌ Complex web applications
❌ Advanced regex parsing (use PCRE)

## Version History

- **v2.0.0** (2025-11-16) - Event system, dynamic extensions, LOAD command, SOCKET extension, CALCULUS library, array global system, Unicode support, MATRIX/WINDOW/ANSI libraries
- **v1.5.1** (2025-10-22) - ARRAY and BINARY commands, dual-language manuals
- **v1.6.0** (2025-10-21) - REGEXP/REGSUB standalone, Complete manual
- **v1.5.0** (2025-10-21) - REPL rewrite, system commands
- **v1.0.0** (2025-10-20) - Initial implementation

See [CHANGELOG.md](CHANGELOG.md) for complete version history.

## Standard Libraries

BCL includes five standard libraries for common programming tasks:

- **ANSI.BLB** - Terminal control with colors, styles, and Unicode graphics
- **WINDOW.BLB** - Terminal window management with menus and progress bars
- **MATRIX.BLB** - MATLAB-style matrix operations (21 functions)
- **CALCULUS.BLB** - Numerical analysis (derivatives, integration, root finding, ODEs)
- **ROMAN.BLB** - Roman numeral conversion and arithmetic (1-3999)

**Quick Example:**
```bcl
SOURCE "lib/MATRIX.BLB"
SOURCE "lib/ROMAN.BLB"

MAT_EYE I 3                         # Create 3x3 identity matrix
SET roman [DECIMAL_TO_ROMAN 2025]   # "MMXXV"
```

📖 **See [lib/README.md](lib/README.md) for complete library documentation**

## Event System

BCL v2.0 features a complete event-driven programming system for asynchronous I/O and timers:

```bcl
# Timer example
PROC ON_TIMER DO
    PUTS "Timer fired!"
END

EVENT TIMER 1000 ON_TIMER  # Fire after 1 second
EVENT PROCESS 2000         # Wait up to 2 seconds
```

**Features:**
- Asynchronous I/O (READABLE, WRITABLE, EXCEPTION)
- Millisecond-precision timers
- Callbacks with parameters
- Integrated with SOCKET extension for network programming

📖 **See [docs/EVENT_SYSTEM.md](docs/EVENT_SYSTEM.md) for complete event system documentation**

## Dynamic Extensions

Load compiled C extensions at runtime:

```bcl
LOAD "extensions/socket.so"

SET server [SOCKET SERVER 8080]
SET client [SOCKET ACCEPT $server]
SOCKET SEND $client "Hello, World!"
```

**Available Extensions:**
- **socket.so** - TCP/UDP networking with EVENT system integration

📖 **See [extensions/README.md](extensions/README.md) for extension usage**
📖 **See [docs/extensions/main.pdf](docs/extensions/main.pdf) for extension development guide**

## Future Plans

Potential features for future versions:
- Full PCRE2 regex support
- Complete timezone support (IANA database)
- ~~Network module (sockets)~~ ✅ DONE (v2.0 - SOCKET extension)
- JSON/XML serialization
- Native dictionaries
- Exception handling (try/catch)
- More extensions (HTTP, SQLite, JSON, GPIO for embedded)
- epoll/kqueue backends for high-performance event loops
- WebSocket support

## Contributing

Contributions are welcome! Please feel free to submit issues, fork the repository, and send pull requests.

## License

License information pending - to be determined by project owner.

## Author

**Rafa** - BCL Development Team
Co-Authored-By: Claude (Anthropic)

## Acknowledgments

- Inspired by **Tcl 8.x** by John Ousterhout
- Syntax influenced by **BASIC**
- Unix philosophy of simplicity

## Contact

For questions or contributions, please open an issue on GitHub.

---

**Thank you for using BCL!**

*Generated: November 16, 2025*
*BCL Interpreter Version: 2.0.0*
