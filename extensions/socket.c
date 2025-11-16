/**
 * @file socket.c
 * @brief BCL SOCKET Extension - TCP client/server sockets
 * @version 1.0.0
 *
 * Implements TCP socket functionality similar to TCL's socket command.
 *
 * Usage:
 *   SOCKET SERVER port              -> Returns handle to server socket
 *   SOCKET CLIENT host port         -> Returns handle to client socket
 *   SOCKET ACCEPT server_handle     -> Accepts connection, returns client handle
 *   SOCKET SEND handle data         -> Sends data through socket
 *   SOCKET RECV handle [maxbytes]   -> Receives data from socket
 *   SOCKET CLOSE handle             -> Closes socket
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>

/* BCL API will be populated by bcl_extension_init */
static struct {
    int version;
    void *interp;
    int (*register_command)(void *interp, const char *name, void *func);
    void (*set_error)(void *interp, const char *fmt, ...);
    void *(*value_create)(const char *str);
    void (*value_destroy)(void *val);
    const char *(*value_get)(void *val);
    int (*var_set)(void *interp, const char *name, const char *value);
    void *(*var_get)(void *interp, const char *name);
} bcl_api;

/* ========================================================================== */
/* SOCKET REGISTRY                                                           */
/* ========================================================================== */

#define MAX_SOCKETS 256

typedef enum {
    SOCKET_TYPE_SERVER,
    SOCKET_TYPE_CLIENT
} socket_type_t;

typedef struct {
    int fd;                    /* File descriptor */
    socket_type_t type;        /* Server or client */
    int port;                  /* Port number */
    char *host;                /* Hostname (for clients) */
    int in_use;                /* 1 if allocated */
} socket_info_t;

static socket_info_t socket_registry[MAX_SOCKETS];
static int sockets_initialized = 0;

/**
 * @brief Initialize socket registry
 */
static void socket_registry_init(void) {
    if (sockets_initialized) return;

    for (int i = 0; i < MAX_SOCKETS; i++) {
        socket_registry[i].fd = -1;
        socket_registry[i].in_use = 0;
        socket_registry[i].host = NULL;
    }
    sockets_initialized = 1;
}

/**
 * @brief Allocate a new socket handle
 */
static int socket_registry_alloc(int fd, socket_type_t type, int port, const char *host) {
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (!socket_registry[i].in_use) {
            socket_registry[i].fd = fd;
            socket_registry[i].type = type;
            socket_registry[i].port = port;
            socket_registry[i].host = host ? strdup(host) : NULL;
            socket_registry[i].in_use = 1;
            return i;
        }
    }
    return -1;
}

/**
 * @brief Get socket info by handle
 */
static socket_info_t *socket_registry_get(int handle) {
    if (handle < 0 || handle >= MAX_SOCKETS) return NULL;
    if (!socket_registry[handle].in_use) return NULL;
    return &socket_registry[handle];
}

/**
 * @brief Free a socket handle
 */
static void socket_registry_free(int handle) {
    if (handle < 0 || handle >= MAX_SOCKETS) return;

    if (socket_registry[handle].in_use) {
        if (socket_registry[handle].fd >= 0) {
            close(socket_registry[handle].fd);
        }
        if (socket_registry[handle].host) {
            free(socket_registry[handle].host);
        }
        socket_registry[handle].in_use = 0;
        socket_registry[handle].fd = -1;
        socket_registry[handle].host = NULL;
    }
}

/* ========================================================================== */
/* SOCKET SERVER                                                             */
/* ========================================================================== */

/**
 * SOCKET SERVER port
 * Creates a TCP server socket listening on the given port
 */
