# Documentation Updates Needed for BCL Manuals

## Overview

The BCL interpreter has been tested comprehensively and several bugs have been fixed.
The manuals need updating to reflect the actual implementation behavior.

## Manual Files to Update

- `docs/manual-eng/main.tex` (English manual - 131 pages)
- `docs/manual-es/main.tex` (Spanish manual - 132 pages)

## Specific Corrections Needed

### 1. SEEK Command (File I/O Chapter)

**Current Documentation:** Likely specifies "SET, CUR, END"
**Actual Behavior:** Accepts "START or SET, CUR or CURRENT, END"

```latex
% Update in File I/O chapter
\subsection{SEEK}
Syntax: SEEK handle offset whence

Where whence can be:
- START or SET: from beginning of file
- CUR or CURRENT: from current position
- END: from end of file
```

### 2. INFO Command (Introspection Chapter)

**Add:** INFO BODY subcommand documentation

```latex
\subsection{INFO BODY}
Returns the body of a procedure.

Syntax: INFO BODY procname

Returns: String representation of procedure body
```

### 3. ENV Command (System Chapter)

**Current Documentation:** May specify error on missing variable
**Actual Behavior:** Returns empty string if variable doesn't exist

```latex
\subsection{ENV}
Returns the value of an environment variable, or empty string if not found.

Syntax: ENV varname

Returns: Value of environment variable, or "" if not set
```

### 4. LAPPEND Command (List Chapter)

**Clarify:** LAPPEND modifies the variable in-place

```latex
\subsection{LAPPEND}
Appends elements to a list variable.

Syntax: LAPPEND varname element ?element...?

Note: This command modifies the variable in-place and returns the new list.

Example:
SET mylist {a b c}
LAPPEND mylist d e
# mylist is now {a b c d e}
```

### 5. FOR Loop Syntax (Control Flow Chapter)

**Verify/Clarify:** FOR uses internal `$__FOR` variable

```latex
\subsection{FOR}
Iterates from start to end with optional step.

Syntax: FOR start TO end [STEP step] DO
    ... commands ...
END

The loop counter is available as $__FOR variable.

Example:
FOR 1 TO 5 DO
    PUTS "Count: $__FOR"
END
```

### 6. Negative Indices (List Chapter)

**Document Limitation:** Negative indices not supported

```latex
\subsection{LINDEX - Limitations}
Note: Negative indices (e.g., -1 for last element) are not currently supported.
Use LLENGTH and positive indices instead.

Example to get last element:
SET len [LLENGTH $list]
SET last [LINDEX $list [EXPR $len - 1]]
```

## Known Bugs to Document

### STRING LENGTH Segfault

Add warning in STRING chapter:

```latex
\subsection{STRING LENGTH - Known Issue}
\textbf{Warning:} There is a known segmentation fault bug when using
STRING LENGTH in certain contexts (particularly with command substitution).

Workaround: Use STRING EQUAL to check for empty strings instead.

% Instead of:
IF [EXPR [STRING LENGTH $str] > 0] THEN
    ...
END

% Use:
IF ![STRING EQUAL $str ""] THEN
    ...
END
```

### REGEXP NOCASE Option

```latex
\subsection{REGEXP - Limitations}
Note: The NOCASE option is not currently implemented.
Case-insensitive matching must be done by converting both pattern
and text to the same case first.
```

### REGSUB ALL Option

```latex
\subsection{REGSUB - Limitations}
Note: The ALL option is not fully implemented.
Currently only replaces the first occurrence.
```

## Command Reference Table Updates

Update the command reference tables to mark:
- ✅ Fully functional commands
- ⚠️ Commands with limitations
- 🐛 Commands with known bugs

| Command | Status | Notes |
|---------|--------|-------|
| SET | ✅ | Fully functional |
| LAPPEND | ✅ | Modifies variable in-place |
| LINDEX | ⚠️ | No negative index support |
| STRING LENGTH | 🐛 | Segfault issue in some contexts |
| REGEXP | ⚠️ | NOCASE not implemented |
| REGSUB | ⚠️ | ALL not fully implemented |
| SEEK | ✅ | Accepts START/SET, CUR/CURRENT, END |
| ENV | ✅ | Returns empty string if not found |
| INFO BODY | ✅ | Newly implemented |

## Examples to Update

### Example Scripts

Review and test all example scripts in:
- `docs/manual-eng/examples/`
- `docs/manual-es/examples/`

Ensure they:
1. Don't use STRING LENGTH in problematic ways
2. Don't rely on negative indices
3. Don't use REGEXP NOCASE
4. Use correct SEEK syntax

## Testing Examples

Add this testing example to both manuals:

```latex
\chapter{Testing BCL Scripts}

BCL includes a comprehensive test suite in the tests/ directory.

\subsection{Running Tests}
# Run all tests
make test-all

# Run specific test
./bin/bcl tests/test_variables.bcl

\subsection{Test Structure}
Tests are organized by command category:
- test_variables.bcl: Variable commands
- test_control_flow.bcl: Control structures
- test_expressions.bcl: EXPR and math
- test_lists.bcl: List manipulation
- test_strings.bcl: String operations
... (etc)

\subsection{Test Results}
Current test pass rate: 77% (10/13 suites)
See TEST_RESULTS.md for detailed results.
```

## Version Number

Update version in all documentation:
- Change from v1.5.1 to v1.6.0 (to reflect bug fixes)
- Or v1.5.2 if minor version bump preferred

## Compilation Instructions

### English Manual

```bash
cd docs/manual-eng
pdflatex main.tex
makeindex main.idx
pdflatex main.tex
pdflatex main.tex
```

### Spanish Manual

```bash
cd docs/manual-es
pdflatex main.tex
makeindex main.idx
pdflatex main.tex
pdflatex main.tex
```

## Priority Updates

### High Priority (Required)
1. ✅ SEEK command syntax
2. ✅ ENV command behavior
3. ✅ LAPPEND in-place modification
4. ✅ INFO BODY addition
5. 🐛 STRING LENGTH warning

### Medium Priority (Recommended)
6. ⚠️ Negative index limitation
7. ⚠️ FOR loop $__FOR variable
8. ⚠️ REGEXP/REGSUB limitations

### Low Priority (Nice to Have)
9. 📝 Testing chapter addition
10. 📝 Version number update
11. 📝 Command reference table

## Validation

After updating documentation:

1. **Compile PDFs** - Ensure LaTeX compiles without errors
2. **Verify Examples** - Run all example code from the manuals
3. **Cross-Reference** - Check that implementation matches documentation
4. **Test Links** - Verify all internal references work

## Notes for Manual Authors

- Keep both English and Spanish versions synchronized
- Test all code examples before including
- Mark known issues clearly with warning boxes
- Include workarounds for known bugs
- Reference test files for examples of correct usage
- Update table of contents and index

---

*Generated: 2025-11-16*
*Based on BCL v1.5.1 testing and v1.6.0 fixes*
*Test results: 10/13 suites passing (77%)*
