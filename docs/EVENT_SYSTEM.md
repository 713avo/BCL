# Sistema de Eventos BCL v2.0

## Índice

1. [Introducción](#introducción)
2. [Arquitectura](#arquitectura)
3. [Comando EVENT](#comando-event)
4. [Ejemplos Prácticos](#ejemplos-prácticos)
5. [Integración con Bibliotecas](#integración-con-bibliotecas)
6. [Crear Extensiones con Eventos](#crear-extensiones-con-eventos)
7. [Referencia API](#referencia-api)
8. [Troubleshooting](#troubleshooting)

---

## Introducción

BCL v2.0 introduce un sistema de eventos asíncrono completo que permite:

- **Eventos I/O**: Detectar cuando un socket, archivo o stdin tiene datos listos
- **Timers**: Ejecutar código después de un intervalo de tiempo
- **Event Loop**: Procesar múltiples eventos concurrentemente
- **Callbacks con parámetros**: Los callbacks reciben el handle que disparó el evento

### Características Principales

✓ Arquitectura **híbrida**: Core POSIX + extensiones opcionales
✓ **Sin bloqueo**: select() con timeout configurable
✓ **Portable**: Funciona en Linux, macOS, *BSD
✓ **Extensible**: API para GPIO, epoll, kqueue, etc.
✓ **Callbacks con parámetros**: Reciben FD/handle como argumento

---

## Arquitectura

### Diseño Híbrido

```
┌─────────────────────────────────────────────┐
│             BCL Core (bcl_event.c)          │
│  ┌────────────────────────────────────────┐ │
│  │  Event Loop (select-based)             │ │
│  │  • Portable POSIX select()             │ │
│  │  • Timer queue (millisecond precision) │ │
│  │  • Event registration/dispatch         │ │
│  └────────────────────────────────────────┘ │
└─────────────────────────────────────────────┘
           │                    │
           ├────────────────────┴──────────────┐
           ▼                                   ▼
┌───────────────────────┐       ┌──────────────────────┐
│  Platform Extensions  │       │  Domain Extensions   │
│  (loadable via LOAD)  │       │  (loadable via LOAD) │
│                       │       │                      │
│  • epoll (Linux)      │       │  • GPIO (embedded)   │
│  • kqueue (BSD/macOS) │       │  • Serial ports      │
│  • IOCP (Windows)     │       │  • USB events        │
└───────────────────────┘       └──────────────────────┘
```

### Estructura de Eventos

```c
typedef struct bcl_event {
    bcl_event_source_t source;      // FD o TIMER
    union {
        struct {
            int fd;                  // File descriptor
            bcl_event_type_t types;  // READABLE|WRITABLE|EXCEPTION
        } fd_event;
        struct {
            uint64_t expire_time_ms;
            uint32_t interval_ms;    // 0 = one-shot
        } timer_event;
    };
    char *callback;                  // Nombre del procedimiento
    struct bcl_event *next;          // Lista enlazada
} bcl_event_t;
```

---

## Comando EVENT

### Sintaxis General

```bcl
EVENT subcommand [args...]
```

### Subcomandos

#### EVENT CREATE

Registra un evento de I/O en un file descriptor.

**Sintaxis:**
```bcl
EVENT CREATE handle type callback
```

**Parámetros:**
- `handle`: "stdin", "stdout", "stderr", número FD, o "sock0", "sock1", etc.
- `type`: "READABLE", "WRITABLE", o "EXCEPTION"
- `callback`: Nombre del procedimiento a ejecutar

**Callback recibe:**
- Parámetro 1: File descriptor que disparó el evento

**Ejemplo:**
```bcl
PROC ON_STDIN_DATA WITH fd DO
    SET line [GETS $fd]
    PUTS "Leído de FD $fd: $line"
END

EVENT CREATE stdin READABLE ON_STDIN_DATA
EVENT LOOP
```

---

#### EVENT DELETE

Elimina un evento registrado.

**Sintaxis:**
```bcl
EVENT DELETE handle [type]
```

**Parámetros:**
- `handle`: Handle del evento a eliminar
- `type`: (Opcional) Tipo específico. Si se omite, elimina todos los tipos.

**Ejemplo:**
```bcl
EVENT DELETE stdin
EVENT DELETE $client_socket READABLE
```

---

#### EVENT TIMER

Crea un timer que ejecuta un callback después de un intervalo.

**Sintaxis:**
```bcl
EVENT TIMER milliseconds callback
```

**Parámetros:**
- `milliseconds`: Tiempo de espera en milisegundos
- `callback`: Procedimiento a ejecutar (sin parámetros)

**Callback recibe:**
- Sin parámetros (los timers son one-shot o periódicos)

**Ejemplo:**
```bcl
PROC ON_TIMEOUT DO
    PUTS "Timer disparado en [CLOCK MILLISECONDS]"
    EVENT STOP  # Detiene el event loop
END

EVENT TIMER 5000 ON_TIMEOUT  # 5 segundos
EVENT LOOP
```

---

#### EVENT PROCESS

Procesa eventos una sola vez (non-blocking con timeout).

**Sintaxis:**
```bcl
EVENT PROCESS [timeout]
```

**Parámetros:**
- `timeout`: (Opcional) Timeout en ms. Si se omite, bloquea hasta evento.

**Retorna:**
- "1" si se procesó algún evento
- "0" si no hay eventos o expiró timeout

**Ejemplo:**
```bcl
# Procesar eventos con timeout de 1 segundo
SET events_fired [EVENT PROCESS 1000]
IF [EXPR $events_fired == 1] THEN
    PUTS "Evento procesado"
ELSE
    PUTS "Timeout sin eventos"
END
```

---

#### EVENT LOOP

Ejecuta el event loop indefinidamente hasta que se llame `EVENT STOP`.

**Sintaxis:**
```bcl
EVENT LOOP
```

**Ejemplo:**
```bcl
PROC ON_SIGNAL WITH fd DO
    PUTS "Señal recibida, saliendo..."
    EVENT STOP
END

EVENT CREATE $signal_fd READABLE ON_SIGNAL
EVENT LOOP  # Bloquea hasta EVENT STOP
PUTS "Event loop terminado"
```

---

#### EVENT STOP

Detiene el event loop activo (llamar desde un callback).

**Sintaxis:**
```bcl
EVENT STOP
```

**Ejemplo:**
```bcl
PROC ON_EXIT_COMMAND WITH client_fd DO
    GLOBAL running
    SET data [SOCKET RECV $client_fd 1024]

    IF [STRING EQUAL $data "EXIT"] THEN
        PUTS "Comando EXIT recibido"
        EVENT STOP
    END
END

EVENT CREATE $client READABLE ON_EXIT_COMMAND
EVENT LOOP
```

---

#### EVENT INFO

Lista todos los eventos registrados (útil para debugging).

**Sintaxis:**
```bcl
EVENT INFO
```

**Retorna:**
- String con información de eventos registrados

**Ejemplo:**
```bcl
EVENT TIMER 1000 MY_CALLBACK
EVENT CREATE stdin READABLE ON_INPUT

SET info [EVENT INFO]
PUTS $info
# Output:
# FD 0 (R) -> ON_INPUT
# TIMER in 987ms -> MY_CALLBACK
```

---

## Ejemplos Prácticos

### Ejemplo 1: Servidor Echo Multi-Cliente

```bcl
#!/usr/bin/env bcl
SOURCE "lib/ANSI.BLB"
LOAD "extensions/socket.so"

GLOBAL server clients client_count
SET client_count 0

# Callback cuando llega conexión (recibe server handle como parámetro)
PROC ON_ACCEPT WITH server_fd DO
    GLOBAL clients client_count

    SET client [SOCKET ACCEPT $server_fd]
    SET clients($client) 1
    INCR client_count

    EVENT CREATE $client READABLE ON_CLIENT_DATA

    ANSI_SET_FG $ANSI_FG_GREEN
    PUTS "Cliente conectado: $client (total: $client_count)"
    ANSI_RESET
END

# Callback cuando hay datos del cliente (recibe client handle)
PROC ON_CLIENT_DATA WITH client_fd DO
    GLOBAL clients client_count

    SET data [SOCKET RECV $client_fd 1024]

    IF [EXPR [STRING LENGTH $data] == 0] THEN
        # Cliente desconectado
        EVENT DELETE $client_fd
        SOCKET CLOSE $client_fd
        UNSET clients($client_fd)
        SET client_count [EXPR $client_count - 1]

        ANSI_SET_FG $ANSI_FG_RED
        PUTS "Cliente desconectado: $client_fd"
        ANSI_RESET
        RETURN
    END

    # Echo de vuelta
    PUTS "[$client_fd] $data"
    SOCKET SEND $client_fd "Echo: $data"
END

# Timer de estadísticas cada 5 segundos
PROC ON_STATS_TIMER DO
    GLOBAL client_count
    PUTS "--- Estadísticas: $client_count clientes ---"

    # Re-programar (los timers son one-shot)
    EVENT TIMER 5000 ON_STATS_TIMER
END

# Inicializar
ANSI_CLEAR
PUTS "Servidor Echo en puerto 9999"
PUTS "Ctrl+C para salir"
PUTS ""

SET server [SOCKET SERVER 9999]
EVENT CREATE $server READABLE ON_ACCEPT
EVENT TIMER 5000 ON_STATS_TIMER

EVENT LOOP
```

---

### Ejemplo 2: Cliente Interactivo

```bcl
#!/usr/bin/env bcl
LOAD "extensions/socket.so"

GLOBAL running
SET running 1

# Recibir datos del servidor (recibe socket FD)
PROC ON_SERVER_DATA WITH sock_fd DO
    SET data [SOCKET RECV $sock_fd 1024]

    IF [EXPR [STRING LENGTH $data] == 0] THEN
        PUTS "Servidor desconectado"
        GLOBAL running
        SET running 0
        EVENT STOP
        RETURN
    END

    PUTS "Servidor: $data"
END

# Leer del teclado (recibe stdin FD)
PROC ON_KEYBOARD WITH stdin_fd DO
    GLOBAL server running

    SET line [GETS $stdin_fd]

    IF [STRING EQUAL $line "quit"] THEN
        PUTS "Saliendo..."
        SET running 0
        EVENT STOP
        RETURN
    END

    SOCKET SEND $server "$line\n"
END

# Conectar
SET server [SOCKET CLIENT "localhost" 9999]
PUTS "Conectado. Escribe 'quit' para salir."

# Registrar eventos
EVENT CREATE $server READABLE ON_SERVER_DATA
EVENT CREATE stdin READABLE ON_KEYBOARD

EVENT LOOP

# Cleanup
SOCKET CLOSE $server
PUTS "Conexión cerrada"
```

---

### Ejemplo 3: Timers Múltiples

```bcl
#!/usr/bin/env bcl

GLOBAL count1 count2
SET count1 0
SET count2 0

# Timer rápido (cada 500ms)
PROC FAST_TIMER DO
    GLOBAL count1
    INCR count1
    PUTS "[FAST] Tick $count1"

    IF [EXPR $count1 < 10] THEN
        EVENT TIMER 500 FAST_TIMER  # Re-programar
    END
END

# Timer lento (cada 1000ms)
PROC SLOW_TIMER DO
    GLOBAL count2
    INCR count2
    PUTS "[SLOW] Tick $count2"

    IF [EXPR $count2 < 5] THEN
        EVENT TIMER 1000 SLOW_TIMER  # Re-programar
    ELSE
        EVENT STOP  # Terminar después de 5 iteraciones
    END
END

PUTS "Iniciando timers múltiples..."
EVENT TIMER 500 FAST_TIMER
EVENT TIMER 1000 SLOW_TIMER

EVENT LOOP
PUTS "Timers completados"
```

---

### Ejemplo 4: Monitoreo de Archivos

```bcl
#!/usr/bin/env bcl

# Monitorear cambios en un archivo
PROC WATCH_FILE WITH filename DO
    GLOBAL last_size

    SET size [FILE SIZE $filename]

    IF [EXPR $size != $last_size] THEN
        PUTS "Archivo modificado: $filename (size: $size bytes)"
        SET last_size $size

        # Leer últimas líneas
        SET fd [OPEN $filename "r"]
        SEEK $fd -100 end  # Últimos 100 bytes
        SET tail [READ $fd 100]
        CLOSE $fd

        PUTS "Últimas líneas:"
        PUTS $tail
    END

    # Re-programar para chequear en 1 segundo
    EVENT TIMER 1000 CHECK_FILE
END

PROC CHECK_FILE DO
    WATCH_FILE "/var/log/system.log"
END

# Inicializar
GLOBAL last_size
SET last_size [FILE SIZE "/var/log/system.log"]

PUTS "Monitoreando /var/log/system.log"
EVENT TIMER 1000 CHECK_FILE
EVENT LOOP
```

---

## Integración con Bibliotecas

### Uso con ANSI.BLB

```bcl
SOURCE "lib/ANSI.BLB"

PROC UPDATE_DISPLAY WITH timer_id DO
    GLOBAL progress
    INCR progress 10

    ANSI_CURSOR_HOME
    ANSI_PROGRESS_BAR 10 10 50 $progress

    IF [EXPR $progress >= 100] THEN
        ANSI_CURSOR_GOTO 12 1
        PUTS "Completado!"
        EVENT STOP
    ELSE
        EVENT TIMER 200 UPDATE_DISPLAY  # Re-programar
    END
END

GLOBAL progress
SET progress 0

ANSI_INIT
EVENT TIMER 200 UPDATE_DISPLAY
EVENT LOOP
ANSI_CLEANUP
```

---

### Uso con MATRIX.BLB

```bcl
SOURCE "lib/MATRIX.BLB"

GLOBAL M
MAT_RAND M 3 3

PROC MATRIX_ANIMATION DO
    GLOBAL M

    # Rotar valores
    SET val $M(0,0)
    MAT_FILL M [EXPR rand()]

    ANSI_CLEAR
    ANSI_CURSOR_HOME
    MAT_PRINT M

    EVENT TIMER 500 MATRIX_ANIMATION  # Continuar animación
END

ANSI_INIT
EVENT TIMER 500 MATRIX_ANIMATION
EVENT LOOP
```

---

## Crear Extensiones con Eventos

### API para Extensiones

Las extensiones pueden registrar sus propios eventos usando la API de BCL:

```c
// Extensión GPIO de ejemplo
#include "bcl.h"

// Callback cuando GPIO cambia
static void gpio_isr_handler(int pin) {
    bcl_interp_t *interp = get_global_interp();

    // Disparar evento BCL
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "%s %d",
             gpio_callbacks[pin], pin);

    bcl_eval(interp, cmd, NULL);
}

// Comando: GPIO_INTERRUPT pin edge callback
static bcl_result_t cmd_gpio_interrupt(bcl_interp_t *interp,
                                       int argc, char **argv,
                                       bcl_value_t **result) {
    int pin = atoi(argv[0]);
    const char *edge = argv[1];  // "RISING", "FALLING", "BOTH"
    const char *callback = argv[2];

    // Verificar que callback existe
    if (!bcl_proc_exists(interp, callback)) {
        bcl_set_error(interp, "procedure not found: %s", callback);
        return BCL_ERROR;
    }

    // Guardar callback
    gpio_callbacks[pin] = strdup(callback);

    // Configurar ISR de hardware
    gpio_setup_interrupt(pin, edge, gpio_isr_handler);

    *result = bcl_value_create("");
    return BCL_OK;
}
```

Uso desde BCL:

```bcl
LOAD "extensions/gpio.so"

PROC ON_BUTTON WITH pin DO
    PUTS "Botón presionado en pin $pin"
    GLOBAL led_state
    SET led_state [EXPR !$led_state]
    GPIO_WRITE 13 $led_state
END

GPIO_MODE 2 INPUT
GPIO_MODE 13 OUTPUT
GPIO_INTERRUPT 2 FALLING ON_BUTTON

EVENT LOOP  # El ISR del GPIO dispara eventos en BCL
```

---

## Referencia API

### Tipos de Eventos

| Tipo | Descripción | Callback recibe |
|------|-------------|-----------------|
| **READABLE** | FD tiene datos para leer | `WITH fd DO` |
| **WRITABLE** | FD listo para escribir | `WITH fd DO` |
| **EXCEPTION** | Condición excepcional en FD | `WITH fd DO` |
| **TIMER** | Timer expiró | `DO` (sin params) |

### Handles Soportados

| Handle | Descripción |
|--------|-------------|
| `stdin` | Entrada estándar (FD 0) |
| `stdout` | Salida estándar (FD 1) |
| `stderr` | Error estándar (FD 2) |
| `sock0`, `sock1`, ... | Sockets de SOCKET extension |
| Número directo | File descriptor numérico |

### Retornos de Comandos

| Comando | Retorno |
|---------|---------|
| `EVENT CREATE` | "" (vacío si OK) |
| `EVENT DELETE` | "" (vacío si OK) |
| `EVENT TIMER` | "" (vacío si OK) |
| `EVENT PROCESS` | "1" si evento procesado, "0" si timeout |
| `EVENT LOOP` | "" al terminar |
| `EVENT INFO` | String con info de eventos |

---

## Troubleshooting

### Problema: "procedure not found"

**Causa:** El callback no existe al registrar el evento.

**Solución:**
```bcl
# MAL - callback no definido todavía
EVENT TIMER 1000 MY_CALLBACK
PROC MY_CALLBACK DO
    PUTS "Hola"
END

# BIEN - definir callback primero
PROC MY_CALLBACK DO
    PUTS "Hola"
END
EVENT TIMER 1000 MY_CALLBACK
```

---

### Problema: Variables no accesibles en callback

**Causa:** Falta declaración GLOBAL.

**Solución:**
```bcl
SET counter 0

PROC ON_TIMER DO
    GLOBAL counter  # ← IMPORTANTE
    INCR counter
    PUTS "Counter: $counter"
END
```

---

### Problema: Event loop no termina

**Causa:** No se llama a `EVENT STOP`.

**Solución:**
```bcl
PROC ON_QUIT WITH fd DO
    SET cmd [GETS $fd]
    IF [STRING EQUAL $cmd "quit"] THEN
        EVENT STOP  # ← Necesario para salir de EVENT LOOP
    END
END
```

---

### Problema: Timer no se repite

**Causa:** Los timers son one-shot por defecto.

**Solución:**
```bcl
PROC REPEATING_TIMER DO
    PUTS "Tick"
    EVENT TIMER 1000 REPEATING_TIMER  # ← Re-programar
END

EVENT TIMER 1000 REPEATING_TIMER
```

---

## Notas de Implementación

### Limitaciones Actuales

- Máximo de eventos limitado por memoria
- select() tiene límite de FD_SETSIZE (típicamente 1024)
- Timers con precisión de ~1ms (depende del OS)
- No hay prioridades de eventos

### Mejoras Futuras

- Soporte epoll/kqueue via extensiones
- Timers periódicos nativos (sin re-programar)
- Prioridades de eventos
- Event groups/categorías

---

## Ejemplos Adicionales

Ver carpeta `examples/`:
- `examples/socket_server.bcl` - Servidor TCP completo
- `examples/socket_client.bcl` - Cliente TCP interactivo
- `tests/test_events.bcl` - Suite de tests del sistema

---

**BCL Event System v2.0**
*Documentación actualizada: 2025-11-16*
