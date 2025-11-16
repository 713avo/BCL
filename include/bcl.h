/**
 * @file bcl.h
 * @brief BCL Interpreter - Header principal
 * @version 1.5.0
 * @date 2025-10
 *
 * BCL (Basic Command Language) - Intérprete completo
 * Inspirado en Tcl 8.x con sintaxis tipo BASIC
 *
 * Características:
 * - Case-insensitive
 * - Todo valor es STRING
 * - Soporte Unicode completo
 * - Portable a microcontroladores
 */

#ifndef BCL_H
#define BCL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

/* ========================================================================== */
/* CONFIGURACIÓN Y VERSIÓN                                                   */
/* ========================================================================== */

#define BCL_VERSION "1.6"
#define BCL_VERSION_MAJOR 1
#define BCL_VERSION_MINOR 6
#define BCL_VERSION_PATCH 0

/* Límites configurables */
#define BCL_MAX_TOKEN_LEN       4096    /**< Longitud máxima de un token */
#define BCL_MAX_LINE_LEN        8192    /**< Longitud máxima de línea */
#define BCL_MAX_RECURSION       1000    /**< Profundidad máxima de recursión */
#define BCL_MAX_SCOPE_DEPTH     256     /**< Profundidad máxima de scopes */
#define BCL_HASH_TABLE_SIZE     256     /**< Tamaño de hash tables */

/* Flags de compilación condicional */
#ifdef BCL_EMBEDDED
  #define BCL_NO_READLINE       /* Sin readline en embebidos */
  #define BCL_NO_FILES          /* Sin I/O de archivos */
  #define BCL_NO_EXEC           /* Sin EXEC */
  #undef BCL_MAX_RECURSION
  #define BCL_MAX_RECURSION 100 /* Menor recursión en embebidos */
#endif

/* ========================================================================== */
/* TIPOS DE DATOS BÁSICOS                                                    */
/* ========================================================================== */

/**
 * @brief Tipo de resultado de operaciones
 */
typedef enum {
    BCL_OK = 0,                 /**< Operación exitosa */
    BCL_ERROR = 1,              /**< Error genérico */
    BCL_BREAK = 2,              /**< Break en bucle */
    BCL_CONTINUE = 3,           /**< Continue en bucle */
    BCL_RETURN = 4,             /**< Return desde procedimiento */
    BCL_EXIT = 5                /**< Exit del intérprete */
} bcl_result_t;

/**
 * @brief Estructura de string dinámica
 */
typedef struct {
    char *data;                 /**< Puntero a los datos */
    size_t len;                 /**< Longitud actual */
    size_t capacity;            /**< Capacidad alocada */
} bcl_string_t;

/**
 * @brief Valor BCL (siempre string internamente)
 */
typedef struct {
    bcl_string_t str;           /**< Representación string */
    bool is_cached_number;      /**< Cache: ¿es número válido? */
    double cached_number;       /**< Cache: valor numérico */
} bcl_value_t;

/* ========================================================================== */
/* HASH TABLE (case-insensitive)                                             */
/* ========================================================================== */

/**
 * @brief Entrada de hash table
 */
typedef struct bcl_hash_entry {
    char *key;                          /**< Clave (case-insensitive) */
    bcl_value_t *value;                 /**< Valor asociado */
    struct bcl_hash_entry *next;        /**< Siguiente en lista de colisión */
} bcl_hash_entry_t;

/**
 * @brief Tabla hash
 */
typedef struct {
    bcl_hash_entry_t *buckets[BCL_HASH_TABLE_SIZE];
    size_t count;                       /**< Número de elementos */
} bcl_hash_table_t;

/* ========================================================================== */
/* PROCEDIMIENTOS                                                            */
/* ========================================================================== */

/**
 * @brief Parámetro de procedimiento
 */
typedef struct {
    char *name;                 /**< Nombre del parámetro */
    bool optional;              /**< true si es @param */
} bcl_param_t;

/**
 * @brief Definición de procedimiento
 */
