/**
 * @file bcl_expr.c
 * @brief Evaluador de expresiones aritméticas y lógicas
 * @note Implementación con algoritmo Shunting Yard + evaluación RPN
 */

#define _USE_MATH_DEFINES
#include "bcl.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ========================================================================== */
/* TIPOS DE TOKENS                                                           */
/* ========================================================================== */

typedef enum {
    TOK_NUMBER,
    TOK_PLUS,
    TOK_MINUS,
    TOK_MULTIPLY,
    TOK_DIVIDE,
    TOK_MODULO,
    TOK_POWER,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_COMMA,     /* , */
    TOK_LT,        /* < */
    TOK_LE,        /* <= */
    TOK_GT,        /* > */
    TOK_GE,        /* >= */
    TOK_EQ,        /* == */
    TOK_NE,        /* != */
    TOK_AND,       /* && o AND */
    TOK_OR,        /* || o OR */
    TOK_NOT,       /* ! o NOT */
    TOK_FUNCTION,  /* sin, cos, sqrt, etc */
    TOK_END
} token_type_t;

typedef struct {
    token_type_t type;
    double value;
    char func_name[32];
} expr_token_t;

/* ========================================================================== */
/* PRECEDENCIA DE OPERADORES                                                 */
/* ========================================================================== */

static int get_precedence(token_type_t type) {
    switch (type) {
        case TOK_OR:       return 1;
        case TOK_AND:      return 2;
        case TOK_EQ:
        case TOK_NE:       return 3;
        case TOK_LT:
        case TOK_LE:
        case TOK_GT:
        case TOK_GE:       return 4;
        case TOK_PLUS:
        case TOK_MINUS:    return 5;
        case TOK_MULTIPLY:
        case TOK_DIVIDE:
        case TOK_MODULO:   return 6;
        case TOK_NOT:      return 7;
        case TOK_POWER:    return 8;
        default:           return 0;
    }
}

static bool is_right_associative(token_type_t type) {
    return type == TOK_POWER || type == TOK_NOT;
}

/* ========================================================================== */
/* FUNCIONES MATEMÁTICAS                                                     */
/* ========================================================================== */

static double eval_function(const char *name, double arg) {
    /* Trigonométricas */
    if (bcl_strcasecmp(name, "sin") == 0) return sin(arg);
    if (bcl_strcasecmp(name, "cos") == 0) return cos(arg);
    if (bcl_strcasecmp(name, "tan") == 0) return tan(arg);
    if (bcl_strcasecmp(name, "asin") == 0) return asin(arg);
    if (bcl_strcasecmp(name, "acos") == 0) return acos(arg);
    if (bcl_strcasecmp(name, "atan") == 0) return atan(arg);

    /* Hiperbólicas */
    if (bcl_strcasecmp(name, "sinh") == 0) return sinh(arg);
    if (bcl_strcasecmp(name, "cosh") == 0) return cosh(arg);
    if (bcl_strcasecmp(name, "tanh") == 0) return tanh(arg);

    /* Raíces y potencias */
    if (bcl_strcasecmp(name, "sqrt") == 0) return sqrt(arg);
    if (bcl_strcasecmp(name, "cbrt") == 0) return cbrt(arg);  /* Raíz cúbica */

    /* Redondeo */
    if (bcl_strcasecmp(name, "abs") == 0) return fabs(arg);
    if (bcl_strcasecmp(name, "int") == 0) return floor(arg);
    if (bcl_strcasecmp(name, "double") == 0) return arg;
    if (bcl_strcasecmp(name, "ceil") == 0) return ceil(arg);
    if (bcl_strcasecmp(name, "floor") == 0) return floor(arg);
    if (bcl_strcasecmp(name, "round") == 0) return round(arg);

    /* Logaritmos y exponenciales */
    if (bcl_strcasecmp(name, "ln") == 0) return log(arg);
    if (bcl_strcasecmp(name, "log") == 0) return log10(arg);
    if (bcl_strcasecmp(name, "log10") == 0) return log10(arg);
    if (bcl_strcasecmp(name, "log2") == 0) return log2(arg);
    if (bcl_strcasecmp(name, "exp") == 0) return exp(arg);

    /* Signo */
    if (bcl_strcasecmp(name, "sign") == 0) {
        if (arg > 0) return 1.0;
        if (arg < 0) return -1.0;
        return 0.0;
    }

    /* Aleatorios */
    if (bcl_strcasecmp(name, "rand") == 0) return (double)rand() / RAND_MAX;

    /* Grados/Radianes */
    if (bcl_strcasecmp(name, "rad") == 0) return arg * M_PI / 180.0;
    if (bcl_strcasecmp(name, "deg") == 0) return arg * 180.0 / M_PI;

    return 0.0;
}