static int socket_server(void *interp, int port, void **result) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        bcl_api.set_error(interp, "SOCKET SERVER: cannot create socket: %s", strerror(errno));
        return -1;
    }

    /* Allow socket reuse */
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* Bind to port */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        bcl_api.set_error(interp, "SOCKET SERVER: cannot bind to port %d: %s", port, strerror(errno));
        close(sockfd);
        return -1;
    }

    /* Listen */
    if (listen(sockfd, 5) < 0) {
        bcl_api.set_error(interp, "SOCKET SERVER: cannot listen: %s", strerror(errno));
        close(sockfd);
        return -1;
    }

    /* Register socket */
    int handle = socket_registry_alloc(sockfd, SOCKET_TYPE_SERVER, port, NULL);
    if (handle < 0) {
        bcl_api.set_error(interp, "SOCKET SERVER: too many sockets");
        close(sockfd);
        return -1;
    }

    /* Return handle as "sock<N>" */
    char handle_str[32];
    snprintf(handle_str, sizeof(handle_str), "sock%d", handle);
    *result = bcl_api.value_create(handle_str);

    return 0;
}

/* ========================================================================== */
/* SOCKET CLIENT                                                             */
/* ========================================================================== */

/**
 * SOCKET CLIENT host port
 * Connects to a TCP server
 */
static int socket_client(void *interp, const char *host, int port, void **result) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        bcl_api.set_error(interp, "SOCKET CLIENT: cannot create socket: %s", strerror(errno));
        return -1;
    }

    /* Resolve hostname */
    struct hostent *server = gethostbyname(host);
    if (!server) {
        bcl_api.set_error(interp, "SOCKET CLIENT: cannot resolve host %s", host);
        close(sockfd);
        return -1;
    }

    /* Connect */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    memcpy(&addr.sin_addr.s_addr, server->h_addr, server->h_length);
    addr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        bcl_api.set_error(interp, "SOCKET CLIENT: cannot connect to %s:%d: %s", host, port, strerror(errno));
        close(sockfd);
        return -1;
    }

    /* Register socket */
    int handle = socket_registry_alloc(sockfd, SOCKET_TYPE_CLIENT, port, host);
    if (handle < 0) {
        bcl_api.set_error(interp, "SOCKET CLIENT: too many sockets");
        close(sockfd);
        return -1;
    }

    /* Return handle */
    char handle_str[32];
    snprintf(handle_str, sizeof(handle_str), "sock%d", handle);
    *result = bcl_api.value_create(handle_str);

    return 0;
}

/* ========================================================================== */
/* SOCKET ACCEPT                                                             */
/* ========================================================================== */

/**
 * SOCKET ACCEPT server_handle
 * Accepts a connection on a server socket
 */
static int socket_accept(void *interp, const char *handle_str, void **result) {
    /* Parse handle */
    int handle = -1;
    if (sscanf(handle_str, "sock%d", &handle) != 1) {
        bcl_api.set_error(interp, "SOCKET ACCEPT: invalid handle %s", handle_str);
        return -1;
    }

    socket_info_t *sock = socket_registry_get(handle);
    if (!sock || sock->type != SOCKET_TYPE_SERVER) {
        bcl_api.set_error(interp, "SOCKET ACCEPT: %s is not a server socket", handle_str);
        return -1;
    }

    /* Accept connection */
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(sock->fd, (struct sockaddr*)&client_addr, &client_len);

    if (client_fd < 0) {
        bcl_api.set_error(interp, "SOCKET ACCEPT: accept failed: %s", strerror(errno));
        return -1;
    }

    /* Get client info */
    char *client_host = inet_ntoa(client_addr.sin_addr);
    int client_port = ntohs(client_addr.sin_port);

    /* Register client socket */
    int client_handle = socket_registry_alloc(client_fd, SOCKET_TYPE_CLIENT, client_port, client_host);
    if (client_handle < 0) {
        bcl_api.set_error(interp, "SOCKET ACCEPT: too many sockets");
        close(client_fd);
        return -1;
    }

    /* Return handle */
    char client_handle_str[32];
    snprintf(client_handle_str, sizeof(client_handle_str), "sock%d", client_handle);
    *result = bcl_api.value_create(client_handle_str);

    return 0;
}

/* ========================================================================== */
/* SOCKET SEND                                                               */
/* ========================================================================== */