typedef struct {
    char *name;                 /**< Nombre del procedimiento */
    bcl_param_t *params;        /**< Array de parámetros */
    size_t param_count;         /**< Número de parámetros */
    struct bcl_block *body_block; /**< Bloque del cuerpo (pre-parseado) */
} bcl_procedure_t;

/* ========================================================================== */
/* ÁMBITOS (SCOPES)                                                          */
/* ========================================================================== */

/**
 * @brief Ámbito de ejecución (para procedimientos)
 */
typedef struct bcl_scope {
    bcl_hash_table_t *vars;             /**< Variables locales */
    bcl_hash_table_t *global_refs;      /**< Referencias a globales (GLOBAL) */
    bcl_hash_table_t *global_prefixes;  /**< Prefijos globales para arrays */
    struct bcl_scope *parent;           /**< Ámbito padre */
} bcl_scope_t;

/* ========================================================================== */
/* HANDLES DE ARCHIVOS                                                       */
/* ========================================================================== */

#ifndef BCL_NO_FILES
/**
 * @brief Modo de apertura de archivos
 */
typedef enum {
    BCL_FILE_READ,              /**< Modo lectura (R) */
    BCL_FILE_WRITE,             /**< Modo escritura (W) */
    BCL_FILE_APPEND,            /**< Modo append (A) */
    BCL_FILE_READ_WRITE         /**< Modo lectura/escritura (RW) */
} bcl_file_mode_t;

/**
 * @brief Handle de archivo
 */
typedef struct {
    FILE *fp;                   /**< Puntero al archivo */
    bcl_file_mode_t mode;       /**< Modo de apertura */
    char *path;                 /**< Ruta del archivo */
    bool eof_reached;           /**< Flag EOF */
} bcl_file_handle_t;
#endif

/* ========================================================================== */
/* EXTENSIONES DINÁMICAS                                                     */
/* ========================================================================== */

/* Forward declaration para bcl_interp */
struct bcl_interp;

/**
 * @brief Tipo de función para comandos de extensión
 */
typedef bcl_result_t (*bcl_extension_cmd_func_t)(
    struct bcl_interp *interp,
    int argc,
    char **argv,
    bcl_value_t **result
);

/**
 * @brief Estructura de inicialización de extensión
 *
 * Cada extensión (.so) debe exportar una función:
 *   int bcl_extension_init(bcl_extension_api_t *api)
 *
 * Esta función debe:
 *   1. Verificar api->version es compatible
 *   2. Registrar comandos vía api->register_command()
 *   3. Retornar 0 si éxito, -1 si error
 */
typedef struct {
    int version;                        /**< Versión de la API (debe ser BCL_EXTENSION_API_VERSION) */
    void *interp;                       /**< Puntero al intérprete (opaco para extensiones) */

    /**
     * @brief Registra un nuevo comando desde la extensión
     * @param interp Intérprete (usar el de api->interp)
     * @param name Nombre del comando (case-insensitive)
     * @param func Función que implementa el comando
     * @return 0 si éxito, -1 si error
     */
    int (*register_command)(struct bcl_interp *interp, const char *name,
                           bcl_extension_cmd_func_t func);

    /**
     * @brief Establece mensaje de error
     */
    void (*set_error)(struct bcl_interp *interp, const char *fmt, ...);

    /**
     * @brief Crea un valor BCL
     */
    bcl_value_t *(*value_create)(const char *str);

    /**
     * @brief Destruye un valor BCL
     */
    void (*value_destroy)(bcl_value_t *val);

    /**
     * @brief Obtiene string de un valor
     */
    const char *(*value_get)(bcl_value_t *val);

    /**
     * @brief Establece variable
     */
    bcl_result_t (*var_set)(struct bcl_interp *interp, const char *name, const char *value);

    /**
     * @brief Obtiene variable
     */
    bcl_value_t *(*var_get)(struct bcl_interp *interp, const char *name);
} bcl_extension_api_t;

#define BCL_EXTENSION_API_VERSION 1

/**
 * @brief Tipo de función de inicialización de extensión
 */
typedef int (*bcl_extension_init_func_t)(bcl_extension_api_t *api);

/**
 * @brief Handle de extensión cargada
 */
