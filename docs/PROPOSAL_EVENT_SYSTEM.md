# Propuesta: Sistema de Eventos para BCL
## Análisis de Implementación

### Contexto

Implementar un sistema de eventos similar a `fileevent` de TCL que permita:
- **Eventos I/O**: Sockets, archivos, stdin (READABLE/WRITABLE)
- **Eventos de teclado**: Input asíncrono
- **Eventos GPIO**: Para microcontroladores (embedded)
- **Timers**: Callbacks temporizados
- **Eventos personalizados**: Desde extensiones

### Comparación: Core vs Extensión

#### Opción 1: Sistema en el Core
```
✓ Ventajas:
  - Siempre disponible, no requiere LOAD
  - Mejor integración con el intérprete
  - Acceso directo a internals (scopes, variables)
  - Más eficiente (sin overhead de extensión)
  - Event loop puede controlar REPL y ejecución

✗ Desventajas:
  - Aumenta complejidad del core (~1500 líneas)
  - No se puede desactivar si no se usa
  - Dificulta portabilidad (select vs epoll vs kqueue)
  - Más difícil mantener y actualizar
```

#### Opción 2: Sistema como Extensión (LOAD)
```
✓ Ventajas:
  - Opcional, solo se carga si se necesita
  - Core permanece simple y portable
  - Fácil actualizar independientemente
  - Diferentes backends sin recompilar BCL
  - Ideal para casos específicos (GPIO en embedded)

✗ Desventajas:
  - Requiere LOAD explícito
  - Overhead de llamadas a extensión
  - Complejo coordinar event loop con intérprete
  - Extensión no puede controlar flujo del intérprete
```

#### Opción 3: Sistema Híbrido (RECOMENDADO)
```
Core Mínimo:
  - Registro de callbacks (EVENT command)
  - Event loop básico con select()
  - Integración con AFTER
  - API para extensiones registren eventos

Extensiones Opcionales:
  - EVENT_EPOLL.so (Linux epoll backend)
  - EVENT_KQUEUE.so (BSD/macOS kqueue backend)
  - EVENT_GPIO.so (microcontroladores)
  - EVENT_WIN32.so (Windows IOCP)

✓✓ Beneficios:
  - Lo mejor de ambos mundos
  - Core simple y portable (select funciona en todos lados)
  - Optimizaciones via extensiones opcionales
  - Extensible a nuevos tipos de eventos
```

---

## Propuesta de Diseño: Sistema Híbrido

### Arquitectura

```
┌─────────────────────────────────────────────┐
│           BCL Core (bcl_event.c)            │
│  ┌───────────────────────────────────────┐  │
│  │ Event Loop (select/poll básico)      │  │
│  │ - Registro de callbacks               │  │
│  │ - Timer queue                         │  │
│  │ - Dispatch de eventos                 │  │
│  └───────────────────────────────────────┘  │
│                     ▲                        │
│                     │ API                    │
│  ┌─────────────────┴─────────────────────┐  │
│  │  bcl_event_register_fd()              │  │
│  │  bcl_event_register_timer()           │  │
│  │  bcl_event_register_custom()          │  │
│  └───────────────────────────────────────┘  │
└──────────────────┬──────────────────────────┘
                   │
                   │ Extensiones usan API
                   ▼
    ┌──────────────────────────────┐
    │  EVENT_EPOLL.so (opcional)   │
    │  - Backend Linux epoll       │
    │  - Mayor eficiencia          │
    └──────────────────────────────┘

    ┌──────────────────────────────┐
    │  EVENT_GPIO.so (opcional)    │
    │  - Eventos GPIO embedded     │
    │  - Interrupciones hardware   │
    └──────────────────────────────┘
```

### Comandos BCL (Core)

```bcl
# Crear event handler para descriptor de archivo
EVENT CREATE handle type callback
  handle   = socket/file/stdin
  type     = READABLE | WRITABLE | EXCEPTION
  callback = nombre del procedimiento BCL

# Eliminar event handler
EVENT DELETE handle [type]

# Procesar eventos una vez (timeout en ms, -1 = infinito)
EVENT PROCESS [timeout]

# Event loop infinito (sale con EXIT o error)
EVENT LOOP

# Listar eventos registrados
EVENT INFO

# Configurar backend (opcional, via extensión)
EVENT BACKEND epoll|select|kqueue|poll
```

### Ejemplo de Uso: Servidor Echo Asíncrono