/**
 * SOCKET SEND handle data
 * Sends data through socket
 */
static int socket_send(void *interp, const char *handle_str, const char *data, void **result) {
    /* Parse handle */
    int handle = -1;
    if (sscanf(handle_str, "sock%d", &handle) != 1) {
        bcl_api.set_error(interp, "SOCKET SEND: invalid handle %s", handle_str);
        return -1;
    }

    socket_info_t *sock = socket_registry_get(handle);
    if (!sock) {
        bcl_api.set_error(interp, "SOCKET SEND: invalid socket %s", handle_str);
        return -1;
    }

    /* Send data */
    size_t len = strlen(data);
    ssize_t sent = send(sock->fd, data, len, 0);

    if (sent < 0) {
        bcl_api.set_error(interp, "SOCKET SEND: send failed: %s", strerror(errno));
        return -1;
    }

    /* Return bytes sent */
    char bytes_str[32];
    snprintf(bytes_str, sizeof(bytes_str), "%ld", (long)sent);
    *result = bcl_api.value_create(bytes_str);

    return 0;
}

/* ========================================================================== */
/* SOCKET RECV                                                               */
/* ========================================================================== */

/**
 * SOCKET RECV handle [maxbytes]
 * Receives data from socket
 */
static int socket_recv(void *interp, const char *handle_str, int maxbytes, void **result) {
    /* Parse handle */
    int handle = -1;
    if (sscanf(handle_str, "sock%d", &handle) != 1) {
        bcl_api.set_error(interp, "SOCKET RECV: invalid handle %s", handle_str);
        return -1;
    }

    socket_info_t *sock = socket_registry_get(handle);
    if (!sock) {
        bcl_api.set_error(interp, "SOCKET RECV: invalid socket %s", handle_str);
        return -1;
    }

    /* Allocate buffer */
    if (maxbytes <= 0) maxbytes = 4096;
    char *buffer = malloc(maxbytes + 1);
    if (!buffer) {
        bcl_api.set_error(interp, "SOCKET RECV: out of memory");
        return -1;
    }

    /* Receive data */
    ssize_t received = recv(sock->fd, buffer, maxbytes, 0);

    if (received < 0) {
        bcl_api.set_error(interp, "SOCKET RECV: recv failed: %s", strerror(errno));
        free(buffer);
        return -1;
    }

    buffer[received] = '\0';
    *result = bcl_api.value_create(buffer);
    free(buffer);

    return 0;
}

/* ========================================================================== */
/* SOCKET CLOSE                                                              */
/* ========================================================================== */

/**
 * SOCKET CLOSE handle
 * Closes a socket
 */
static int socket_close(void *interp, const char *handle_str, void **result) {
    /* Parse handle */
    int handle = -1;
    if (sscanf(handle_str, "sock%d", &handle) != 1) {
        bcl_api.set_error(interp, "SOCKET CLOSE: invalid handle %s", handle_str);
        return -1;
    }

    socket_info_t *sock = socket_registry_get(handle);
    if (!sock) {
        bcl_api.set_error(interp, "SOCKET CLOSE: invalid socket %s", handle_str);
        return -1;
    }

    socket_registry_free(handle);
    *result = bcl_api.value_create("");

    return 0;
}

/* ========================================================================== */
/* SOCKET COMMAND DISPATCHER                                                 */
/* ========================================================================== */

/**
 * Main SOCKET command
 */