typedef struct bcl_extension {
    void *dl_handle;                    /**< Handle de dlopen() */
    char *path;                         /**< Ruta del archivo .so */
    char *name;                         /**< Nombre de la extensión */
    struct bcl_extension *next;         /**< Siguiente en lista enlazada */
} bcl_extension_t;

/* ========================================================================== */
/* SISTEMA DE EVENTOS                                                        */
/* ========================================================================== */

/**
 * @brief Tipos de eventos
 */
typedef enum {
    BCL_EVENT_READABLE   = 0x01,        /**< Descriptor listo para lectura */
    BCL_EVENT_WRITABLE   = 0x02,        /**< Descriptor listo para escritura */
    BCL_EVENT_EXCEPTION  = 0x04         /**< Condición excepcional */
} bcl_event_type_t;

/**
 * @brief Fuente del evento
 */
typedef enum {
    BCL_EVENT_SOURCE_FD,                /**< File descriptor (socket, file, stdin) */
    BCL_EVENT_SOURCE_TIMER              /**< Timer callback */
} bcl_event_source_t;

/**
 * @brief Entrada de evento registrado
 */
typedef struct bcl_event {
    bcl_event_source_t source;          /**< Tipo de fuente */

    union {
        struct {
            int fd;                     /**< File descriptor */
            bcl_event_type_t types;     /**< Tipos de eventos (OR de flags) */
        } fd_event;

        struct {
            uint64_t expire_time_ms;    /**< Cuándo disparar (ms desde epoch) */
            uint32_t interval_ms;       /**< Intervalo de repetición (0 = one-shot) */
        } timer_event;
    };

    char *callback;                     /**< Nombre del procedimiento BCL */
    struct bcl_event *next;             /**< Siguiente en lista */
} bcl_event_t;

/**
 * @brief Estado del event loop
 */
typedef struct bcl_event_loop {
    bcl_event_t *events;                /**< Lista de eventos registrados */
    bool running;                       /**< true si event loop está activo */
    int max_fd;                         /**< FD más alto (para select) */
    size_t event_count;                 /**< Número de eventos registrados */
} bcl_event_loop_t;

/* Forward declaration del intérprete */
struct bcl_interp;

/* ========================================================================== */
/* INTÉRPRETE PRINCIPAL                                                      */
/* ========================================================================== */

/**
 * @brief Estado del intérprete BCL
 */
typedef struct bcl_interp {
    /* Variables y procedimientos */
    bcl_hash_table_t *global_vars;      /**< Variables globales */
    bcl_hash_table_t *procedures;       /**< Procedimientos definidos */

    /* Stack de scopes */
    bcl_scope_t **scope_stack;          /**< Stack de ámbitos */
    size_t scope_depth;                 /**< Profundidad actual */

    /* Handles de archivos */
#ifndef BCL_NO_FILES
    bcl_hash_table_t *file_handles;     /**< Handles de archivos abiertos */
    size_t next_handle_id;              /**< ID para próximo handle */
#endif

    /* Extensiones dinámicas */
    bcl_extension_t *extensions;        /**< Lista de extensiones cargadas */
    bcl_hash_table_t *extension_cmds;   /**< Comandos registrados por extensiones */

    /* Sistema de eventos */
    struct bcl_event_loop *event_loop;  /**< Event loop (NULL si no inicializado) */

    /* Estado de control de flujo */
    bcl_result_t flow_result;           /**< Resultado de flujo (BREAK, etc.) */
    bcl_value_t *return_value;          /**< Valor de retorno (RETURN) */
    int exit_code;                      /**< Código de salida (EXIT) */

    /* Argumentos del script */
    int argc;                           /**< Número de argumentos */
    char **argv;                        /**< Array de argumentos */

    /* Configuración */
    bool interactive;                   /**< Modo REPL */
    size_t recursion_depth;             /**< Profundidad de recursión actual */

    /* Buffer de error */
    char error_msg[BCL_MAX_LINE_LEN];   /**< Último mensaje de error */
} bcl_interp_t;

/* ========================================================================== */
/* FUNCIONES PÚBLICAS - INTÉRPRETE                                           */
/* ========================================================================== */