```bcl
#!/usr/bin/env bcl

SOURCE "lib/ANSI.BLB"
LOAD "extensions/socket.so"

GLOBAL server clients

# Callback cuando llega conexión (recibe handle del servidor)
PROC ON_ACCEPT WITH server_handle DO
    GLOBAL clients client_count

    # Aceptar nueva conexión
    SET client [SOCKET ACCEPT $server_handle]

    # Registrar evento de lectura para el cliente
    EVENT CREATE $client READABLE ON_CLIENT_READ

    # Guardar en lista de clientes
    SET clients($client) 1
    INCR client_count

    ANSI_SET_COLOR $ANSI_FG_GREEN $ANSI_BG_BLACK
    PUTS "Cliente conectado: $client (total: $client_count)"
    ANSI_RESET
END

# Callback cuando hay datos del cliente (recibe handle del cliente)
PROC ON_CLIENT_READ WITH client_handle DO
    GLOBAL clients

    # Recibir datos
    SET data [SOCKET RECV $client_handle 1024]

    # Si no hay datos, cliente desconectado
    IF [EXPR [STRING LENGTH $data] == 0] THEN
        EVENT DELETE $client_handle
        SOCKET CLOSE $client_handle
        UNSET clients($client_handle)

        ANSI_SET_COLOR $ANSI_FG_RED $ANSI_BG_BLACK
        PUTS "Cliente desconectado: $client_handle"
        ANSI_RESET
        RETURN
    END

    # Echo de vuelta
    PUTS "[$client_handle] $data"
    SOCKET SEND $client_handle "Echo: $data"
END

# Timer periódico cada 5 segundos
PROC ON_TIMER DO
    GLOBAL client_count
    PUTS "--- Estadísticas: $client_count clientes conectados ---"

    # Re-programar timer (one-shot, hay que repetir)
    EVENT TIMER 5000 ON_TIMER
END

# Inicializar
SET server [SOCKET SERVER 8080]
SET client_count 0

# Registrar eventos
EVENT CREATE $server READABLE ON_ACCEPT
EVENT TIMER 5000 ON_TIMER

PUTS "Servidor echo asíncrono en puerto 8080"
PUTS "Presione Ctrl+C para salir"
PUTS ""

# Event loop infinito
EVENT LOOP
```

### Ejemplo: Eventos de Teclado

```bcl
#!/usr/bin/env bcl

# Callback recibe el handle (stdin) como parámetro
PROC ON_KEYBOARD WITH handle DO
    # Leer desde stdin
    SET key [GETS $handle]

    IF [STRING EQUAL $key "q"] THEN
        PUTS "Saliendo..."
        EXIT 0
    END

    PUTS "Tecla presionada: $key"
END

# Registrar stdin como evento
EVENT CREATE stdin READABLE ON_KEYBOARD

PUTS "Presione teclas (q para salir):"
EVENT LOOP
```

### Ejemplo: GPIO en Microcontrolador (con extensión)

```bcl
#!/usr/bin/env bcl

# Cargar extensión GPIO (solo en embedded)
LOAD "extensions/gpio.so"

GLOBAL led_state
SET led_state 0

# Callback recibe pin y estado como parámetros
PROC ON_BUTTON_PRESS WITH pin state DO
    GLOBAL led_state

    # Toggle LED
    IF [EXPR $led_state == 0] THEN
        GPIO_WRITE 13 1
        SET led_state 1
    ELSE
        GPIO_WRITE 13 0
        SET led_state 0
    END

    PUTS "LED toggled (pin $pin, estado: $state)"
END

# Configurar GPIO
GPIO_MODE 13 OUTPUT  ;# LED
GPIO_MODE 2 INPUT    ;# Botón

# Registrar evento de interrupción (GPIO extensión usa EVENT internamente)
GPIO_INTERRUPT 2 FALLING ON_BUTTON_PRESS

EVENT LOOP
```

---

## Implementación Propuesta

### Fase 1: Core Event System (include/bcl.h)

