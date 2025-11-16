# BCL Dynamic Extensions

This directory contains dynamically loadable extensions for BCL.

## Available Extensions

### socket.so - TCP/UDP Networking

TCP client/server networking support.

```bcl
LOAD "extensions/socket.so"

# Create server
SET server [SOCKET SERVER 9999]

# Accept connections
SET client [SOCKET ACCEPT $server]

# Send/receive data
SOCKET SEND $client "Hello"
SET data [SOCKET RECV $client 1024]

# Close connection
SOCKET CLOSE $client
```

**Commands:**
- `SOCKET CREATE` - Create socket
- `SOCKET BIND` - Bind to address/port
- `SOCKET LISTEN` - Listen for connections
- `SOCKET ACCEPT` - Accept connection
- `SOCKET CONNECT` - Connect to remote host
- `SOCKET SEND` - Send data
- `SOCKET RECV` - Receive data
- `SOCKET CLOSE` - Close socket
- `SOCKET GETOPT/SETOPT` - Socket options

**Features:**
- TCP and UDP support
- IPv4 and IPv6
- Non-blocking I/O with EVENT system
- Server and client modes

---

## Loading Extensions

```bcl
LOAD "path/to/extension.so"
```

The extension will register its commands with the interpreter.

## Event System Integration

Extensions integrate seamlessly with the EVENT system:

```bcl
LOAD "extensions/socket.so"

# Create server
SET server [SOCKET SERVER 9999]

# Register event handler
PROC ON_ACCEPT WITH server_fd DO
    SET client [SOCKET ACCEPT $server_fd]
    EVENT CREATE $client READABLE ON_CLIENT_DATA
END

EVENT CREATE $server READABLE ON_ACCEPT
EVENT LOOP
```

See `docs/EVENT_SYSTEM.md` for complete event system documentation.

## Building Extensions

```bash
cd extensions
make socket.so
```

## Creating New Extensions

See:
- `docs/extensions/main.pdf` - Complete extension development guide
- `docs/llm_extensions.md` - LLM-friendly API reference (coming soon)
- `socket.c` - Reference implementation

### Basic Extension Structure

```c
#include "bcl.h"

// Command implementation
static int my_command(ClientData clientData, Tcl_Interp *interp, 
                      int argc, const char *argv[]) {
    // Implementation
    return TCL_OK;
}

// Extension initialization
int bcl_extension_init(bcl_extension_api_t *api) {
    api->register_command(interp, "MYCOMMAND", my_command);
    return 0;
}
```

## Documentation

- Extension development: `docs/extensions/main.pdf`
- Event system integration: `docs/EVENT_SYSTEM.md`
- Man pages: `man 1 bcl`, `man 7 bcl`

## Contributing

When creating new extensions:

1. Follow the naming convention: `name.c` → `name.so`
2. Implement `bcl_extension_init()` entry point
3. Use uppercase for command names
4. Add comprehensive documentation
5. Create test suite
6. Update this README