static double eval_function2(const char *name, double arg1, double arg2) {
    if (bcl_strcasecmp(name, "pow") == 0) return pow(arg1, arg2);
    if (bcl_strcasecmp(name, "hypot") == 0) return hypot(arg1, arg2);
    if (bcl_strcasecmp(name, "atan2") == 0) return atan2(arg1, arg2);
    if (bcl_strcasecmp(name, "min") == 0) return (arg1 < arg2) ? arg1 : arg2;
    if (bcl_strcasecmp(name, "max") == 0) return (arg1 > arg2) ? arg1 : arg2;
    if (bcl_strcasecmp(name, "fmod") == 0) return fmod(arg1, arg2);
    return 0.0;
}

/* Verifica si una función requiere 2 argumentos */
static bool is_function2(const char *name) {
    return (bcl_strcasecmp(name, "pow") == 0 ||
            bcl_strcasecmp(name, "hypot") == 0 ||
            bcl_strcasecmp(name, "atan2") == 0 ||
            bcl_strcasecmp(name, "min") == 0 ||
            bcl_strcasecmp(name, "max") == 0 ||
            bcl_strcasecmp(name, "fmod") == 0);
}

/* ========================================================================== */
/* TOKENIZADOR DE EXPRESIONES                                                */
/* ========================================================================== */

static const char *skip_spaces(const char *str) {
    while (*str && isspace((unsigned char)*str)) str++;
    return str;
}

static bool is_func_char(char c) {
    return isalpha((unsigned char)c) || c == '_';
}

static const char *parse_expr_token(const char *expr, expr_token_t *tok) {
    expr = skip_spaces(expr);

    if (!*expr) {
        tok->type = TOK_END;
        return expr;
    }

    /* Números */
    if (isdigit((unsigned char)*expr) || (*expr == '.' && isdigit((unsigned char)expr[1]))) {
        char *endptr;
        tok->value = strtod(expr, &endptr);
        tok->type = TOK_NUMBER;
        return endptr;
    }

    /* Funciones (identificadores) */
    if (is_func_char(*expr)) {
        size_t i = 0;
        while (is_func_char(*expr) && i < sizeof(tok->func_name) - 1) {
            tok->func_name[i++] = *expr++;
        }
        tok->func_name[i] = '\0';

        /* Verificar si es operador lógico */
        if (bcl_strcasecmp(tok->func_name, "AND") == 0) {
            tok->type = TOK_AND;
        } else if (bcl_strcasecmp(tok->func_name, "OR") == 0) {
            tok->type = TOK_OR;
        } else if (bcl_strcasecmp(tok->func_name, "NOT") == 0) {
            tok->type = TOK_NOT;
        } else {
            tok->type = TOK_FUNCTION;
        }
        return expr;
    }

    /* Operadores de dos caracteres */
    if (expr[0] && expr[1]) {
        if (expr[0] == '*' && expr[1] == '*') {
            tok->type = TOK_POWER;
            return expr + 2;
        }
        if (expr[0] == '<' && expr[1] == '=') {
            tok->type = TOK_LE;
            return expr + 2;
        }
        if (expr[0] == '>' && expr[1] == '=') {
            tok->type = TOK_GE;
            return expr + 2;
        }
        if (expr[0] == '!' && expr[1] == '=') {
            tok->type = TOK_NE;
            return expr + 2;
        }
        if (expr[0] == '&' && expr[1] == '&') {
            tok->type = TOK_AND;
            return expr + 2;
        }
        if (expr[0] == '|' && expr[1] == '|') {
            tok->type = TOK_OR;
            return expr + 2;
        }
        if (expr[0] == '=' && expr[1] == '=') {
            tok->type = TOK_EQ;
            return expr + 2;
        }
    }

    /* Operadores de un carácter */
    switch (*expr) {
        case '+': tok->type = TOK_PLUS; return expr + 1;
        case '-': tok->type = TOK_MINUS; return expr + 1;
        case '*': tok->type = TOK_MULTIPLY; return expr + 1;
        case '/': tok->type = TOK_DIVIDE; return expr + 1;
        case '%': tok->type = TOK_MODULO; return expr + 1;
        case '^': tok->type = TOK_POWER; return expr + 1;
        case '(': tok->type = TOK_LPAREN; return expr + 1;
        case ')': tok->type = TOK_RPAREN; return expr + 1;
        case ',': tok->type = TOK_COMMA; return expr + 1;
        case '<': tok->type = TOK_LT; return expr + 1;
        case '>': tok->type = TOK_GT; return expr + 1;
        case '!': tok->type = TOK_NOT; return expr + 1;
        default:
            tok->type = TOK_END;
            return expr;
    }
}

