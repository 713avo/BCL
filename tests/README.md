# BCL Test Suite

This directory contains comprehensive tests for all BCL commands.

## Test Organization

Tests are organized by command category:

- **test_variables.bcl** - SET, UNSET, INCR, APPEND, GLOBAL
- **test_control_flow.bcl** - IF, WHILE, FOR, FOREACH, SWITCH, BREAK, CONTINUE, RETURN
- **test_expressions.bcl** - EXPR command with arithmetic, logical, and math functions
- **test_lists.bcl** - LIST, SPLIT, JOIN, LINDEX, LRANGE, LLENGTH, LAPPEND, LINSERT, LREPLACE, CONCAT, LSORT, LSEARCH
- **test_strings.bcl** - STRING command with all subcommands
- **test_arrays.bcl** - ARRAY command (associative arrays)
- **test_procedures.bcl** - PROC command (procedures/functions)
- **test_binary.bcl** - BINARY FORMAT and BINARY SCAN
- **test_format_scan.bcl** - FORMAT and SCAN commands
- **test_regexp.bcl** - REGEXP and REGSUB
- **test_info_clock.bcl** - INFO and CLOCK commands
- **test_system.bcl** - EVAL, SOURCE, AFTER, EXEC, ENV, ARGV
- **test_files.bcl** - File I/O commands (OPEN, CLOSE, READ, GETS, PUTS, etc.)

## Running Tests

### Run all tests:
```bash
make test-all
```

### Run individual test:
```bash
./bin/bcl tests/test_variables.bcl
```

### Run master test suite:
```bash
./bin/bcl tests/run_all_tests.bcl
```

## Test Coverage

The test suite covers all 62 built-in BCL commands:

### Variables (5 commands)
- SET, UNSET, GLOBAL, INCR, APPEND

### I/O (3 commands)
- PUTS, PUTSN, GETS

### Control Flow (8 commands)
- IF, SWITCH, WHILE, FOR, FOREACH, BREAK, CONTINUE, EXIT

### Procedures (2 commands)
- PROC, RETURN

### Expressions (1 command)
- EXPR (with 30+ math functions)

### Arrays (1 command with subcommands)
- ARRAY (EXISTS, SIZE, NAMES, GET, SET, UNSET)

### Binary Data (1 command with subcommands)
- BINARY (FORMAT, SCAN)

### Lists (11 commands)
- LIST, SPLIT, JOIN, LINDEX, LRANGE, LLENGTH, LAPPEND, LINSERT, LREPLACE, CONCAT, LSORT, LSEARCH

### Strings (1 command with 20+ subcommands)
- STRING (LENGTH, INDEX, RANGE, MATCH, COMPARE, EQUAL, TOUPPER, TOLOWER, TOTITLE, TRIM, TRIMLEFT, TRIMRIGHT, REPLACE, REPEAT, REVERSE, MAP, CAT, FIRST, LAST, IS, WORDSTART, WORDEND)

### Format/Scan (2 commands)
- FORMAT, SCAN

### Files (11 commands)
- OPEN, CLOSE, READ, TELL, SEEK, EOF, FILE, PWD, GLOB

### Regular Expressions (2 commands)
- REGEXP, REGSUB

### Introspection (1 command with subcommands)
- INFO (EXISTS, COMMANDS, PROCS, VARS, LOCALS, GLOBALS, BODY)

### Time (1 command with subcommands)
- CLOCK (SECONDS, MILLISECONDS, MICROSECONDS, FORMAT, SCAN, ADD)

### System (5 commands)
- EVAL, SOURCE, AFTER, EXEC, ENV, ARGV

## Test Results

Each test file produces PASS/FAIL output for individual test cases. Review the output to identify any failing tests.

## Adding New Tests

To add new tests:

1. Create a new `.bcl` file in this directory
2. Follow the existing test structure with PASS/FAIL output
3. Add the test to `run_all_tests.bcl`
4. Document the test in this README

## Test Specifications

All tests are based on the official BCL specifications documented in:
- `docs/manual-eng/main.pdf` (English manual - 131 pages)
- `docs/manual-es/main.pdf` (Spanish manual - 132 pages)
