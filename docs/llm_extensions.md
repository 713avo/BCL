# BCL Extension Development Guide for LLMs

This document provides a concise API reference for Large Language Models to programmatically create BCL extensions.

## Overview

BCL extensions are C shared libraries (.so files) that extend the interpreter with new commands. Extensions are loaded at runtime using the `LOAD` command.

## Quick Start

### Minimal Extension Template

```c
#include <tcl.h>
#include <string.h>

// Command implementation
static int MyCommand(ClientData clientData, Tcl_Interp *interp, 
                     int argc, const char *argv[]) {
    if (argc != 2) {
        Tcl_SetResult(interp, "Usage: MYCOMMAND arg", TCL_STATIC);
        return TCL_ERROR;
    }
    
    // Your logic here
    const char *arg = argv[1];
    
    // Set result
    Tcl_SetResult(interp, "Success", TCL_STATIC);
    return TCL_OK;
}

// Extension initialization (REQUIRED)
int bcl_extension_init(Tcl_Interp *interp) {
    // Register commands (uppercase names)
    Tcl_CreateCommand(interp, "MYCOMMAND", MyCommand, NULL, NULL);
    
    return TCL_OK;  // Must return TCL_OK on success
}
```

### Building

```makefile
# Makefile
CC = gcc
CFLAGS = -fPIC -Wall -O2 -I../include

myext.so: myext.c
	$(CC) $(CFLAGS) -shared -o $@ $<

clean:
	rm -f *.so
```

```bash
make myext.so
```

### Usage in BCL

```bcl
LOAD "path/to/myext.so"
MYCOMMAND "test"
```

## Core API Reference

### Extension Entry Point

```c
int bcl_extension_init(Tcl_Interp *interp);
```

**Must be defined.** Called when extension is loaded. Return `TCL_OK` on success, `TCL_ERROR` on failure.

### Command Registration

```c
Tcl_Command Tcl_CreateCommand(
    Tcl_Interp *interp,
    const char *cmdName,        // Uppercase command name
    Tcl_CmdProc *proc,          // Command implementation
    ClientData clientData,       // Custom data (usually NULL)
    Tcl_CmdDeleteProc *deleteProc  // Cleanup callback (usually NULL)
);
```

**Example:**
```c
Tcl_CreateCommand(interp, "SOCKET", Socket_Cmd, NULL, NULL);
```

### Command Implementation Signature

```c
int CommandProc(
    ClientData clientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]
);
```

**Parameters:**
- `clientData`: Custom data passed during registration
- `interp`: Interpreter instance
- `argc`: Argument count (includes command name)
- `argv`: Argument array (`argv[0]` is command name)

**Return:** `TCL_OK` or `TCL_ERROR`

### Setting Results

```c
// Static string (must be constant or static)
Tcl_SetResult(interp, "Result", TCL_STATIC);

// Dynamic string (will be freed by Tcl)
char *result = strdup("Dynamic result");
Tcl_SetResult(interp, result, TCL_DYNAMIC);

// Volatile string (will be copied)
char buffer[256];
snprintf(buffer, sizeof(buffer), "Value: %d", 42);
Tcl_SetResult(interp, buffer, TCL_VOLATILE);
```

### Error Handling

```c
// Set error message and return
Tcl_SetResult(interp, "Error: invalid argument", TCL_STATIC);
return TCL_ERROR;

// Formatted error
char err[256];
snprintf(err, sizeof(err), "Error: expected number, got '%s'", argv[1]);
Tcl_SetResult(interp, err, TCL_VOLATILE);
return TCL_ERROR;
```

### Argument Parsing

```c
// Get integer
int value;
if (Tcl_GetInt(interp, argv[1], &value) != TCL_OK) {
    return TCL_ERROR;  // Error message already set
}

// Get double
double dvalue;
if (Tcl_GetDouble(interp, argv[1], &dvalue) != TCL_OK) {
    return TCL_ERROR;
}

// String argument (no conversion needed)
const char *str = argv[1];

// Check argument count
if (argc != 3) {
    Tcl_SetResult(interp, "Usage: COMMAND arg1 arg2", TCL_STATIC);
    return TCL_ERROR;
}
```