/**
 * @brief Crea un nuevo intérprete
 * @return Puntero al intérprete o NULL si falla
 */
bcl_interp_t *bcl_interp_create(void);

/**
 * @brief Destruye un intérprete y libera recursos
 * @param interp Intérprete a destruir
 */
void bcl_interp_destroy(bcl_interp_t *interp);

/**
 * @brief Evalúa código BCL
 * @param interp Intérprete
 * @param code Código a evaluar
 * @param result Puntero donde guardar resultado (puede ser NULL)
 * @return BCL_OK si éxito, BCL_ERROR si error
 */
bcl_result_t bcl_eval(bcl_interp_t *interp, const char *code,
                      bcl_value_t **result);

/**
 * @brief Evalúa código BCL con parser estructurado (soporta bloques)
 * @param interp Intérprete
 * @param code Código a evaluar
 * @return BCL_OK si éxito, BCL_ERROR si error
 */
bcl_result_t bcl_eval_structured(bcl_interp_t *interp, const char *code);

/**
 * @brief Evalúa un archivo BCL
 * @param interp Intérprete
 * @param filename Nombre del archivo
 * @return BCL_OK si éxito, BCL_ERROR si error
 */
bcl_result_t bcl_eval_file(bcl_interp_t *interp, const char *filename);

/**
 * @brief Inicia modo REPL (Read-Eval-Print Loop)
 * @param interp Intérprete
 * @return Código de salida
 */
int bcl_repl(bcl_interp_t *interp);

/**
 * @brief Obtiene último mensaje de error
 * @param interp Intérprete
 * @return String con mensaje de error
 */
const char *bcl_get_error(bcl_interp_t *interp);

/**
 * @brief Establece mensaje de error
 * @param interp Intérprete
 * @param fmt Formato printf-style
 */
void bcl_set_error(bcl_interp_t *interp, const char *fmt, ...);

/* ========================================================================== */
/* FUNCIONES PÚBLICAS - STRINGS                                              */
/* ========================================================================== */

bcl_string_t *bcl_string_create(const char *initial);
bcl_string_t *bcl_string_create_empty(size_t capacity);
void bcl_string_destroy(bcl_string_t *str);
void bcl_string_append(bcl_string_t *str, const char *text);
void bcl_string_append_char(bcl_string_t *str, char c);
void bcl_string_clear(bcl_string_t *str);
char *bcl_string_cstr(bcl_string_t *str);

/* ========================================================================== */
/* FUNCIONES PÚBLICAS - VALORES                                              */
/* ========================================================================== */

bcl_value_t *bcl_value_create(const char *str);
bcl_value_t *bcl_value_create_empty(void);
void bcl_value_destroy(bcl_value_t *val);
void bcl_value_set(bcl_value_t *val, const char *str);
const char *bcl_value_get(bcl_value_t *val);
double bcl_value_to_number(bcl_value_t *val, bool *ok);
bool bcl_value_to_bool(bcl_value_t *val);
bcl_value_t *bcl_value_clone(bcl_value_t *val);

/* ========================================================================== */
/* FUNCIONES PÚBLICAS - HASH TABLES                                          */
/* ========================================================================== */

bcl_hash_table_t *bcl_hash_create(void);
void bcl_hash_destroy(bcl_hash_table_t *table);
void bcl_hash_set(bcl_hash_table_t *table, const char *key, bcl_value_t *value);
bcl_value_t *bcl_hash_get(bcl_hash_table_t *table, const char *key);
bool bcl_hash_exists(bcl_hash_table_t *table, const char *key);
void bcl_hash_remove(bcl_hash_table_t *table, const char *key);
char **bcl_hash_keys(bcl_hash_table_t *table, size_t *count);

/* ========================================================================== */
/* FUNCIONES PÚBLICAS - VARIABLES                                            */
/* ========================================================================== */

bcl_result_t bcl_var_set(bcl_interp_t *interp, const char *name,
                         const char *value);