```c
/* ========================================================================== */
/* SISTEMA DE EVENTOS                                                        */
/* ========================================================================== */

typedef enum {
    BCL_EVENT_READABLE   = 0x01,
    BCL_EVENT_WRITABLE   = 0x02,
    BCL_EVENT_EXCEPTION  = 0x04
} bcl_event_type_t;

typedef enum {
    BCL_EVENT_SOURCE_FD,      /* File descriptor (sockets, files) */
    BCL_EVENT_SOURCE_TIMER,   /* Timer callback */
    BCL_EVENT_SOURCE_CUSTOM   /* Custom event (from extensions) */
} bcl_event_source_t;

/**
 * @brief Event handler entry
 */
typedef struct bcl_event {
    bcl_event_source_t source;

    union {
        struct {
            int fd;                    /* File descriptor */
            bcl_event_type_t types;    /* OR of event types */
        } fd_event;

        struct {
            uint64_t expire_time;      /* When to fire (ms since epoch) */
            uint32_t interval;         /* Repeat interval (0 = one-shot) */
        } timer_event;

        struct {
            void *data;                /* Extension-specific data */
            void (*cleanup)(void*);    /* Cleanup function */
        } custom_event;
    };

    char *callback;                    /* BCL procedure name */
    struct bcl_event *next;            /* Next in list */
} bcl_event_t;

/**
 * @brief Event loop state
 */
typedef struct {
    bcl_event_t *events;               /* Event list */
    bool running;                      /* Loop is active */
    int max_fd;                        /* Highest FD for select() */
} bcl_event_loop_t;
```

### Fase 2: Core Commands (src/bcl_event.c)

```c
/**
 * EVENT CREATE handle type callback
 */
bcl_result_t bcl_cmd_event_create(bcl_interp_t *interp, int argc,
                                  char **argv, bcl_value_t **result);

/**
 * EVENT DELETE handle [type]
 */
bcl_result_t bcl_cmd_event_delete(bcl_interp_t *interp, int argc,
                                  char **argv, bcl_value_t **result);

/**
 * EVENT TIMER milliseconds callback
 */
bcl_result_t bcl_cmd_event_timer(bcl_interp_t *interp, int argc,
                                 char **argv, bcl_value_t **result);

/**
 * EVENT PROCESS [timeout]
 */
bcl_result_t bcl_cmd_event_process(bcl_interp_t *interp, int argc,
                                   char **argv, bcl_value_t **result);

/**
 * EVENT LOOP
 */
bcl_result_t bcl_cmd_event_loop(bcl_interp_t *interp, int argc,
                                char **argv, bcl_value_t **result);

/**
 * Main EVENT dispatcher
 */
bcl_result_t bcl_cmd_event(bcl_interp_t *interp, int argc,
                           char **argv, bcl_value_t **result);
```

### Fase 3: Extension API

```c
/**
 * @brief API para extensiones registren eventos personalizados
 */

/* Registrar evento de FD */
int bcl_event_register_fd(bcl_interp_t *interp,
                         int fd,
                         bcl_event_type_t types,
                         const char *callback);

/* Registrar timer */
int bcl_event_register_timer(bcl_interp_t *interp,
                             uint32_t milliseconds,
                             const char *callback,
                             bool repeat);

/* Registrar evento personalizado */
int bcl_event_register_custom(bcl_interp_t *interp,
                              void *data,
                              void (*trigger)(bcl_interp_t*, void*),
                              void (*cleanup)(void*),
                              const char *callback);

/* Cancelar evento */
int bcl_event_unregister(bcl_interp_t *interp, int event_id);
```

### Fase 4: Extensiones Opcionales

#### EVENT_EPOLL.so (Linux)
```c
/* Backend eficiente con epoll para muchos FDs */
int bcl_extension_init(bcl_extension_api_t *api) {
    /* Reemplaza select() con epoll_wait() */
    bcl_event_set_backend("epoll", epoll_backend_funcs);
    return 0;
}
```

#### EVENT_GPIO.so (Embedded)
```c
/* Eventos GPIO con interrupciones hardware */
int bcl_extension_init(bcl_extension_api_t *api) {
    api->register_command(api->interp, "GPIO_MODE", bcl_cmd_gpio_mode);
    api->register_command(api->interp, "GPIO_READ", bcl_cmd_gpio_read);
    api->register_command(api->interp, "GPIO_WRITE", bcl_cmd_gpio_write);

    /* Usar API de eventos del core */
    bcl_event_register_custom(api->interp, &gpio_state,
                             gpio_trigger_check,
                             gpio_cleanup,
                             "GPIO_ISR");
    return 0;
}
```

---

## Parámetros de Callbacks

Los callbacks de eventos reciben parámetros según el tipo de evento:

