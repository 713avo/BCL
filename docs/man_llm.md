# BCL Language Reference for LLM Code Generation

**Version:** 2.0.0
**Purpose:** Complete reference optimized for RAG-based code generation
**Target:** Large Language Models generating BCL code

---

## Table of Contents

1. [Language Overview](#language-overview)
2. [Syntax Fundamentals](#syntax-fundamentals)
3. [Variables and Data](#variables-and-data)
4. [Expressions and Math](#expressions-and-math)
5. [Control Flow](#control-flow)
6. [Procedures (Functions)](#procedures-functions)
7. [Lists](#lists)
8. [Strings](#strings)
9. [Arrays (Associative)](#arrays-associative)
10. [File Operations](#file-operations)
11. [Regular Expressions](#regular-expressions)
12. [Time and Date](#time-and-date)
13. [System Commands](#system-commands)
14. [Binary Data](#binary-data)
15. [Introspection](#introspection)
16. [Common Patterns](#common-patterns)
17. [Known Limitations](#known-limitations)

---

## Language Overview

BCL (Basic Command Language) is a lightweight scripting language with BASIC-like syntax inspired by Tcl.

### Core Principles
- **Everything is a string** - All data is stored as strings, converted as needed
- **Case-insensitive commands** - `PUTS`, `puts`, `Puts` all work
- **Variable substitution** - `$var` expands to variable value
- **Command substitution** - `[command args]` executes and returns result
- **Block structure** - All blocks end with `END` keyword

### Basic Script Structure
```bcl
#!/usr/bin/env bcl
# This is a comment

# Variable assignment
SET variable "value"

# Command execution
PUTS $variable

# Command substitution
SET result [EXPR 5 + 3]
PUTS "5 + 3 = $result"
```

---

## Syntax Fundamentals

### Comments
```bcl
# Single-line comment (must be at start of line or after semicolon)
PUTS "Hello"  # This does NOT work - comments must start at line beginning
```

### Variable Substitution
```bcl
SET name "World"
PUTS "Hello, $name"              # Simple substitution
PUTS "Hello, ${name}!"           # Braced substitution (recommended with punctuation)
SET x 5
SET y [EXPR $x * 2]              # y = 10
```

### Command Substitution
```bcl
SET length [STRING LENGTH "Hello"]      # Nested command execution
SET sum [EXPR [EXPR 5 + 3] * 2]        # Nested expressions
SET files [GLOB *.txt]                  # Command returns list
```

### Quoting
```bcl
SET simple hello                        # No quotes for simple strings
SET phrase "Hello World"                # Quotes for strings with spaces
SET path "/home/user/file.txt"         # Quotes for paths
SET escaped "Line 1\nLine 2"           # Escape sequences work
```

### Escape Sequences
```bcl
# Standard escape sequences
SET newline "Line 1\nLine 2"           # \n = newline
SET tab "Col1\tCol2\tCol3"             # \t = tab
SET backslash "Path\\File"             # \\ = backslash
SET quote "Say \"Hello\""              # \" = double quote

# Unicode escape sequences (v2.0+)
SET box_corner "\u250C"                # \uXXXX = 4-digit Unicode (┌)
SET emoji "\U0001F600"                 # \UXXXXXXXX = 8-digit Unicode (😀)
SET arrow "\u2192"                     # → (right arrow)
SET checkmark "\u2713"                 # ✓ (checkmark)

# Common Unicode box drawing
SET border_top "\u2500"                # ─ horizontal line
SET border_left "\u2502"               # │ vertical line
SET corner_tl "\u250C"                 # ┌ top-left
SET corner_tr "\u2510"                 # ┐ top-right
SET corner_bl "\u2514"                 # └ bottom-left
SET corner_br "\u2518"                 # ┘ bottom-right
```

### Line Continuation
```bcl
# No explicit line continuation - commands span multiple lines naturally
SET result [EXPR
    5 + 3 * 2 +
    10 / 2
]
```

---

## Variables and Data

### SET - Assign or retrieve variable
```bcl
SET name "John"                # Assign string
SET age 30                     # Assign number (stored as string)
SET value [EXPR 5 + 3]        # Assign result of expression

SET x                          # Retrieve value (returns value of x)
SET result [SET name]          # Retrieve and assign to another variable
```

### UNSET - Delete variable
```bcl
SET temp 123
UNSET temp                     # Variable no longer exists
UNSET var1 var2 var3          # Delete multiple variables
```

### INCR - Increment numeric variable
```bcl
SET counter 5
INCR counter                   # counter = 6 (increment by 1)
INCR counter 5                 # counter = 11 (increment by 5)
INCR counter -3                # counter = 8 (decrement by 3)

# If variable doesn't exist, creates it with value 0 then increments
INCR newvar                    # newvar = 1
```

### APPEND - Append to variable
```bcl
SET str "Hello"
APPEND str " " "World"         # str = "Hello World"
APPEND str "!"                 # str = "Hello World!"

# Works on non-existent variables (creates them)
APPEND newstr "First"          # newstr = "First"
```

### GLOBAL - Access global variable from procedure
```bcl
SET globalVar "global value"

PROC modifyGlobal WITH DO
    GLOBAL globalVar           # Access global scope
    SET globalVar "modified"   # Modifies global, not local
END

modifyGlobal
PUTS $globalVar               # Prints "modified"
```

### Variable Naming Rules
```bcl
# Valid variable names
SET myVar 123
SET my_var 456
SET MyVar789 "test"
SET _private "value"

# Variable names are case-insensitive
SET myvar 10
SET MYVAR 20                  # Same variable! MYVAR = 20
```

---

## Expressions and Math

### EXPR - Evaluate mathematical expression
```bcl
# Basic arithmetic operators: +  -  *  /  %  **
SET sum [EXPR 5 + 3]                    # Addition: 8
SET diff [EXPR 10 - 4]                  # Subtraction: 6
SET product [EXPR 6 * 7]                # Multiplication: 42
SET quotient [EXPR 20 / 4]              # Division: 5
SET remainder [EXPR 17 % 5]             # Modulo: 2
SET power [EXPR 2 ** 8]                 # Exponentiation: 256

# Comparison operators: ==  !=  <  >  <=  >=
IF [EXPR 10 > 5] THEN
    PUTS "10 is greater than 5"
END

IF [EXPR $x == $y] THEN
    PUTS "Equal"
END

# Logical operators: &&  ||  !
IF [EXPR $a > 0 && $b > 0] THEN
    PUTS "Both positive"
END

IF [EXPR $value < 0 || $value > 100] THEN
    PUTS "Out of range"
END

IF [EXPR !$flag] THEN
    PUTS "Flag is false/zero"
END

# Parentheses for precedence
SET result [EXPR (5 + 3) * 2]           # 16, not 11
SET complex [EXPR ($a + $b) / ($c - $d)]
```

### Math Functions (30+ available)

#### Trigonometric Functions
```bcl
SET angle [EXPR 3.14159 / 4]            # 45 degrees in radians
SET sine [EXPR sin($angle)]             # sin(45°) ≈ 0.707
SET cosine [EXPR cos($angle)]           # cos(45°) ≈ 0.707
SET tangent [EXPR tan($angle)]          # tan(45°) ≈ 1.0
SET arcsine [EXPR asin(0.5)]            # arcsin(0.5) ≈ 0.524 (30°)
SET arccosine [EXPR acos(0.5)]          # arccos(0.5) ≈ 1.047 (60°)
SET arctangent [EXPR atan(1.0)]         # arctan(1.0) ≈ 0.785 (45°)
SET atan2result [EXPR atan2($y, $x)]    # atan2(y, x) - angle from origin
```

#### Hyperbolic Functions
```bcl
SET hypsine [EXPR sinh(1.0)]            # Hyperbolic sine
SET hypcosine [EXPR cosh(1.0)]          # Hyperbolic cosine
SET hyptangent [EXPR tanh(1.0)]         # Hyperbolic tangent
```

#### Exponential and Logarithmic
```bcl
SET exponential [EXPR exp(2.0)]         # e^2 ≈ 7.389
SET natural_log [EXPR log(10.0)]        # ln(10) ≈ 2.303
SET base10_log [EXPR log10(100.0)]      # log₁₀(100) = 2.0
SET power [EXPR pow(2, 10)]             # 2^10 = 1024
```

#### Rounding and Absolute Value
```bcl
SET absolute [EXPR abs(-42)]            # abs(-42) = 42
SET floored [EXPR floor(3.7)]           # floor(3.7) = 3
SET ceiled [EXPR ceil(3.2)]             # ceil(3.2) = 4
SET rounded [EXPR round(3.6)]           # round(3.6) = 4
```

#### Square Root
```bcl
SET root [EXPR sqrt(16)]                # sqrt(16) = 4.0
SET root2 [EXPR sqrt(2)]                # sqrt(2) ≈ 1.414
```

#### Min/Max (variadic - accepts multiple arguments)
```bcl
SET minimum [EXPR min(5, 3, 9, 1)]      # min(...) = 1
SET maximum [EXPR max(5, 3, 9, 1)]      # max(...) = 9
SET min2 [EXPR min($a, $b)]             # Works with 2 or more args
```

#### Random Number
```bcl
SET random [EXPR rand()]                # Random float between 0.0 and 1.0
SET random_int [EXPR int(rand() * 100)] # Random integer 0-99
```

#### Type Conversion
```bcl
SET integer [EXPR int(3.7)]             # int(3.7) = 3 (truncate)
SET floating [EXPR double(42)]          # Convert to floating-point
```

---

## Control Flow

### IF/THEN/ELSE/ELSEIF/END
```bcl
# Simple IF/THEN
IF [EXPR $x > 0] THEN
    PUTS "Positive"
END

# IF/THEN/ELSE
IF [EXPR $age >= 18] THEN
    PUTS "Adult"
ELSE
    PUTS "Minor"
END

# IF/ELSEIF/ELSE chain
SET score 85
IF [EXPR $score >= 90] THEN
    PUTS "Grade: A"
ELSEIF [EXPR $score >= 80] THEN
    PUTS "Grade: B"
ELSEIF [EXPR $score >= 70] THEN
    PUTS "Grade: C"
ELSEIF [EXPR $score >= 60] THEN
    PUTS "Grade: D"
ELSE
    PUTS "Grade: F"
END

# Nested IF
IF [EXPR $a > 0] THEN
    IF [EXPR $b > 0] THEN
        PUTS "Both positive"
    END
END
```

### WHILE/DO/END
```bcl
# Basic WHILE loop
SET counter 0
WHILE [EXPR $counter < 5] DO
    PUTS "Count: $counter"
    INCR counter
END

# WHILE with BREAK
SET i 0
WHILE [EXPR $i < 100] DO
    INCR i
    IF [EXPR $i == 10] THEN
        BREAK                  # Exit loop immediately
    END
END

# WHILE with CONTINUE
SET sum 0
SET n 0
WHILE [EXPR $n < 10] DO
    INCR n
    IF [EXPR $n % 2 == 0] THEN
        CONTINUE               # Skip to next iteration
    END
    INCR sum $n               # Only adds odd numbers
END
```

### FOR/FROM/TO/STEP/DO/END
```bcl
# Basic FOR loop (1 to 5 inclusive)
FOR 1 TO 5 DO
    PUTS "Iteration: $__FOR"   # $__FOR contains current value
END

# FOR with explicit FROM
FOR FROM 0 TO 10 DO
    PUTS $__FOR
END

# FOR with STEP (custom increment)
FOR 0 TO 100 STEP 10 DO
    PUTS $__FOR               # Prints: 0, 10, 20, ..., 100
END

# FOR with negative step (countdown)
FOR 10 TO 0 STEP -1 DO
    PUTS "T-minus $__FOR"
END

# FOR loop accumulation
SET total 0
FOR 1 TO 10 DO
    INCR total $__FOR         # Sum of 1+2+3+...+10 = 55
END
```

### FOREACH/IN/DO/END
```bcl
# FOREACH over list
SET fruits [LIST apple banana cherry]
FOREACH fruit IN $fruits DO
    PUTS "Fruit: $fruit"
END

# FOREACH over array keys
SET colors(red) "#FF0000"
SET colors(green) "#00FF00"
SET colors(blue) "#0000FF"

FOREACH key IN [ARRAY NAMES colors] DO
    PUTS "$key = $colors($key)"
END

# FOREACH with BREAK/CONTINUE
SET numbers [LIST 1 2 3 4 5 6 7 8 9 10]
FOREACH num IN $numbers DO
    IF [EXPR $num % 2 == 0] THEN
        CONTINUE               # Skip even numbers
    END
    PUTS $num                  # Only prints odd numbers
END
```

### SWITCH/CASE/DEFAULT/END
```bcl
# Basic SWITCH
SET day "Tuesday"
SWITCH $day DO
    CASE Monday
        PUTS "Start of week"
    CASE Tuesday
    CASE Wednesday
    CASE Thursday
        PUTS "Midweek"
    CASE Friday
        PUTS "Almost weekend"
    DEFAULT
        PUTS "Weekend!"
END

# SWITCH with multiple actions per case
SET command "help"
SWITCH $command DO
    CASE help
    CASE h
        PUTS "Help: Available commands..."
        PUTS "  start - Start service"
        PUTS "  stop  - Stop service"
    CASE start
        PUTS "Starting service..."
        # Execute start logic
    CASE stop
        PUTS "Stopping service..."
    DEFAULT
        PUTS "Unknown command: $command"
END
```

### BREAK - Exit loop
```bcl
# Find first element matching condition
SET found ""
SET items [LIST apple banana cherry date]
FOREACH item IN $items DO
    IF [STRING MATCH "c*" $item] THEN
        SET found $item
        BREAK                  # Stop searching once found
    END
END
```

### CONTINUE - Skip to next iteration
```bcl
# Process only even numbers
FOR 1 TO 10 DO
    IF [EXPR $__FOR % 2 != 0] THEN
        CONTINUE               # Skip odd numbers
    END
    PUTS "Processing: $__FOR"
END
```

### RETURN - Return from procedure
```bcl
PROC add WITH a b DO
    RETURN [EXPR $a + $b]
END

SET result [add 5 3]          # result = 8

# Early return
PROC checkValue WITH n DO
    IF [EXPR $n < 0] THEN
        RETURN "negative"
    END
    IF [EXPR $n == 0] THEN
        RETURN "zero"
    END
    RETURN "positive"
END
```

### EXIT - Terminate program
```bcl
# Exit with status code
IF [EXPR $error] THEN
    PUTS "Fatal error occurred"
    EXIT 1                     # Exit with error code
END

EXIT 0                         # Successful exit
```

---

## Procedures (Functions)

### PROC - Define procedure
```bcl
# Basic procedure with no parameters
PROC greet WITH DO
    PUTS "Hello, World!"
END

greet                          # Call procedure

# Procedure with parameters
PROC sayHello WITH name DO
    PUTS "Hello, $name!"
END

sayHello "Alice"               # Call with argument

# Multiple parameters
PROC add WITH a b DO
    SET sum [EXPR $a + $b]
    RETURN $sum
END

SET result [add 10 20]         # result = 30

# Parameters with variable argument count
PROC sum WITH args DO
    SET total 0
    FOREACH num IN $args DO
        INCR total $num
    END
    RETURN $total
END

SET s [sum 1 2 3 4 5]         # s = 15

# Procedure with local variables
PROC calculateArea WITH width height DO
    SET area [EXPR $width * $height]
    SET perimeter [EXPR 2 * ($width + $height)]

    # Local variables don't leak outside
    RETURN $area
END

# Recursive procedure
PROC factorial WITH n DO
    IF [EXPR $n <= 1] THEN
        RETURN 1
    ELSE
        SET prev [factorial [EXPR $n - 1]]
        RETURN [EXPR $n * $prev]
    END
END

SET f5 [factorial 5]           # f5 = 120

# Procedure accessing global variable
SET globalCounter 0

PROC incrementGlobal WITH DO
    GLOBAL globalCounter       # Must declare GLOBAL to modify
    INCR globalCounter
END

incrementGlobal
PUTS $globalCounter            # Prints: 1
```

### Procedure Naming Rules
```bcl
# Valid procedure names
PROC myProc WITH DO
    # ...
END

PROC my_procedure WITH x y DO
    # ...
END

PROC MyProcedure123 WITH DO
    # ...
END

# Call procedures (case-insensitive)
myProc
MYPROC                         # Same procedure
MyProc                         # Same procedure
```

---

## Lists

Lists are space-separated strings. Multi-word elements require braces `{}` or quotes.

### LIST - Create list
```bcl
SET mylist [LIST apple banana cherry]
# mylist = "apple banana cherry"

SET mixed [LIST one two "three four" five]
# mixed = "one two {three four} five"

SET numbers [LIST 1 2 3 4 5]

# Empty list
SET empty [LIST]
```

### SPLIT - Split string into list
```bcl
SET csv "apple,banana,cherry"
SET fruits [SPLIT $csv ","]
# fruits = "apple banana cherry"

SET path "/usr/local/bin"
SET parts [SPLIT $path "/"]
# parts = " usr local bin" (note leading empty element)

# Split by whitespace (default)
SET text "one   two    three"
SET words [SPLIT $text]
# words = "one two three"
```

### JOIN - Join list into string
```bcl
SET items [LIST apple banana cherry]
SET csv [JOIN $items ","]
# csv = "apple,banana,cherry"

SET path [JOIN [LIST usr local bin] "/"]
# path = "usr/local/bin"

# Join with space (default)
SET joined [JOIN $items]
# joined = "apple banana cherry"
```

### LINDEX - Get element at index
```bcl
SET fruits [LIST apple banana cherry date]
SET first [LINDEX $fruits 0]           # first = "apple"
SET second [LINDEX $fruits 1]          # second = "banana"
SET last [LINDEX $fruits 3]            # last = "date"

# Negative indices NOT supported
# Use LLENGTH to calculate: [LINDEX $list [EXPR [LLENGTH $list] - 1]]
```

### LRANGE - Extract sublist
```bcl
SET numbers [LIST 0 1 2 3 4 5 6 7 8 9]
SET subset [LRANGE $numbers 2 5]       # subset = "2 3 4 5"
SET from3 [LRANGE $numbers 3 end]      # from3 = "3 4 5 6 7 8 9"
SET first3 [LRANGE $numbers 0 2]       # first3 = "0 1 2"
```

### LLENGTH - Get list length
```bcl
SET items [LIST a b c d e]
SET count [LLENGTH $items]             # count = 5

SET empty [LIST]
SET zero [LLENGTH $empty]              # zero = 0

# Check if list is empty
IF [EXPR [LLENGTH $mylist] == 0] THEN
    PUTS "List is empty"
END
```

### LAPPEND - Append element to list
```bcl
SET fruits [LIST apple banana]
LAPPEND fruits cherry                  # fruits = "apple banana cherry"
LAPPEND fruits date elderberry         # fruits = "apple banana cherry date elderberry"

# LAPPEND creates list if variable doesn't exist
LAPPEND newlist first                  # newlist = "first"
```

### LINSERT - Insert element at position
```bcl
SET numbers [LIST 1 2 4 5]
SET fixed [LINSERT $numbers 2 3]       # fixed = "1 2 3 4 5"

SET list [LIST a c]
SET list2 [LINSERT $list 1 b]          # list2 = "a b c"
```

### LREPLACE - Replace range in list
```bcl
SET numbers [LIST 0 1 2 3 4 5]
SET replaced [LREPLACE $numbers 2 3 X Y]
# replaced = "0 1 X Y 4 5"

# Delete elements (no replacement)
SET deleted [LREPLACE $numbers 1 2]
# deleted = "0 3 4 5"
```

### CONCAT - Concatenate lists
```bcl
SET list1 [LIST a b c]
SET list2 [LIST d e f]
SET combined [CONCAT $list1 $list2]
# combined = "a b c d e f"

SET all [CONCAT [LIST 1 2] [LIST 3 4] [LIST 5 6]]
# all = "1 2 3 4 5 6"
```

### LSORT - Sort list
```bcl
SET unsorted [LIST cherry apple banana]
SET sorted [LSORT $unsorted]
# sorted = "apple banana cherry"

SET numbers [LIST 10 2 5 1 8]
SET numSorted [LSORT $numbers]
# numSorted = "1 10 2 5 8" (lexicographic sort!)

# For numeric sort, use custom comparison in EXPR
```

### LSEARCH - Find element in list
```bcl
SET fruits [LIST apple banana cherry]
SET index [LSEARCH $fruits "banana"]   # index = 1
SET notfound [LSEARCH $fruits "grape"] # notfound = -1

# Check if element exists
IF [EXPR [LSEARCH $fruits "apple"] >= 0] THEN
    PUTS "Found apple"
END
```

---

## Strings

### STRING LENGTH - Get string length
```bcl
SET text "Hello"
SET len [STRING LENGTH $text]          # len = 5

SET empty ""
SET zero [STRING LENGTH $empty]        # zero = 0
```

### STRING INDEX - Get character at position
```bcl
SET word "Hello"
SET first [STRING INDEX $word 0]       # first = "H"
SET last [STRING INDEX $word 4]        # last = "o"

# Negative indices NOT supported
```

### STRING RANGE - Extract substring
```bcl
SET text "Hello, World!"
SET hello [STRING RANGE $text 0 4]     # hello = "Hello"
SET world [STRING RANGE $text 7 11]    # world = "World"
SET fromH [STRING RANGE $text 7 end]   # fromH = "World!"
```

### STRING TOUPPER - Convert to uppercase
```bcl
SET lower "hello world"
SET upper [STRING TOUPPER $lower]      # upper = "HELLO WORLD"
```

### STRING TOLOWER - Convert to lowercase
```bcl
SET mixed "Hello World"
SET lower [STRING TOLOWER $mixed]      # lower = "hello world"
```

### STRING TOTITLE - Convert to title case
```bcl
SET lower "hello world"
SET title [STRING TOTITLE $lower]      # title = "Hello World"
```

### STRING TRIM - Remove whitespace from both ends
```bcl
SET padded "  Hello  "
SET trimmed [STRING TRIM $padded]      # trimmed = "Hello"

SET custom "***Hello***"
SET trimmed2 [STRING TRIM $custom "*"] # trimmed2 = "Hello"
```

### STRING TRIMLEFT - Remove from left
```bcl
SET text "  Hello"
SET trimmed [STRING TRIMLEFT $text]    # trimmed = "Hello"
```

### STRING TRIMRIGHT - Remove from right
```bcl
SET text "Hello  "
SET trimmed [STRING TRIMRIGHT $text]   # trimmed = "Hello"
```

### STRING COMPARE - Compare strings
```bcl
SET result [STRING COMPARE "abc" "abc"] # result = 0 (equal)
SET result [STRING COMPARE "abc" "xyz"] # result < 0 (abc < xyz)
SET result [STRING COMPARE "xyz" "abc"] # result > 0 (xyz > abc)

# Case-insensitive comparison
SET result [STRING COMPARE -NOCASE "Hello" "hello"] # result = 0
```

### STRING EQUAL - Test equality
```bcl
IF [STRING EQUAL $name "Alice"] THEN
    PUTS "Hello, Alice!"
END

# Case-insensitive
IF [STRING EQUAL -NOCASE $input "yes"] THEN
    PUTS "User confirmed"
END
```

### STRING MATCH - Glob pattern matching
```bcl
# Patterns: * (any chars), ? (one char), [abc] (char class)
IF [STRING MATCH "*.txt" $filename] THEN
    PUTS "Text file"
END

IF [STRING MATCH "test_*" $name] THEN
    PUTS "Test function"
END

# Case-insensitive
IF [STRING MATCH -NOCASE "*error*" $message] THEN
    PUTS "Error found"
END
```

### STRING REPLACE - Replace substring
```bcl
SET text "Hello World"
SET replaced [STRING REPLACE $text "World" "BCL"]
# replaced = "Hello BCL"

# Replace first occurrence only (default)
SET text "aaa"
SET replaced [STRING REPLACE $text "a" "b"]
# replaced = "baa"
```

### STRING REPEAT - Repeat string
```bcl
SET dash [STRING REPEAT "-" 10]        # dash = "----------"
SET pattern [STRING REPEAT "ab" 3]     # pattern = "ababab"
```

### STRING REVERSE - Reverse string
```bcl
SET text "Hello"
SET reversed [STRING REVERSE $text]    # reversed = "olleH"
```

### STRING MAP - Character mapping
```bcl
SET mapping [LIST a A e E i I o O u U]
SET text "hello"
SET result [STRING MAP $mapping $text]
# result = "hEllO" (vowels capitalized)
```

### STRING FIRST - Find first occurrence
```bcl
SET text "Hello World"
SET index [STRING FIRST "o" $text]     # index = 4
SET notfound [STRING FIRST "x" $text]  # notfound = -1

# Search from specific position
SET index [STRING FIRST "o" $text 5]   # index = 7 (second 'o')
```

### STRING LAST - Find last occurrence
```bcl
SET text "Hello World"
SET index [STRING LAST "o" $text]      # index = 7
```

---

## Arrays (Associative)

Associative arrays use parenthesis syntax: `$array(key)`

### ARRAY Syntax
```bcl
# Set array element
SET config(host) "localhost"
SET config(port) 8080
SET config(database) "mydb"

# Get array element
PUTS "Host: $config(host)"
SET p $config(port)

# Array keys can be variables
SET key "host"
PUTS $config($key)
```

### ARRAY EXISTS - Check if array exists
```bcl
SET myarray(key) "value"

IF [ARRAY EXISTS myarray] THEN
    PUTS "Array exists"
END

IF ![ARRAY EXISTS notexist] THEN
    PUTS "Array doesn't exist"
END
```

### ARRAY SIZE - Get number of elements
```bcl
SET config(a) 1
SET config(b) 2
SET config(c) 3

SET count [ARRAY SIZE config]          # count = 3
```

### ARRAY NAMES - Get list of keys
```bcl
SET person(name) "Alice"
SET person(age) 30
SET person(city) "NYC"

SET keys [ARRAY NAMES person]
# keys = "name age city" (order may vary)

# With pattern (glob-style)
SET keys [ARRAY NAMES person "na*"]    # keys = "name"
```

### ARRAY GET - Get all key-value pairs
```bcl
SET data(x) 10
SET data(y) 20

SET pairs [ARRAY GET data]
# pairs = "x 10 y 20" (flat list of alternating keys and values)

# Restore array from pairs
ARRAY SET newdata $pairs
# newdata(x) = 10, newdata(y) = 20
```

### ARRAY SET - Set from key-value list
```bcl
SET pairs [LIST key1 value1 key2 value2]
ARRAY SET myarray $pairs
# myarray(key1) = "value1"
# myarray(key2) = "value2"
```

### ARRAY UNSET - Delete array or element
```bcl
# Delete specific element
SET arr(a) 1
SET arr(b) 2
ARRAY UNSET arr a              # Removes arr(a)

# Delete entire array
ARRAY UNSET arr                # Removes all elements
```

### Array Global Scope (v2.0+)

BCL 2.0 introduces automatic array global prefix detection. When you declare a variable as GLOBAL in a procedure, the system detects whether it's an array and automatically makes all elements with that prefix global.

```bcl
# Example 1: Creating array inside procedure
PROC INIT_CONFIG DO
    GLOBAL config               # Marks entire config(...) array as global
    SET config(host) "localhost"
    SET config(port) 8080
    SET config(db) "mydb"
END

INIT_CONFIG
PUTS $config(host)              # Works! Prints: localhost
PUTS $config(port)              # Works! Prints: 8080
```

**How it works:**
1. `GLOBAL config` checks if `config` exists as a scalar variable
2. If not scalar, it marks the prefix `config(` as global
3. All future assignments to `config(*)` go to global scope
4. This works whether the array exists before or after the GLOBAL declaration

```bcl
# Example 2: Array exists before GLOBAL
SET data(x) 10

PROC MODIFY_DATA DO
    GLOBAL data                 # Detects existing data(...) elements
    SET data(y) 20             # Creates globally
    INCR data(x)               # Modifies global value
END

MODIFY_DATA
PUTS "$data(x) $data(y)"       # Prints: 11 20
```

```bcl
# Example 3: Multiple arrays in one procedure
PROC MATRIX_OP DO
    GLOBAL A B C               # All three arrays are global
    SET A(0,0) 1
    SET B(0,0) 2
    SET C(0,0) [EXPR $A(0,0) + $B(0,0)]
END

MATRIX_OP
PUTS $C(0,0)                   # Prints: 3
```

```bcl
# Example 4: Library pattern with dynamic GLOBAL
PROC MAT_CREATE WITH name rows cols DO
    GLOBAL _mat                # Metadata array is global
    EVAL "GLOBAL $name"        # Data array is global (dynamic)

    SET _mat($name.rows) $rows
    SET _mat($name.cols) $cols

    # Now all $name(...) elements will be global
    SET $name(0,0) 0
END

MAT_CREATE M 3 3
PUTS $_mat(M.rows)             # Prints: 3
PUTS $M(0,0)                   # Prints: 0
```

**Important notes:**
- If a scalar variable exists with the same name, GLOBAL marks only the scalar (not the array)
- Array indices can contain any characters: dots, commas, etc.
- Useful for libraries that manage window, matrix, or configuration data

### Iterating Over Arrays
```bcl
SET scores(Alice) 95
SET scores(Bob) 87
SET scores(Charlie) 92

FOREACH name IN [ARRAY NAMES scores] DO
    PUTS "$name scored $scores($name)"
END
```

---

## File Operations

### OPEN - Open file
```bcl
# Open modes: "r" (read), "w" (write), "a" (append)
SET fh [OPEN "/path/to/file.txt" "r"]
SET fh [OPEN "output.txt" "w"]
SET fh [OPEN "log.txt" "a"]

# File handle is returned (use for subsequent operations)
```

### CLOSE - Close file
```bcl
SET fh [OPEN "file.txt" "r"]
# ... operations ...
CLOSE $fh
```

### READ - Read entire file or N bytes
```bcl
# Read entire file
SET fh [OPEN "file.txt" "r"]
SET content [READ $fh]
CLOSE $fh

# Read N bytes
SET fh [OPEN "binary.dat" "r"]
SET chunk [READ $fh 1024]      # Read 1024 bytes
CLOSE $fh
```

### GETS - Read one line
```bcl
SET fh [OPEN "file.txt" "r"]
SET line1 [GETS $fh]           # First line (without newline)
SET line2 [GETS $fh]           # Second line
CLOSE $fh

# Read all lines
SET fh [OPEN "file.txt" "r"]
WHILE ![EOF $fh] DO
    SET line [GETS $fh]
    PUTS "Line: $line"
END
CLOSE $fh
```

### PUTS - Write to file (or stdout)
```bcl
# Write to stdout
PUTS "Hello, World!"

# Write to file
SET fh [OPEN "output.txt" "w"]
PUTS $fh "Line 1"
PUTS $fh "Line 2"
CLOSE $fh

# Write without newline
SET fh [OPEN "output.txt" "w"]
PUTS -NONEWLINE $fh "Prompt: "
CLOSE $fh
```

### TELL - Get current file position
```bcl
SET fh [OPEN "file.txt" "r"]
SET pos [TELL $fh]             # Current position (bytes from start)
CLOSE $fh
```

### SEEK - Set file position
```bcl
SET fh [OPEN "file.txt" "r"]
SEEK $fh 100 START             # Seek to byte 100 from start
SEEK $fh 10 CURRENT            # Seek 10 bytes forward
SEEK $fh -5 CURRENT            # Seek 5 bytes backward
SEEK $fh 0 END                 # Seek to end of file
CLOSE $fh
```

### EOF - Check end of file
```bcl
SET fh [OPEN "file.txt" "r"]
IF [EOF $fh] THEN
    PUTS "Already at end"
END

WHILE ![EOF $fh] DO
    SET line [GETS $fh]
    PUTS $line
END
CLOSE $fh
```

### FLUSH - Flush file buffer
```bcl
SET fh [OPEN "output.txt" "w"]
PUTS $fh "Important data"
FLUSH $fh                      # Ensure written to disk
CLOSE $fh
```

### FILE EXISTS - Check if file exists
```bcl
IF [FILE EXISTS "/path/to/file.txt"] THEN
    PUTS "File exists"
ELSE
    PUTS "File not found"
END
```

### FILE SIZE - Get file size in bytes
```bcl
SET bytes [FILE SIZE "myfile.txt"]
PUTS "File size: $bytes bytes"
```

### FILE TYPE - Get file type
```bcl
SET type [FILE TYPE "myfile.txt"]
# type = "file" or "directory" or "link" or "unknown"

IF [STRING EQUAL $type "directory"] THEN
    PUTS "It's a directory"
END
```

### FILE DELETE - Delete file
```bcl
FILE DELETE "tempfile.txt"
FILE DELETE "file1.txt" "file2.txt" "file3.txt"
```

### FILE RENAME - Rename/move file
```bcl
FILE RENAME "oldname.txt" "newname.txt"
FILE RENAME "file.txt" "/backup/file.txt"
```

### FILE MTIME - Get modification time
```bcl
SET timestamp [FILE MTIME "file.txt"]
# timestamp = Unix timestamp (seconds since epoch)
```

### PWD - Get current directory
```bcl
SET currentdir [PWD]
PUTS "Current directory: $currentdir"
```

### GLOB - Find files matching pattern
```bcl
# Find all .txt files
SET txtfiles [GLOB "*.txt"]

# Find all .c and .h files
SET sources [GLOB "*.c"]
SET headers [GLOB "*.h"]

# Recursive pattern (depends on implementation)
SET allpy [GLOB "**/*.py"]

# Check if any files match
SET files [GLOB "*.log"]
IF [EXPR [LLENGTH $files] > 0] THEN
    PUTS "Found [LLENGTH $files] log files"
END
```

### Complete File Processing Example
```bcl
# Read input file, process lines, write output
IF ![FILE EXISTS "input.txt"] THEN
    PUTS "Error: input.txt not found"
    EXIT 1
END

SET infh [OPEN "input.txt" "r"]
SET outfh [OPEN "output.txt" "w"]

SET linenum 0
WHILE ![EOF $infh] DO
    SET line [GETS $infh]
    INCR linenum

    # Process line
    SET upper [STRING TOUPPER $line]
    PUTS $outfh "$linenum: $upper"
END

CLOSE $infh
CLOSE $outfh
PUTS "Processed $linenum lines"
```

---

## Regular Expressions

BCL has built-in regex support (limited compared to PCRE).

### REGEXP - Match pattern
```bcl
# Basic usage
IF [REGEXP "hello" "hello world"] THEN
    PUTS "Match found"
END

# Capture groups
REGEXP "([0-9]+)" "Age: 25" whole number
# whole = "25", number = "25"

# Multiple captures
REGEXP "(\\d+)-(\\d+)-(\\d+)" "2025-11-16" full year month day
# full = "2025-11-16", year = "2025", month = "11", day = "16"

# Case-insensitive matching
REGEXP -NOCASE "hello" "HELLO WORLD"    # Matches

# Check if pattern matches
IF [REGEXP "^[0-9]+$" $input] THEN
    PUTS "Input is numeric"
END
```

### REGSUB - Substitute pattern
```bcl
# Basic substitution
SET text "Hello World"
SET result [REGSUB "World" $text "BCL"]
# result = "Hello BCL"

# Replace all occurrences
SET text "aaa bbb aaa"
SET result [REGSUB -ALL "aaa" $text "xxx"]
# result = "xxx bbb xxx"

# Case-insensitive substitution
SET text "Hello WORLD"
SET result [REGSUB -NOCASE "world" $text "BCL"]
# result = "Hello BCL"

# Using capture groups
SET text "2025-11-16"
SET result [REGSUB "(\\d+)-(\\d+)-(\\d+)" $text "\\3/\\2/\\1"]
# result = "16/11/2025"
```

### Pattern Syntax
```bcl
# Metacharacters:
#   .       - Any single character
#   ^       - Start of string
#   $       - End of string
#   *       - Zero or more of previous
#   +       - One or more of previous
#   ?       - Zero or one of previous
#   [abc]   - Character class (a, b, or c)
#   [^abc]  - Negated class (not a, b, or c)
#   [a-z]   - Range (a through z)
#   \d      - Digit [0-9]
#   \D      - Non-digit
#   \w      - Word character [a-zA-Z0-9_]
#   \W      - Non-word character
#   \s      - Whitespace [ \t\n\r]
#   \S      - Non-whitespace
#   (...)   - Capture group

# Examples:
REGEXP "^\\d+$" $input                  # All digits
REGEXP "^[a-zA-Z]+$" $input             # All letters
REGEXP "\\w+@\\w+\\.\\w+" $email        # Simple email pattern
REGEXP "^\\s*$" $line                   # Blank line (whitespace only)
```

---

## Time and Date

### CLOCK SECONDS - Get Unix timestamp
```bcl
SET now [CLOCK SECONDS]
PUTS "Current timestamp: $now"
```

### CLOCK MILLISECONDS - Millisecond precision
```bcl
SET ms [CLOCK MILLISECONDS]
PUTS "Milliseconds: $ms"
```

### CLOCK MICROSECONDS - Microsecond precision
```bcl
SET us [CLOCK MICROSECONDS]
PUTS "Microseconds: $us"
```

### CLOCK FORMAT - Format timestamp
```bcl
# Get current time formatted
SET now [CLOCK SECONDS]
SET formatted [CLOCK FORMAT $now "%Y-%m-%d %H:%M:%S"]
PUTS "Current time: $formatted"

# Simplified syntax (auto-detect format string)
SET date [CLOCK FORMAT $now "%Y-%m-%d"]
SET time [CLOCK FORMAT $now "%H:%M:%S"]

# Format specifiers (strftime-style):
#   %Y  - Year (4 digits)
#   %m  - Month (01-12)
#   %d  - Day (01-31)
#   %H  - Hour 24h (00-23)
#   %M  - Minute (00-59)
#   %S  - Second (00-59)
#   %a  - Weekday abbreviated (Mon, Tue, ...)
#   %A  - Weekday full (Monday, Tuesday, ...)
#   %b  - Month abbreviated (Jan, Feb, ...)
#   %B  - Month full (January, February, ...)

# Common formats
SET iso8601 [CLOCK FORMAT $now "%Y-%m-%dT%H:%M:%S"]
SET human [CLOCK FORMAT $now "%A, %B %d, %Y"]
```

### CLOCK SCAN - Parse date string
```bcl
# Parse date string to timestamp
SET timestamp [CLOCK SCAN "2025-11-16" "%Y-%m-%d"]
SET timestamp [CLOCK SCAN "11/16/2025" "%m/%d/%Y"]

# Parse datetime
SET ts [CLOCK SCAN "2025-11-16 14:30:00" "%Y-%m-%d %H:%M:%S"]
```

### CLOCK ADD - Time arithmetic
```bcl
SET now [CLOCK SECONDS]

# Add seconds
SET future [CLOCK ADD $now 3600 seconds]        # +1 hour
SET future [CLOCK ADD $now 86400 seconds]       # +1 day

# Units: seconds, minutes, hours, days, weeks
SET tomorrow [CLOCK ADD $now 1 days]
SET nextweek [CLOCK ADD $now 1 weeks]

# Subtract (negative value)
SET yesterday [CLOCK ADD $now -1 days]
SET lasthour [CLOCK ADD $now -3600 seconds]
```

### Timing Code Execution
```bcl
SET start [CLOCK MICROSECONDS]

# ... code to time ...
FOR 1 TO 1000 DO
    SET x [EXPR $__FOR * 2]
END

SET end [CLOCK MICROSECONDS]
SET elapsed [EXPR $end - $start]
PUTS "Elapsed: $elapsed microseconds"
```

---

## System Commands

### EXEC - Execute system command
```bcl
# Execute command and capture output
SET output [EXEC "ls -la"]
PUTS $output

SET hostname [EXEC "hostname"]
PUTS "Hostname: $hostname"

# Multi-line output
SET files [EXEC "find . -name '*.txt'"]
PUTS "Files:\n$files"
```

### ENV - Access environment variables
```bcl
# Get environment variable
SET home [ENV HOME]
PUTS "Home: $home"

SET path [ENV PATH]
SET user [ENV USER]

# Check if env var exists
IF ![STRING EQUAL [ENV MYVAR] ""] THEN
    PUTS "MYVAR is set"
END
```

### ARGV - Get command-line arguments
```bcl
# Get all arguments as list
SET args [ARGV]
PUTS "Arguments: $args"

# Access individual arguments
SET argc [LLENGTH $args]
IF [EXPR $argc > 0] THEN
    SET firstarg [LINDEX $args 0]
    PUTS "First argument: $firstarg"
END

# Process arguments
FOREACH arg IN $args DO
    PUTS "Processing: $arg"
END
```

### EVAL - Evaluate code string
```bcl
# Execute code from string
EVAL "SET x 10"
EVAL "PUTS $x"

# Build command dynamically
SET cmd "SET result [EXPR 5 + 3]"
EVAL $cmd
PUTS $result                   # Prints: 8

# Sequential evaluation
EVAL "SET a 5"
EVAL "SET b 10"
SET sum [EVAL "EXPR $a + $b"]
PUTS "Sum: $sum"
```

### SOURCE - Load and execute script
```bcl
# Load external script file
SOURCE "config.bcl"            # Executes all commands in config.bcl

# Load library of procedures
SOURCE "mylib.bcl"
myLibFunction arg1 arg2        # Call function from library

# Conditional loading
IF [FILE EXISTS "optional.bcl"] THEN
    SOURCE "optional.bcl"
END
```

### AFTER - Delay execution
```bcl
# Delay in milliseconds
PUTS "Waiting..."
AFTER 1000                     # Wait 1 second
PUTS "Done waiting"

# Delay in loop
FOR 1 TO 5 DO
    PUTS "Tick $__FOR"
    AFTER 500                  # 500ms delay
END
```

---

## Binary Data

### BINARY FORMAT - Pack binary data
```bcl
# Format codes:
#   a  - ASCII string (space-padded)
#   A  - ASCII string (null-padded)
#   c  - Signed 8-bit integer
#   C  - Unsigned 8-bit integer
#   s  - Signed 16-bit integer
#   S  - Unsigned 16-bit integer
#   i  - Signed 32-bit integer
#   I  - Unsigned 32-bit integer
#   f  - Float (32-bit)
#   d  - Double (64-bit)
#   H  - Hex string (high nibble first)
#   h  - Hex string (low nibble first)
#   x  - Null byte
#   X  - Back up one byte
#   @  - Absolute position

# Pack integers
SET data [BINARY FORMAT "ccc" 65 66 67]        # ABC

# Pack with hex
SET data [BINARY FORMAT "H8" "deadbeef"]

# Pack mixed data
SET data [BINARY FORMAT "cisH4" 65 12345 "test" "abcd"]

# Pack with count
SET data [BINARY FORMAT "c3" 1 2 3]            # 3 bytes

# Network byte order (big-endian)
SET data [BINARY FORMAT "I" 0x12345678]
```

### BINARY SCAN - Unpack binary data
```bcl
# Scan bytes
SET data [BINARY FORMAT "ccc" 65 66 67]
BINARY SCAN $data "ccc" byte1 byte2 byte3
PUTS "$byte1 $byte2 $byte3"    # 65 66 67

# Scan integers
SET data [BINARY FORMAT "I" 12345]
BINARY SCAN $data "I" value
PUTS $value                    # 12345

# Scan hex
SET data [BINARY FORMAT "H8" "deadbeef"]
BINARY SCAN $data "H8" hex
PUTS $hex                      # deadbeef

# Scan mixed data
SET data [BINARY FORMAT "cisH4" 65 12345 "test" "abcd"]
BINARY SCAN $data "cisH4" c i s h
PUTS "c=$c, i=$i, s=$s, h=$h"
```

### Binary File I/O
```bcl
# Read binary file
SET fh [OPEN "binary.dat" "r"]
SET data [READ $fh]
CLOSE $fh

# Unpack binary data
BINARY SCAN $data "Ic" version count
PUTS "Version: $version, Count: $count"

# Write binary file
SET data [BINARY FORMAT "IcH8" 1 42 "deadbeef"]
SET fh [OPEN "output.dat" "w"]
PUTS $fh $data
CLOSE $fh
```

---

## Introspection

### INFO EXISTS - Check if variable exists
```bcl
SET myvar 123
IF [INFO EXISTS myvar] THEN
    PUTS "myvar exists"
END

IF ![INFO EXISTS notexist] THEN
    PUTS "notexist doesn't exist"
END
```

### INFO VARS - List all variables (global)
```bcl
SET allvars [INFO VARS]
PUTS "Variables: $allvars"

# With pattern
SET tvars [INFO VARS "t*"]     # All vars starting with 't'
```

### INFO GLOBALS - List global variables
```bcl
SET globals [INFO GLOBALS]
FOREACH var IN $globals DO
    PUTS "$var = [SET $var]"
END
```

### INFO LOCALS - List local variables (in procedure)
```bcl
PROC showLocals WITH a b DO
    SET local1 10
    SET local2 20

    SET locals [INFO LOCALS]
    PUTS "Local variables: $locals"
    # Prints: a b local1 local2
END

showLocals 1 2
```

### INFO PROCS - List all procedures
```bcl
SET procs [INFO PROCS]
PUTS "Procedures: $procs"

# With pattern
SET testprocs [INFO PROCS "test*"]
```

### INFO COMMANDS - List all built-in commands
```bcl
SET commands [INFO COMMANDS]
PUTS "Available commands: $commands"
```

### INFO BODY - Get procedure body
```bcl
PROC myProc WITH x DO
    SET result [EXPR $x * 2]
    RETURN $result
END

SET body [INFO BODY myProc]
PUTS "Procedure body: $body"
```

---

## Standard Libraries

BCL includes powerful standard libraries for terminal graphics, window management, and matrix operations.

### MATRIX Library (MATRIX.BLB)

MATLAB-style matrix operations with full global array support.

#### Usage
```bcl
SOURCE "lib/MATRIX.BLB"
```

#### Matrix Creation Functions

**MAT_CREATE** - Create matrix metadata
```bcl
MAT_CREATE M 3 4                # Create 3x4 matrix M (metadata only)
PUTS $_mat(M.rows)              # Prints: 3
PUTS $_mat(M.cols)              # Prints: 4
```

**MAT_ZEROS** - Create zero matrix
```bcl
MAT_ZEROS Z 3 3                 # Create 3x3 matrix of zeros
PUTS $Z(0,0)                    # Prints: 0
PUTS $Z(2,2)                    # Prints: 0
```

**MAT_ONES** - Create ones matrix
```bcl
MAT_ONES O 2 3                  # Create 2x3 matrix of ones
MAT_PRINT O
# Output:
# Matrix O (2 x 3):
#   [  1.0000  1.0000  1.0000 ]
#   [  1.0000  1.0000  1.0000 ]
```

**MAT_EYE** - Create identity matrix
```bcl
MAT_EYE I 3                     # Create 3x3 identity matrix
MAT_PRINT I
# Output:
# Matrix I (3 x 3):
#   [  1.0000  0.0000  0.0000 ]
#   [  0.0000  1.0000  0.0000 ]
#   [  0.0000  0.0000  1.0000 ]
```

**MAT_FROM_LIST** - Create from flat list
```bcl
MAT_FROM_LIST A 2 2 "1 2 3 4"
MAT_PRINT A
# Output:
# Matrix A (2 x 2):
#   [  1.0000  2.0000 ]
#   [  3.0000  4.0000 ]
```

**MAT_RAND** - Create random matrix
```bcl
MAT_RAND R 2 2 100              # 2x2 matrix with random values 0-99
MAT_PRINT R
```

#### Matrix Operations

**MAT_ADD** - Matrix addition
```bcl
MAT_FROM_LIST A 2 2 "1 2 3 4"
MAT_FROM_LIST B 2 2 "5 6 7 8"
MAT_ADD A B C                   # C = A + B
MAT_PRINT C
# Output:
# Matrix C (2 x 2):
#   [  6.0000  8.0000 ]
#   [ 10.0000 12.0000 ]
```

**MAT_SUB** - Matrix subtraction
```bcl
MAT_SUB A B D                   # D = A - B
```

**MAT_MUL** - Matrix multiplication
```bcl
MAT_FROM_LIST A 2 2 "1 2 3 4"
MAT_FROM_LIST B 2 2 "5 6 7 8"
MAT_MUL A B C                   # C = A * B (matrix multiplication)
MAT_PRINT C
# Output:
# Matrix C (2 x 2):
#   [ 19.0000 22.0000 ]
#   [ 43.0000 50.0000 ]
```

**MAT_SCALAR_MUL** - Scalar multiplication
```bcl
MAT_FROM_LIST A 2 2 "1 2 3 4"
MAT_SCALAR_MUL A 2 B            # B = A * 2
MAT_PRINT B
# Output:
# Matrix B (2 x 2):
#   [  2.0000  4.0000 ]
#   [  6.0000  8.0000 ]
```

**MAT_ELEM_MUL** - Element-wise multiplication
```bcl
MAT_ELEM_MUL A B C              # C(i,j) = A(i,j) * B(i,j)
```

**MAT_TRANSPOSE** - Matrix transpose
```bcl
MAT_FROM_LIST A 2 3 "1 2 3 4 5 6"
MAT_TRANSPOSE A A_T             # A_T is 3x2
MAT_PRINT A_T
```

#### Matrix Analysis

**MAT_SUM** - Sum of all elements
```bcl
MAT_FROM_LIST A 2 2 "1 2 3 4"
MAT_SUM A total                 # total = 10
PUTS "Sum: $total"
```

**MAT_MEAN** - Mean of all elements
```bcl
MAT_MEAN A average              # average = 2.5
PUTS "Mean: $average"
```

**MAT_MIN** / **MAT_MAX** - Minimum/maximum element
```bcl
MAT_MIN A min_val
MAT_MAX A max_val
PUTS "Range: $min_val to $max_val"
```

**MAT_TRACE** - Trace (sum of diagonal)
```bcl
MAT_TRACE A tr
PUTS "Trace: $tr"
```

**MAT_DET_2X2** - Determinant (2x2 only)
```bcl
MAT_DET_2X2 A det
PUTS "Determinant: $det"
```

#### Matrix Utilities

**MAT_PRINT** - Print formatted matrix
```bcl
MAT_PRINT M                     # Pretty-print matrix
```

**MAT_COPY** - Copy matrix
```bcl
MAT_COPY A B                    # B = copy of A
```

**MAT_FILL** - Fill with value
```bcl
MAT_FILL M 42                   # Set all elements to 42
```

**MAT_GET_ROW** / **MAT_GET_COL** - Extract row/column
```bcl
MAT_GET_ROW A 0 row_data        # row_data = list of row 0 elements
MAT_GET_COL A 1 col_data        # col_data = list of column 1 elements
```

### ANSI Library (ANSI.BLB)

Terminal control and Unicode graphics.

#### Usage
```bcl
SOURCE "lib/ANSI.BLB"
```

#### Color Constants

**Foreground Colors**
```bcl
ANSI_FG_BLACK, ANSI_FG_RED, ANSI_FG_GREEN, ANSI_FG_YELLOW
ANSI_FG_BLUE, ANSI_FG_MAGENTA, ANSI_FG_CYAN, ANSI_FG_WHITE

ANSI_FG_BRIGHT_BLACK, ANSI_FG_BRIGHT_RED, ANSI_FG_BRIGHT_GREEN
ANSI_FG_BRIGHT_YELLOW, ANSI_FG_BRIGHT_BLUE, ANSI_FG_BRIGHT_MAGENTA
ANSI_FG_BRIGHT_CYAN, ANSI_FG_BRIGHT_WHITE
```

**Background Colors**
```bcl
ANSI_BG_BLACK, ANSI_BG_RED, ANSI_BG_GREEN, ANSI_BG_YELLOW
ANSI_BG_BLUE, ANSI_BG_MAGENTA, ANSI_BG_CYAN, ANSI_BG_WHITE

ANSI_BG_BRIGHT_BLACK, ANSI_BG_BRIGHT_RED, ANSI_BG_BRIGHT_GREEN
ANSI_BG_BRIGHT_YELLOW, ANSI_BG_BRIGHT_BLUE, ANSI_BG_BRIGHT_MAGENTA
ANSI_BG_BRIGHT_CYAN, ANSI_BG_BRIGHT_WHITE
```

#### Unicode Box Drawing Characters (v2.0)

**Single Line**
```bcl
ANSI_BOX_TL    # ┌ top-left
ANSI_BOX_TR    # ┐ top-right
ANSI_BOX_BL    # └ bottom-left
ANSI_BOX_BR    # ┘ bottom-right
ANSI_BOX_H     # ─ horizontal
ANSI_BOX_V     # │ vertical
```

**Double Line**
```bcl
ANSI_BOX_D_TL  # ╔ top-left
ANSI_BOX_D_TR  # ╗ top-right
ANSI_BOX_D_BL  # ╚ bottom-left
ANSI_BOX_D_BR  # ╝ bottom-right
ANSI_BOX_D_H   # ═ horizontal
ANSI_BOX_D_V   # ║ vertical
```

**Rounded**
```bcl
ANSI_BOX_R_TL  # ╭ top-left
ANSI_BOX_R_TR  # ╮ top-right
ANSI_BOX_R_BL  # ╰ bottom-left
ANSI_BOX_R_BR  # ╯ bottom-right
```

**Other Unicode Characters**
```bcl
ANSI_BLOCK_FULL   # █ full block
ANSI_BLOCK_LIGHT  # ░ light shade
ANSI_BLOCK_MEDIUM # ▒ medium shade
ANSI_BLOCK_DARK   # ▓ dark shade

ANSI_ARROW_UP     # ↑
ANSI_ARROW_DOWN   # ↓
ANSI_ARROW_LEFT   # ←
ANSI_ARROW_RIGHT  # →

ANSI_CHECKMARK    # ✓
ANSI_CROSSMARK    # ✗
ANSI_BULLET       # •
ANSI_STAR         # ★

ANSI_SPIN_0       # | spinner frames
ANSI_SPIN_1       # /
ANSI_SPIN_2       # -
ANSI_SPIN_3       # \
```

#### Terminal Control Functions

**ANSI_CLEAR** - Clear screen
```bcl
ANSI_CLEAR
```

**ANSI_CURSOR_GOTO** - Move cursor
```bcl
ANSI_CURSOR_GOTO 10 20          # Move to row 10, column 20
```

**ANSI_CURSOR_HOME** - Move to top-left
```bcl
ANSI_CURSOR_HOME
```

**ANSI_CURSOR_HIDE** / **ANSI_CURSOR_SHOW**
```bcl
ANSI_CURSOR_HIDE
# ... do work ...
ANSI_CURSOR_SHOW
```

**ANSI_SET_COLOR** - Set foreground and background
```bcl
ANSI_SET_COLOR $ANSI_FG_BRIGHT_WHITE $ANSI_BG_BLUE
PUTS "Colored text"
ANSI_RESET
```

**ANSI_SET_STYLE** - Set text style
```bcl
# Style constants:
# ANSI_BOLD, ANSI_DIM, ANSI_ITALIC, ANSI_UNDERLINE
# ANSI_BLINK, ANSI_REVERSE, ANSI_HIDDEN

ANSI_SET_STYLE $ANSI_BOLD
PUTS "Bold text"
ANSI_RESET
```

**ANSI_RESET** - Reset all formatting
```bcl
ANSI_RESET
```

**Example: Colored Box**
```bcl
SOURCE "lib/ANSI.BLB"

ANSI_CLEAR
ANSI_CURSOR_GOTO 5 10
ANSI_SET_COLOR $ANSI_FG_BRIGHT_CYAN $ANSI_BG_BLACK

# Draw box
PUTS -NONEWLINE $ANSI_BOX_TL
FOR 0 TO 38 DO
    PUTS -NONEWLINE $ANSI_BOX_H
END
PUTS $ANSI_BOX_TR

ANSI_CURSOR_GOTO 6 10
PUTS -NONEWLINE $ANSI_BOX_V
ANSI_CURSOR_GOTO 6 50
PUTS $ANSI_BOX_V

ANSI_CURSOR_GOTO 7 10
PUTS -NONEWLINE $ANSI_BOX_BL
FOR 0 TO 38 DO
    PUTS -NONEWLINE $ANSI_BOX_H
END
PUTS $ANSI_BOX_BR

ANSI_RESET
```

### WINDOW Library (WINDOW.BLB)

Advanced window management with Unicode borders and global array persistence.

#### Usage
```bcl
SOURCE "lib/WINDOW.BLB"    # Automatically sources ANSI.BLB
WIN_INIT                   # Initialize window system
```

#### Window Creation

**WIN_CREATE** - Create a window
```bcl
WIN_CREATE id row col width height title
# id:     Window identifier (0-9)
# row:    Top-left row position
# col:    Top-left column position
# width:  Window width (including border)
# height: Window height (including border)
# title:  Window title string

# Example:
WIN_CREATE 0 5 10 40 10 "My Window"
WIN_DRAW 0
```

**WIN_DRAW** - Draw window with current settings
```bcl
WIN_DRAW 0
```

#### Window Styling

**WIN_SET_BORDER_STYLE** - Set border type
```bcl
WIN_SET_BORDER_STYLE id style
# style: 0 = single line (┌─┐)
#        1 = double line (╔═╗)
#        2 = rounded     (╭─╮)

WIN_SET_BORDER_STYLE 0 1      # Double line border
```

**WIN_SET_COLOR** - Set window content colors
```bcl
WIN_SET_COLOR id fg bg

WIN_SET_COLOR 0 $ANSI_FG_BRIGHT_WHITE $ANSI_BG_BLUE
```

**WIN_SET_BORDER_COLOR** - Set border colors
```bcl
WIN_SET_BORDER_COLOR id fg bg

WIN_SET_BORDER_COLOR 0 $ANSI_FG_BRIGHT_CYAN $ANSI_BG_BLUE
```

#### Window Content

**WIN_PRINT** - Print text at row
```bcl
WIN_PRINT id row text

WIN_PRINT 0 1 "Hello, World!"
WIN_PRINT 0 2 "Line 2"
```

**WIN_PRINT_CENTER** - Print centered text
```bcl
WIN_PRINT_CENTER id row text

WIN_PRINT_CENTER 0 5 "Centered Text"
```

**WIN_CLEAR** - Clear window content
```bcl
WIN_CLEAR 0
```

#### Window State

**WIN_HIDE** / **WIN_SHOW** - Hide/show window
```bcl
WIN_HIDE 0
# ... window is hidden ...
WIN_SHOW 0        # Redraws automatically
```

**WIN_MOVE** - Move window
```bcl
WIN_MOVE id new_row new_col

WIN_MOVE 0 10 20
WIN_DRAW 0        # Redraw at new position
```

#### Advanced Features

**WIN_MENU** - Draw menu with selection
```bcl
SET items "Option 1|Option 2|Option 3|Exit"
SET selected 1

WIN_MENU 0 $items $selected
# Displays menu with item 1 highlighted
```

**WIN_PROGRESS** - Draw progress bar
```bcl
WIN_PROGRESS id row percent

FOR 0 TO 10 DO
    SET pct [EXPR $__FOR * 10]
    WIN_PROGRESS 0 3 $pct
END
```

**WIN_BUTTON** - Draw button
```bcl
WIN_BUTTON id row col label

WIN_BUTTON 0 8 10 " OK "
WIN_BUTTON 0 8 20 " Cancel "
```

**WIN_HLINE** - Draw horizontal line
```bcl
WIN_HLINE 0 5      # Draw line at row 5
```

**WIN_MESSAGE_BOX** - Simple message dialog
```bcl
WIN_MESSAGE_BOX "Operation completed successfully!"
```

#### Scrollable Content

**WIN_ADD_LINE** - Add line to content buffer
```bcl
WIN_ADD_LINE 0 "Line 1"
WIN_ADD_LINE 0 "Line 2"
WIN_ADD_LINE 0 "Line 3"
# ... many more lines ...
```

**WIN_REDRAW_CONTENT** - Display buffered content
```bcl
WIN_REDRAW_CONTENT 0       # Shows visible lines
```

**WIN_SCROLL** - Scroll content
```bcl
WIN_SCROLL id direction
# direction: -1 = scroll up, 1 = scroll down

WIN_SCROLL 0 1             # Scroll down one line
```

#### System Management

**WIN_CLEAR_SCREEN** - Clear entire screen
```bcl
WIN_CLEAR_SCREEN
```

**WIN_CLEANUP** - Cleanup and restore terminal
```bcl
WIN_CLEANUP                # Call before exiting
```

#### Complete Example
```bcl
#!/usr/bin/env bcl
SOURCE "lib/WINDOW.BLB"

WIN_INIT

# Create main window
WIN_CREATE 0 3 5 60 15 "Application Title"
WIN_SET_BORDER_STYLE 0 1
WIN_SET_COLOR 0 $ANSI_FG_BRIGHT_WHITE $ANSI_BG_BLUE
WIN_SET_BORDER_COLOR 0 $ANSI_FG_BRIGHT_CYAN $ANSI_BG_BLUE
WIN_DRAW 0

# Add content
WIN_PRINT_CENTER 0 2 "Welcome to BCL Windows"
WIN_PRINT 0 4 "  Select an option:"
WIN_PRINT 0 5 ""

# Menu
SET menu "New|Open|Save|Exit"
WIN_MENU 0 $menu 1

# Progress bar
WIN_PRINT 0 10 "  Loading..."
WIN_PROGRESS 0 11 75

# Buttons
WIN_HLINE 0 12
WIN_BUTTON 0 13 15 " OK "
WIN_BUTTON 0 13 30 " Cancel "

# Wait for input
ANSI_CURSOR_GOTO 24 1
PUTS -NONEWLINE "Press ENTER..."
GETS input

# Cleanup
WIN_CLEANUP
```

---

## Common Patterns

### File Processing Line-by-Line
```bcl
PROC processFile WITH filename DO
    IF ![FILE EXISTS $filename] THEN
        PUTS "Error: File not found: $filename"
        RETURN
    END

    SET fh [OPEN $filename "r"]
    SET linenum 0

    WHILE ![EOF $fh] DO
        SET line [GETS $fh]
        INCR linenum

        # Process line
        PUTS "$linenum: $line"
    END

    CLOSE $fh
    PUTS "Processed $linenum lines"
END

processFile "data.txt"
```

### Configuration File Parser
```bcl
PROC loadConfig WITH filename DO
    SET fh [OPEN $filename "r"]

    WHILE ![EOF $fh] DO
        SET line [GETS $fh]
        SET line [STRING TRIM $line]

        # Skip comments and empty lines
        IF [STRING EQUAL $line ""] THEN
            CONTINUE
        END
        IF [STRING MATCH "#*" $line] THEN
            CONTINUE
        END

        # Parse key=value
        IF [REGEXP "^(\\w+)=(.*)$" $line full key value] THEN
            SET config($key) $value
            PUTS "Config: $key = $value"
        END
    END

    CLOSE $fh
END

loadConfig "config.ini"
```

### Data Validation
```bcl
PROC validateEmail WITH email DO
    SET pattern "^\\w+@\\w+\\.\\w+$"
    IF [REGEXP $pattern $email] THEN
        RETURN 1
    ELSE
        RETURN 0
    END
END

IF [validateEmail "user@example.com"] THEN
    PUTS "Valid email"
ELSE
    PUTS "Invalid email"
END
```

### Simple CSV Parser
```bcl
PROC parseCSV WITH filename DO
    SET fh [OPEN $filename "r"]
    SET rownum 0

    WHILE ![EOF $fh] DO
        SET line [GETS $fh]
        INCR rownum

        SET fields [SPLIT $line ","]
        SET fieldcount [LLENGTH $fields]

        PUTS "Row $rownum ($fieldcount fields):"
        SET colnum 0
        FOREACH field IN $fields DO
            INCR colnum
            SET trimmed [STRING TRIM $field]
            PUTS "  Col $colnum: $trimmed"
        END
    END

    CLOSE $fh
END

parseCSV "data.csv"
```

### Recursive Directory Walker
```bcl
PROC walkDirectory WITH dir DO
    PUTS "Directory: $dir"

    SET files [GLOB "$dir/*"]
    FOREACH file IN $files DO
        SET type [FILE TYPE $file]

        IF [STRING EQUAL $type "file"] THEN
            PUTS "  File: $file"
        ELSEIF [STRING EQUAL $type "directory"] THEN
            PUTS "  Subdir: $file"
            walkDirectory $file
        END
    END
END

walkDirectory "/home/user"
```

### Command-Line Argument Parsing
```bcl
SET args [ARGV]
SET verbose 0
SET inputfile ""
SET outputfile ""

SET i 0
WHILE [EXPR $i < [LLENGTH $args]] DO
    SET arg [LINDEX $args $i]

    IF [STRING EQUAL $arg "-v"] THEN
        SET verbose 1
    ELSEIF [STRING EQUAL $arg "-i"] THEN
        INCR i
        SET inputfile [LINDEX $args $i]
    ELSEIF [STRING EQUAL $arg "-o"] THEN
        INCR i
        SET outputfile [LINDEX $args $i]
    ELSE
        PUTS "Unknown option: $arg"
        EXIT 1
    END

    INCR i
END

IF [EXPR $verbose] THEN
    PUTS "Verbose mode enabled"
    PUTS "Input: $inputfile"
    PUTS "Output: $outputfile"
END
```

### Error Handling Pattern
```bcl
PROC safeDivide WITH a b DO
    IF [EXPR $b == 0] THEN
        PUTS "Error: Division by zero"
        RETURN "ERROR"
    END

    RETURN [EXPR $a / $b]
END

SET result [safeDivide 10 2]
IF [STRING EQUAL $result "ERROR"] THEN
    PUTS "Division failed"
ELSE
    PUTS "Result: $result"
END
```

### Caching/Memoization
```bcl
# Global cache array
PROC fibonacci WITH n DO
    # Check cache
    IF [ARRAY EXISTS fibcache] THEN
        IF [INFO EXISTS fibcache($n)] THEN
            RETURN $fibcache($n)
        END
    END

    # Calculate
    IF [EXPR $n <= 1] THEN
        SET result $n
    ELSE
        SET f1 [fibonacci [EXPR $n - 1]]
        SET f2 [fibonacci [EXPR $n - 2]]
        SET result [EXPR $f1 + $f2]
    END

    # Store in cache
    SET fibcache($n) $result
    RETURN $result
END

SET f10 [fibonacci 10]         # Much faster with caching
```

---

## Known Limitations

### STRING LENGTH in REPL
```bcl
# ⚠️ WARNING: STRING LENGTH may segfault in interactive REPL
# ✅ WORKAROUND: Use in scripts only, or avoid in REPL

# Safe in scripts:
SET len [STRING LENGTH $text]

# May crash in REPL:
bcl> STRING LENGTH "hello"     # Risky
```

### ARGV/FILE TYPE with SET Assignment
```bcl
# ⚠️ KNOWN BUG: These fail with SET in command substitution
# SET args [ARGV]              # FAILS - "can't read var"
# SET type [FILE TYPE $file]   # FAILS - "can't read var"

# ✅ WORKAROUND: Use PUTS instead
PUTS [ARGV]                    # Works
PUTS [FILE TYPE $file]         # Works
```

### Negative List Indices
```bcl
# ⚠️ NOT SUPPORTED: Negative indices don't work
# SET last [LINDEX $list -1]   # Does NOT work

# ✅ WORKAROUND: Calculate from length
SET len [LLENGTH $list]
SET last [LINDEX $list [EXPR $len - 1]]
```

### FILE COPY Not Implemented
```bcl
# ⚠️ NOT AVAILABLE: FILE COPY command doesn't exist
# FILE COPY "source.txt" "dest.txt"  # Not implemented

# ✅ WORKAROUND: Read and write
SET fh1 [OPEN "source.txt" "r"]
SET content [READ $fh1]
CLOSE $fh1

SET fh2 [OPEN "dest.txt" "w"]
PUTS $fh2 $content
CLOSE $fh2
```

### Expression Wrapping in Conditionals
```bcl
# ⚠️ WRONG: Direct comparison doesn't work
# IF [STRING LENGTH $text >= 5] THEN    # WRONG!

# ✅ CORRECT: Wrap comparisons in EXPR
IF [EXPR [STRING LENGTH $text] >= 5] THEN
    PUTS "Text is long enough"
END

# Same for LSEARCH, LLENGTH, etc.
IF [EXPR [LSEARCH $list "item"] >= 0] THEN
    PUTS "Found"
END
```

### Case Sensitivity
```bcl
# Variables are CASE-INSENSITIVE
SET MyVar 10
SET myvar 20                   # Same variable! myvar = 20

# Array keys are CASE-INSENSITIVE
SET arr(Key) "value1"
SET arr(key) "value2"          # Same key! arr(key) = "value2"

# Be consistent with naming to avoid confusion
```

### No Exception Handling
```bcl
# ⚠️ NO TRY/CATCH: BCL doesn't have exception handling
# Errors terminate script immediately

# ✅ WORKAROUND: Check before operations
IF [FILE EXISTS $filename] THEN
    SET fh [OPEN $filename "r"]
    # ... safe to proceed ...
    CLOSE $fh
ELSE
    PUTS "Error: File not found"
    EXIT 1
END
```

---

## Quick Reference Summary

### Variable Operations
```bcl
SET var value                  # Assign
SET var                        # Retrieve
UNSET var                      # Delete
INCR var ?amount?              # Increment
APPEND var val1 val2...        # Append
GLOBAL var                     # Access global from proc
```

### Control Structures
```bcl
IF cond THEN ... ELSEIF cond THEN ... ELSE ... END
WHILE cond DO ... END
FOR start TO end ?STEP n? DO ... END
FOREACH var IN list DO ... END
SWITCH value DO CASE x ... DEFAULT ... END
BREAK / CONTINUE / RETURN ?value? / EXIT ?code?
```

### Expressions
```bcl
EXPR expression                # Evaluate math/logic
# Operators: + - * / % ** == != < > <= >= && || !
# Functions: sin cos tan sqrt abs min max floor ceil round rand ...
```

### Lists
```bcl
LIST e1 e2 ...                 # Create list
SPLIT string ?sep?             # String to list
JOIN list ?sep?                # List to string
LINDEX list index              # Get element
LRANGE list first last         # Extract range
LLENGTH list                   # Get length
LAPPEND varname val...         # Append to list
LINSERT list index val...      # Insert at position
LREPLACE list first last ?val...? # Replace range
CONCAT list1 list2 ...         # Concatenate lists
LSORT list                     # Sort list
LSEARCH list value             # Find element (-1 if not found)
```

### Strings
```bcl
STRING LENGTH string
STRING INDEX string index
STRING RANGE string first last
STRING TOUPPER / TOLOWER / TOTITLE string
STRING TRIM / TRIMLEFT / TRIMRIGHT string ?chars?
STRING COMPARE ?-NOCASE? s1 s2
STRING EQUAL ?-NOCASE? s1 s2
STRING MATCH ?-NOCASE? pattern string
STRING REPLACE string old new
STRING REPEAT string count
STRING REVERSE string
STRING MAP mapping string
STRING FIRST / LAST needle haystack ?start?
```

### Arrays
```bcl
SET array(key) value           # Set element
$array(key)                    # Get element
ARRAY EXISTS arrayname
ARRAY SIZE arrayname
ARRAY NAMES arrayname ?pattern?
ARRAY GET arrayname
ARRAY SET arrayname list
ARRAY UNSET arrayname ?key?
```

### Files
```bcl
OPEN filename mode             # Returns handle
CLOSE handle
READ handle ?numBytes?
GETS handle                    # Read line
PUTS ?-NONEWLINE? ?handle? string
TELL handle
SEEK handle offset ?origin?   # origin: START/CURRENT/END
EOF handle
FLUSH handle
FILE EXISTS / SIZE / TYPE / DELETE / RENAME / MTIME filename
PWD
GLOB pattern
```

### Regex
```bcl
REGEXP ?-NOCASE? pattern string ?matchVar? ?subMatch...?
REGSUB ?-NOCASE? ?-ALL? pattern string replacement
```

### Time/Date
```bcl
CLOCK SECONDS / MILLISECONDS / MICROSECONDS
CLOCK FORMAT timestamp format
CLOCK SCAN datestring format
CLOCK ADD timestamp value unit
```

### System
```bcl
EXEC command                   # Execute and capture output
ENV varname                    # Get environment variable
ARGV                           # Get command-line arguments
EVAL codestring                # Evaluate code
SOURCE filename                # Load script
AFTER milliseconds             # Delay
```

### Binary
```bcl
BINARY FORMAT formatstring value...
BINARY SCAN data formatstring varname...
```

### Introspection
```bcl
INFO EXISTS / VARS / GLOBALS / LOCALS / PROCS / COMMANDS / BODY
```

### Procedures
```bcl
PROC name WITH ?arg1 arg2...? DO
    # body
    RETURN ?value?
END
```

---

## Complete Example Program

```bcl
#!/usr/bin/env bcl
# Complete example: Log file analyzer

PROC analyzeLogs WITH logfile DO
    # Check if file exists
    IF ![FILE EXISTS $logfile] THEN
        PUTS "Error: Log file not found: $logfile"
        RETURN
    END

    # Initialize counters
    SET totalLines 0
    SET errorCount 0
    SET warningCount 0
    SET infoCount 0

    # Arrays to track unique IPs and users
    SET uniqueIPs(count) 0
    SET uniqueUsers(count) 0

    # Open file
    SET fh [OPEN $logfile "r"]

    # Process each line
    WHILE ![EOF $fh] DO
        SET line [GETS $fh]
        INCR totalLines

        # Count log levels
        IF [STRING MATCH "*ERROR*" $line] THEN
            INCR errorCount
        ELSEIF [STRING MATCH "*WARNING*" $line] THEN
            INCR warningCount
        ELSEIF [STRING MATCH "*INFO*" $line] THEN
            INCR infoCount
        END

        # Extract IP addresses (simple pattern)
        IF [REGEXP "\\d+\\.\\d+\\.\\d+\\.\\d+" $line ip] THEN
            IF ![INFO EXISTS seenIP($ip)] THEN
                SET seenIP($ip) 1
            END
        END

        # Extract usernames (assuming format user=NAME)
        IF [REGEXP "user=(\\w+)" $line full username] THEN
            IF ![INFO EXISTS seenUser($username)] THEN
                SET seenUser($username) 1
            END
        END
    END

    CLOSE $fh

    # Calculate unique counts
    IF [ARRAY EXISTS seenIP] THEN
        SET uniqueIPCount [ARRAY SIZE seenIP]
    ELSE
        SET uniqueIPCount 0
    END

    IF [ARRAY EXISTS seenUser] THEN
        SET uniqueUserCount [ARRAY SIZE seenUser]
    ELSE
        SET uniqueUserCount 0
    END

    # Generate report
    PUTS "=========================================="
    PUTS "Log Analysis Report"
    PUTS "=========================================="
    PUTS "File: $logfile"
    PUTS "Size: [FILE SIZE $logfile] bytes"
    PUTS ""
    PUTS "Total Lines: $totalLines"
    PUTS "Errors:      $errorCount"
    PUTS "Warnings:    $warningCount"
    PUTS "Info:        $infoCount"
    PUTS ""
    PUTS "Unique IPs:  $uniqueIPCount"
    PUTS "Unique Users: $uniqueUserCount"
    PUTS "=========================================="

    # Calculate percentages
    IF [EXPR $totalLines > 0] THEN
        SET errorPct [EXPR ($errorCount * 100.0) / $totalLines]
        SET warningPct [EXPR ($warningCount * 100.0) / $totalLines]
        SET infoPct [EXPR ($infoCount * 100.0) / $totalLines]

        PUTS "Error Rate:   [FORMAT "%.2f" $errorPct]%"
        PUTS "Warning Rate: [FORMAT "%.2f" $warningPct]%"
        PUTS "Info Rate:    [FORMAT "%.2f" $infoPct]%"
    END
END

# Main program
SET args [ARGV]
SET argc [LLENGTH $args]

IF [EXPR $argc < 1] THEN
    PUTS "Usage: bcl loganalyzer.bcl <logfile>"
    EXIT 1
END

SET logfile [LINDEX $args 0]
analyzeLogs $logfile

EXIT 0
```

---

## Tips for LLM Code Generation

1. **Always wrap comparisons in EXPR**
   - `IF [EXPR [STRING LENGTH $x] > 5]` not `IF [STRING LENGTH $x > 5]`

2. **Use proper quoting for multi-word strings**
   - `SET msg "Hello World"` not `SET msg Hello World`

3. **Check file existence before operations**
   - Always use `FILE EXISTS` before `OPEN`

4. **Close file handles**
   - Always `CLOSE` after `OPEN`

5. **Initialize variables before INCR/APPEND**
   - Or rely on auto-initialization (INCR starts at 0, APPEND at "")

6. **Use INFO EXISTS before accessing variables**
   - Prevents errors from undefined variables

7. **Prefer FOR over WHILE when iteration count is known**
   - More readable and less error-prone

8. **Use procedures for reusable code**
   - Keeps code organized and maintainable

9. **Remember case-insensitivity**
   - Variables and array keys are case-insensitive

10. **Test patterns before using in conditionals**
    - Use simple test first: `IF [REGEXP pattern $text]`

11. **Use GLOBAL with arrays in procedures (v2.0+)**
    - `GLOBAL arrayname` marks entire array as global
    - Works with both existing and new arrays
    - Essential for library procedures that create arrays

12. **Use Unicode for better terminal graphics (v2.0+)**
    - `\uXXXX` for 4-digit Unicode (e.g., `"\u250C"` = ┌)
    - `\UXXXXXXXX` for 8-digit Unicode (e.g., `"\U0001F600"` = 😀)
    - ANSI.BLB provides 60+ Unicode constants for box drawing

13. **Use standard libraries for common tasks**
    - `SOURCE "lib/MATRIX.BLB"` for matrix operations
    - `SOURCE "lib/WINDOW.BLB"` for terminal UI
    - `SOURCE "lib/ANSI.BLB"` for terminal control

---

## What's New in v2.0

### Array Global Prefix System
- Arrays now work correctly with GLOBAL in procedures
- Single `GLOBAL arrayname` declaration makes all array elements global
- Critical for libraries that manage shared data structures
- See "Array Global Scope" section for details

### Unicode Support
- Full UTF-8 Unicode escape sequences: `\uXXXX` and `\UXXXXXXXX`
- ANSI library includes 60+ Unicode character constants
- Box drawing characters (single, double, rounded styles)
- Symbols: arrows, checkmarks, blocks, spinners
- See "Escape Sequences" section for details

### Enhanced Standard Libraries
- **MATRIX.BLB v2.0.0** - New MATLAB-style matrix library
  - 21 functions for creation, operations, and analysis
  - Works correctly with global arrays
- **ANSI.BLB v2.0.0** - Unicode graphics support
  - 60+ Unicode character constants
  - Box drawing, arrows, blocks, symbols
- **WINDOW.BLB v2.0.0** - Complete rewrite
  - Unicode borders (single, double, rounded)
  - Menus, progress bars, buttons, scrolling
  - All windows persist correctly with global arrays

---

**End of BCL Language Reference for LLM Code Generation**
**Version 2.0.0 | Now with Array Global Support, Unicode, and Enhanced Libraries**