bcl_value_t *bcl_var_get(bcl_interp_t *interp, const char *name);
bool bcl_var_exists(bcl_interp_t *interp, const char *name);
bcl_result_t bcl_var_unset(bcl_interp_t *interp, const char *name);

/* ========================================================================== */
/* FUNCIONES PÚBLICAS - SCOPES                                               */
/* ========================================================================== */

bcl_result_t bcl_scope_push(bcl_interp_t *interp);
bcl_result_t bcl_scope_pop(bcl_interp_t *interp);

/* ========================================================================== */
/* FUNCIONES PÚBLICAS - PROCEDIMIENTOS                                       */
/* ========================================================================== */

bcl_result_t bcl_proc_define(bcl_interp_t *interp, const char *name,
                             bcl_param_t *params, size_t param_count,
                             struct bcl_block *body_block);
bcl_result_t bcl_proc_call(bcl_interp_t *interp, const char *name,
                           bcl_value_t **args, size_t arg_count,
                           bcl_value_t **result);
bool bcl_proc_exists(bcl_interp_t *interp, const char *name);

/* ========================================================================== */
/* FUNCIONES PÚBLICAS - UTILIDADES                                           */
/* ========================================================================== */

/**
 * @brief Convierte string a minúsculas (para comparaciones case-insensitive)
 */
char *bcl_strtolower(const char *str);

/**
 * @brief Compara strings ignorando case
 */
int bcl_strcasecmp(const char *s1, const char *s2);

/**
 * @brief Duplica string (malloc)
 */
char *bcl_strdup(const char *str);

/**
 * @brief Verifica si un string es numérico
 */
bool bcl_is_number(const char *str);

/**
 * @brief Convierte string a número
 */
double bcl_str_to_number(const char *str, bool *ok);

/**
 * @brief Compara n caracteres ignorando case
 */
int bcl_strncasecmp(const char *s1, const char *s2, size_t n);

/* ========================================================================== */
/* FUNCIONES PÚBLICAS - BLOQUES ESTRUCTURADOS                                */
/* ========================================================================== */

/* Tipo opaco para bloques */
typedef struct bcl_block bcl_block_t;

/**
 * @brief Parsea código en una estructura de bloques
 */
bcl_block_t *bcl_parse_blocks(const char *code);

/**
 * @brief Ejecuta un bloque estructurado
 */
bcl_result_t bcl_exec_block(bcl_interp_t *interp, bcl_block_t *block);

/**
 * @brief Libera un bloque y todos sus hijos
 */
void bcl_block_free(bcl_block_t *block);

/* ========================================================================== */
/* FUNCIONES PÚBLICAS - PARSER                                               */
/* ========================================================================== */

/**
 * @brief Decodifica secuencias de escape
 */
char *bcl_decode_escapes(const char *str);

/**
 * @brief Extrae el próximo token
 */
bool bcl_next_token(const char *line, size_t *pos, char *token, size_t token_size);

/**
 * @brief Expande variables $var en un string
 */
char *bcl_expand_vars(bcl_interp_t *interp, const char *str);

/**
 * @brief Expande subcomandos [..] en un string (recursivo)
 */
char *bcl_expand_subcommands(bcl_interp_t *interp, const char *str);

/**
 * @brief Evalúa un subcomando entre []
 */
char *bcl_eval_subcommand(bcl_interp_t *interp, const char *cmd);

/**
 * @brief Parsea una línea en tokens procesados
 */
char **bcl_parse_line(bcl_interp_t *interp, const char *line, int *argc);

/**
 * @brief Libera array de tokens
 */
void bcl_free_tokens(char **tokens, int count);

/* ========================================================================== */
/* MACROS ÚTILES                                                              */
/* ========================================================================== */

#define BCL_UNUSED(x) ((void)(x))

#ifdef DEBUG
  #define BCL_DEBUG(fmt, ...) \
    fprintf(stderr, "[DEBUG] %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
  #define BCL_DEBUG(fmt, ...) ((void)0)
#endif

#define BCL_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "ASSERTION FAILED: %s at %s:%d\n", \
                    msg, __FILE__, __LINE__); \
            abort(); \
        } \
    } while(0)

#endif /* BCL_H */