## File Descriptors and Event Integration

Extensions that work with file descriptors (sockets, files, GPIO, etc.) should return the FD as an integer so it can be used with the EVENT system.

### Example: Returning File Descriptor

```c
static int Socket_Create(ClientData clientData, Tcl_Interp *interp,
                        int argc, const char *argv[]) {
    // Create socket
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        Tcl_SetResult(interp, "Failed to create socket", TCL_STATIC);
        return TCL_ERROR;
    }
    
    // Return FD as string
    char result[32];
    snprintf(result, sizeof(result), "%d", fd);
    Tcl_SetResult(interp, result, TCL_VOLATILE);
    
    return TCL_OK;
}
```

### Using with EVENT System

```bcl
# Extension returns FD
SET fd [SOCKET CREATE]

# Register event handler
PROC ON_READ WITH fd DO
    SET data [SOCKET RECV $fd 1024]
    PUTS "Received: $data"
END

EVENT CREATE $fd READABLE ON_READ
EVENT LOOP
```

## Complete Example: Simple Extension

### Code: math_ext.c

```c
#include <tcl.h>
#include <math.h>
#include <stdio.h>

// SQRT command
static int Math_Sqrt(ClientData clientData, Tcl_Interp *interp,
                    int argc, const char *argv[]) {
    if (argc != 2) {
        Tcl_SetResult(interp, "Usage: SQRT number", TCL_STATIC);
        return TCL_ERROR;
    }
    
    double value;
    if (Tcl_GetDouble(interp, argv[1], &value) != TCL_OK) {
        return TCL_ERROR;
    }
    
    if (value < 0) {
        Tcl_SetResult(interp, "Error: negative number", TCL_STATIC);
        return TCL_ERROR;
    }
    
    double result = sqrt(value);
    
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%.10g", result);
    Tcl_SetResult(interp, buffer, TCL_VOLATILE);
    
    return TCL_OK;
}

// POW command
static int Math_Pow(ClientData clientData, Tcl_Interp *interp,
                   int argc, const char *argv[]) {
    if (argc != 3) {
        Tcl_SetResult(interp, "Usage: POW base exponent", TCL_STATIC);
        return TCL_ERROR;
    }
    
    double base, exponent;
    if (Tcl_GetDouble(interp, argv[1], &base) != TCL_OK ||
        Tcl_GetDouble(interp, argv[2], &exponent) != TCL_OK) {
        return TCL_ERROR;
    }
    
    double result = pow(base, exponent);
    
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%.10g", result);
    Tcl_SetResult(interp, buffer, TCL_VOLATILE);
    
    return TCL_OK;
}

// Extension initialization
int bcl_extension_init(Tcl_Interp *interp) {
    Tcl_CreateCommand(interp, "SQRT", Math_Sqrt, NULL, NULL);
    Tcl_CreateCommand(interp, "POW", Math_Pow, NULL, NULL);
    return TCL_OK;
}
```

### Makefile

```makefile
CC = gcc
CFLAGS = -fPIC -Wall -O2 -I../include

math_ext.so: math_ext.c
	$(CC) $(CFLAGS) -shared -o $@ $< -lm

clean:
	rm -f math_ext.so
```

### Usage

```bcl
LOAD "math_ext.so"

SET result [SQRT 16]
PUTS "sqrt(16) = $result"  # 4

SET result [POW 2 8]
PUTS "2^8 = $result"  # 256
```

## Advanced Patterns

### Subcommands