/* ========================================================================== */
/* EVALUADOR RPN (Reverse Polish Notation)                                   */
/* ========================================================================== */

static double apply_operator(token_type_t op, double left, double right) {
    switch (op) {
        case TOK_PLUS:     return left + right;
        case TOK_MINUS:    return left - right;
        case TOK_MULTIPLY: return left * right;
        case TOK_DIVIDE:   return right != 0.0 ? left / right : 0.0;
        case TOK_MODULO:   return fmod(left, right);
        case TOK_POWER:    return pow(left, right);
        case TOK_LT:       return left < right ? 1.0 : 0.0;
        case TOK_LE:       return left <= right ? 1.0 : 0.0;
        case TOK_GT:       return left > right ? 1.0 : 0.0;
        case TOK_GE:       return left >= right ? 1.0 : 0.0;
        case TOK_EQ:       return fabs(left - right) < 1e-10 ? 1.0 : 0.0;
        case TOK_NE:       return fabs(left - right) >= 1e-10 ? 1.0 : 0.0;
        case TOK_AND:      return (left != 0.0 && right != 0.0) ? 1.0 : 0.0;
        case TOK_OR:       return (left != 0.0 || right != 0.0) ? 1.0 : 0.0;
        default:           return 0.0;
    }
}

/* ========================================================================== */
/* EVALUADOR PRINCIPAL (SHUNTING YARD)                                       */
/* ========================================================================== */