### Eventos I/O (READABLE/WRITABLE)
```bcl
PROC ON_IO_EVENT WITH handle DO
    # handle = socket/file/stdin que disparó el evento
    SET data [SOCKET RECV $handle 1024]
    PUTS "Recibido de $handle: $data"
END
```

### Eventos TIMER
```bcl
PROC ON_TIMER_EVENT DO
    # Timers no reciben parámetros (son one-shot o periódicos)
    PUTS "Timer disparado"
END
```

### Eventos Personalizados (extensiones)
```bcl
PROC ON_CUSTOM_EVENT WITH param1 param2 DO
    # Parámetros definidos por la extensión
    PUTS "Custom: $param1, $param2"
END
```

---

## Integración con Comandos Existentes

### AFTER (ya existe)
```bcl
# AFTER usa internamente EVENT TIMER
AFTER 1000    # Duerme 1 segundo (blocking)

# Con eventos:
EVENT TIMER 1000 MY_CALLBACK   # No bloquea, callback en event loop
```

### SOCKET (extensión existente)
```bcl
# Antes (blocking):
SET data [SOCKET RECV $sock 1024]

# Con eventos (non-blocking):
EVENT CREATE $sock READABLE ON_DATA
PROC ON_DATA DO
    GLOBAL sock
    SET data [SOCKET RECV $sock 1024]
    PUTS "Recibido: $data"
END
```

---

## Complejidad de Implementación

### Core Event System:
- **bcl.h**: +150 líneas (structs y API)
- **bcl_event.c**: ~800 líneas
  - Event loop con select()
  - Timer queue
  - Callback dispatch
- **bcl_commands.c**: +20 líneas (registrar EVENT)

### Extensiones Opcionales:
- **event_epoll.so**: ~300 líneas
- **event_gpio.so**: ~500 líneas

**Total**: ~1500 líneas en core, extensiones opcionales separadas

---

## Compatibilidad y Portabilidad

### Core (select/poll):
- ✓ Linux, BSD, macOS, Windows (con Winsock)
- ✓ POSIX compliant
- ✓ Funciona en embedded con newlib

### Extensiones:
- epoll: Solo Linux 2.6+
- kqueue: BSD/macOS
- IOCP: Windows
- GPIO: Embedded (STM32, ESP32, Raspberry Pi)

---

## Decisión Recomendada

### **Sistema Híbrido**: Core + Extensiones

**Implementar en el Core:**
1. Event loop básico (select/poll)
2. Comando EVENT con subcomandos
3. Timer queue
4. API para extensiones

**Implementar como Extensiones:**
1. Backends optimizados (epoll, kqueue)
2. GPIO y hardware-specific
3. Eventos especializados

**Justificación:**
- Core simple y portable (select funciona everywhere)
- Funcionalidad básica siempre disponible
- Optimizaciones y features avanzados opcionales
- Extensible a nuevos tipos de eventos sin tocar core
- Balance perfecto entre simplicidad y potencia

---

## Roadmap de Implementación

### Milestone 1: Event System Core
- [ ] Estructuras de datos (bcl.h)
- [ ] Event loop básico con select()
- [ ] Comando EVENT con subcomandos
- [ ] Timer queue
- [ ] Tests básicos

### Milestone 2: Integración SOCKET
- [ ] Modificar SOCKET para soporte non-blocking
- [ ] Ejemplo servidor asíncrono
- [ ] Tests de stress (muchos clientes)

### Milestone 3: Extensión EPOLL (Linux)
- [ ] Backend epoll_wait()
- [ ] Benchmark vs select()
- [ ] Documentación

### Milestone 4: Extensión GPIO (Embedded)
- [ ] Soporte STM32/ESP32
- [ ] Interrupciones hardware
- [ ] Ejemplos embedded

### Milestone 5: Documentación
- [ ] Manual de eventos (LaTeX)
- [ ] API para extensiones
- [ ] Ejemplos completos
- [ ] Guía de performance

---

## Conclusión

**Recomendación Final: Sistema Híbrido**

- **Core**: Event loop básico, portable, siempre disponible
- **Extensiones**: Optimizaciones y features específicos

Esta arquitectura permite:
1. BCL funciona en cualquier plataforma (core simple)
2. Aplicaciones básicas sin dependencias extras
3. Optimizaciones via extensiones cuando se necesitan
4. Extensibilidad a nuevos tipos de eventos
5. Mantiene la filosofía de BCL: simple pero potente

**Próximo paso**: Implementar Milestone 1 (Event System Core)