static int bcl_cmd_socket(void *interp, int argc, char **argv, void **result) {
    if (argc < 1) {
        bcl_api.set_error(interp, "SOCKET: wrong # args: should be \"SOCKET subcommand ?args?\"");
        return 1; /* BCL_ERROR */
    }

    const char *subcmd = argv[0];

    /* SOCKET SERVER port */
    if (strcasecmp(subcmd, "SERVER") == 0) {
        if (argc != 2) {
            bcl_api.set_error(interp, "SOCKET SERVER: wrong # args: should be \"SOCKET SERVER port\"");
            return 1;
        }

        int port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            bcl_api.set_error(interp, "SOCKET SERVER: invalid port %s", argv[1]);
            return 1;
        }

        return socket_server(interp, port, result);
    }

    /* SOCKET CLIENT host port */
    if (strcasecmp(subcmd, "CLIENT") == 0) {
        if (argc != 3) {
            bcl_api.set_error(interp, "SOCKET CLIENT: wrong # args: should be \"SOCKET CLIENT host port\"");
            return 1;
        }

        int port = atoi(argv[2]);
        if (port <= 0 || port > 65535) {
            bcl_api.set_error(interp, "SOCKET CLIENT: invalid port %s", argv[2]);
            return 1;
        }

        return socket_client(interp, argv[1], port, result);
    }

    /* SOCKET ACCEPT server_handle */
    if (strcasecmp(subcmd, "ACCEPT") == 0) {
        if (argc != 2) {
            bcl_api.set_error(interp, "SOCKET ACCEPT: wrong # args: should be \"SOCKET ACCEPT handle\"");
            return 1;
        }

        return socket_accept(interp, argv[1], result);
    }

    /* SOCKET SEND handle data */
    if (strcasecmp(subcmd, "SEND") == 0) {
        if (argc != 3) {
            bcl_api.set_error(interp, "SOCKET SEND: wrong # args: should be \"SOCKET SEND handle data\"");
            return 1;
        }

        return socket_send(interp, argv[1], argv[2], result);
    }

    /* SOCKET RECV handle [maxbytes] */
    if (strcasecmp(subcmd, "RECV") == 0) {
        if (argc < 2 || argc > 3) {
            bcl_api.set_error(interp, "SOCKET RECV: wrong # args: should be \"SOCKET RECV handle ?maxbytes?\"");
            return 1;
        }

        int maxbytes = (argc == 3) ? atoi(argv[2]) : 4096;
        return socket_recv(interp, argv[1], maxbytes, result);
    }

    /* SOCKET CLOSE handle */
    if (strcasecmp(subcmd, "CLOSE") == 0) {
        if (argc != 2) {
            bcl_api.set_error(interp, "SOCKET CLOSE: wrong # args: should be \"SOCKET CLOSE handle\"");
            return 1;
        }

        return socket_close(interp, argv[1], result);
    }

    bcl_api.set_error(interp, "SOCKET: unknown subcommand \"%s\": must be SERVER, CLIENT, ACCEPT, SEND, RECV, or CLOSE", subcmd);
    return 1;
}

/* ========================================================================== */
/* EXTENSION INITIALIZATION                                                  */
/* ========================================================================== */

/**
 * Extension initialization function
 * Called by BCL when LOAD command loads this extension
 */
int bcl_extension_init(void *api_ptr) {
    /* Cast API structure */
    struct {
        int version;
        void *interp;
        int (*register_command)(void *interp, const char *name, void *func);
        void (*set_error)(void *interp, const char *fmt, ...);
        void *(*value_create)(const char *str);
        void (*value_destroy)(void *val);
        const char *(*value_get)(void *val);
        int (*var_set)(void *interp, const char *name, const char *value);
        void *(*var_get)(void *interp, const char *name);
    } *api = api_ptr;

    /* Verify API version */
    if (api->version != 1) {
        return -1;
    }

    /* Save API functions */
    bcl_api.version = api->version;
    bcl_api.interp = api->interp;
    bcl_api.register_command = api->register_command;
    bcl_api.set_error = api->set_error;
    bcl_api.value_create = api->value_create;
    bcl_api.value_destroy = api->value_destroy;
    bcl_api.value_get = api->value_get;
    bcl_api.var_set = api->var_set;
    bcl_api.var_get = api->var_get;

    /* Initialize socket registry */
    socket_registry_init();

    /* Register SOCKET command - use interp from API */
    if (api->register_command(api->interp, "SOCKET", (void*)(intptr_t)bcl_cmd_socket) != 0) {
        return -1;
    }

    return 0;
}