bcl_result_t bcl_cmd_expr(bcl_interp_t *interp, int argc, char **argv,
                          bcl_value_t **result) {
    if (argc < 1) {
        bcl_set_error(interp, "EXPR: wrong # args: should be \"EXPR expression\"");
        return BCL_ERROR;
    }

    /* Concatenar todos los argumentos */
    bcl_string_t *expr_str = bcl_string_create("");
    for (int i = 0; i < argc; i++) {
        if (i > 0) bcl_string_append(expr_str, " ");
        bcl_string_append(expr_str, argv[i]);
    }

    const char *expr = bcl_string_cstr(expr_str);

    /* Stacks para Shunting Yard */
    double value_stack[256];
    int value_top = -1;

    expr_token_t op_stack[256];
    int op_top = -1;

    /* Parsear y evaluar */
    expr_token_t tok;
    const char *pos = expr;

    while (1) {
        pos = parse_expr_token(pos, &tok);

        if (tok.type == TOK_END) break;

        if (tok.type == TOK_NUMBER) {
            value_stack[++value_top] = tok.value;
        }
        else if (tok.type == TOK_FUNCTION) {
            op_stack[++op_top] = tok;
        }
        else if (tok.type == TOK_LPAREN) {
            op_stack[++op_top] = tok;
        }
        else if (tok.type == TOK_COMMA) {
            /* La coma solo separa argumentos, no hace nada en el evaluador */
            /* Los valores ya están en el stack */
            continue;
        }
        else if (tok.type == TOK_RPAREN) {
            /* Pop hasta encontrar ( */
            while (op_top >= 0 && op_stack[op_top].type != TOK_LPAREN) {
                expr_token_t op = op_stack[op_top--];

                if (op.type == TOK_FUNCTION) {
                    /* Verificar si requiere 2 argumentos */
                    if (is_function2(op.func_name)) {
                        if (value_top >= 1) {
                            double arg2 = value_stack[value_top--];
                            double arg1 = value_stack[value_top--];
                            value_stack[++value_top] = eval_function2(op.func_name, arg1, arg2);
                        }
                    } else {
                        /* Función de 1 argumento */
                        if (value_top >= 0) {
                            double arg = value_stack[value_top--];
                            value_stack[++value_top] = eval_function(op.func_name, arg);
                        }
                    }
                } else if (op.type == TOK_NOT) {
                    if (value_top >= 0) {
                        value_stack[value_top] = value_stack[value_top] == 0.0 ? 1.0 : 0.0;
                    }
                } else {
                    if (value_top >= 1) {
                        double right = value_stack[value_top--];
                        double left = value_stack[value_top--];
                        value_stack[++value_top] = apply_operator(op.type, left, right);
                    }
                }
            }
            if (op_top >= 0) op_top--; /* Pop ( */

            /* Verificar si hay una función después del paréntesis */
            if (op_top >= 0 && op_stack[op_top].type == TOK_FUNCTION) {
                expr_token_t func = op_stack[op_top--];

                /* Verificar si requiere 2 argumentos */
                if (is_function2(func.func_name)) {
                    if (value_top >= 1) {
                        double arg2 = value_stack[value_top--];
                        double arg1 = value_stack[value_top--];
                        value_stack[++value_top] = eval_function2(func.func_name, arg1, arg2);
                    }
                } else {
                    /* Función de 1 argumento */
                    if (value_top >= 0) {
                        double arg = value_stack[value_top--];
                        value_stack[++value_top] = eval_function(func.func_name, arg);
                    }
                }
            }
        }
        else {
            /* Operador */
            while (op_top >= 0 && op_stack[op_top].type != TOK_LPAREN) {
                int prec1 = get_precedence(tok.type);
                int prec2 = get_precedence(op_stack[op_top].type);

                if (prec2 > prec1 || (prec2 == prec1 && !is_right_associative(tok.type))) {
                    expr_token_t op = op_stack[op_top--];

                    if (op.type == TOK_NOT) {
                        if (value_top >= 0) {
                            value_stack[value_top] = value_stack[value_top] == 0.0 ? 1.0 : 0.0;
                        }
                    } else if (value_top >= 1) {
                        double right = value_stack[value_top--];
                        double left = value_stack[value_top--];
                        value_stack[++value_top] = apply_operator(op.type, left, right);
                    }
                } else {
                    break;
                }
            }
            op_stack[++op_top] = tok;
        }
    }

    /* Procesar operadores restantes */
    while (op_top >= 0) {
        expr_token_t op = op_stack[op_top--];

        if (op.type == TOK_NOT) {
            if (value_top >= 0) {
                value_stack[value_top] = value_stack[value_top] == 0.0 ? 1.0 : 0.0;
            }
        } else if (value_top >= 1) {
            double right = value_stack[value_top--];
            double left = value_stack[value_top--];
            value_stack[++value_top] = apply_operator(op.type, left, right);
        }
    }

    /* Resultado */
    double final_result = value_top >= 0 ? value_stack[value_top] : 0.0;

    char buf[64];
    /* Si es entero, mostrar sin decimales */
    if (final_result == floor(final_result)) {
        snprintf(buf, sizeof(buf), "%.0f", final_result);
    } else {
        snprintf(buf, sizeof(buf), "%.15g", final_result);
    }

    *result = bcl_value_create(buf);
    bcl_string_destroy(expr_str);

    return BCL_OK;
}