```c
static int MyExt_Cmd(ClientData clientData, Tcl_Interp *interp,
                    int argc, const char *argv[]) {
    if (argc < 2) {
        Tcl_SetResult(interp, "Usage: MYEXT subcommand ...", TCL_STATIC);
        return TCL_ERROR;
    }
    
    const char *subcmd = argv[1];
    
    if (strcmp(subcmd, "CREATE") == 0) {
        // Handle CREATE subcommand
        return MyExt_Create(interp, argc - 1, argv + 1);
    }
    else if (strcmp(subcmd, "DELETE") == 0) {
        // Handle DELETE subcommand
        return MyExt_Delete(interp, argc - 1, argv + 1);
    }
    else {
        char err[256];
        snprintf(err, sizeof(err), "Unknown subcommand '%s'", subcmd);
        Tcl_SetResult(interp, err, TCL_VOLATILE);
        return TCL_ERROR;
    }
}
```

### Persistent State

```c
// State structure
typedef struct {
    int counter;
    // ... other fields
} ExtensionState;

// Cleanup callback
static void State_Cleanup(ClientData clientData) {
    ExtensionState *state = (ExtensionState *)clientData;
    free(state);
}

// Extension init
int bcl_extension_init(Tcl_Interp *interp) {
    // Allocate state
    ExtensionState *state = malloc(sizeof(ExtensionState));
    state->counter = 0;
    
    // Register command with state
    Tcl_CreateCommand(interp, "MYCOMMAND", My_Cmd, 
                     (ClientData)state, State_Cleanup);
    
    return TCL_OK;
}

// Command using state
static int My_Cmd(ClientData clientData, Tcl_Interp *interp,
                 int argc, const char *argv[]) {
    ExtensionState *state = (ExtensionState *)clientData;
    state->counter++;
    
    char result[64];
    snprintf(result, sizeof(result), "Counter: %d", state->counter);
    Tcl_SetResult(interp, result, TCL_VOLATILE);
    
    return TCL_OK;
}
```

## Best Practices

### 1. Command Naming
- Use **UPPERCASE** for command names
- Use descriptive subcommands (CREATE, DELETE, SEND, RECV, etc.)

### 2. Error Messages
- Always set descriptive error messages
- Include usage information for incorrect argument count
- Use `TCL_VOLATILE` for stack-allocated error buffers

### 3. Return Values
- Return `TCL_OK` on success
- Return `TCL_ERROR` on failure
- Always call `Tcl_SetResult()` before returning

### 4. Memory Management
- Use `TCL_STATIC` for string literals
- Use `TCL_DYNAMIC` for malloc'd strings (Tcl will free)
- Use `TCL_VOLATILE` for stack buffers (Tcl will copy)

### 5. Thread Safety
- BCL is single-threaded
- No need for mutex locks
- Avoid blocking operations in commands

### 6. File Descriptors
- Return FDs as integers for EVENT integration
- Document that FD can be used with EVENT CREATE
- Always close FDs in cleanup

## Reference Implementation

The SOCKET extension (`extensions/socket.c`) is a complete reference implementation showing:
- Subcommand pattern
- File descriptor management
- EVENT system integration
- Error handling
- State management

## Testing Your Extension

```bcl
#!/usr/bin/env bcl

LOAD "myext.so"

# Test basic functionality
PROC TEST WITH name DO
    PUTS "Testing: $name"
END

TEST "Command exists"
# Test your commands here

PUTS "All tests passed!"
```

## Troubleshooting

### Extension Won't Load
- Check that `bcl_extension_init` exists and returns `TCL_OK`
- Verify shared library is compiled correctly (`file myext.so`)
- Check for missing dependencies (`ldd myext.so`)

### Segmentation Fault
- Ensure all pointers are valid
- Check array bounds in argv access
- Verify malloc/free pairs

### Command Not Found
- Verify command name is UPPERCASE in `Tcl_CreateCommand`
- Check that `bcl_extension_init` returns `TCL_OK`

## Additional Resources

- **Full Documentation**: `docs/extensions/main.pdf`
- **Reference Implementation**: `extensions/socket.c`
- **Event System**: `docs/EVENT_SYSTEM.md`
- **Man Pages**: `man 1 bcl`, `man 7 bcl`
- **Examples**: `examples/` directory

---

**Note**: This guide uses Tcl's C API because BCL is built on Tcl 8.x core. All Tcl API functions work in BCL extensions.
