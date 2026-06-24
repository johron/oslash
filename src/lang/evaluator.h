#ifndef EVALUATOR_H
#define EVALUATOR_H

#include "parser.h"
#include "lexer.h"

#include <stdbool.h>

typedef enum {
    ENTRY_VAR,
    ENTRY_FUNC,
} EntryType;

static inline const char* get_entry_type_string(EntryType type) {
    switch (type) {
        case ENTRY_VAR: return "ENTRY_VAR";
        case ENTRY_FUNC: return "ENTRY_FUNC";
        default: {
            fprintf(stderr, "Unrecognized entry type '%d'\n", type);
            exit(1);
        }
    }
};

static inline const char* get_entry_type_string_pretty(EntryType type) {
    switch (type) {
        case ENTRY_VAR: return "variable";
        case ENTRY_FUNC: return "function";
        default: {
            fprintf(stderr, "Unrecognized entry type '%d'\n", type);
            exit(1);
        }
    }
};

typedef enum {
    VAL_NUM,
    VAL_FLOAT,
    VAL_STR,
    VAL_VOID,
} ValueType;

typedef struct {
    ValueType type;
    union {
        int num_val;
        double float_val;
        char *str_val;
    };
} RuntimeValue;

typedef RuntimeValue (*RuntimeFunc)(RuntimeValue *args, size_t argc);

static inline RuntimeValue val_num(int v) {
    return (RuntimeValue){ .type = VAL_NUM, .num_val = v };
}

static inline RuntimeValue val_float(double v) {
    return (RuntimeValue){ .type = VAL_FLOAT, .float_val = v };
}

static inline RuntimeValue val_void() {
    return (RuntimeValue){ .type = VAL_VOID };
}

RuntimeValue val_str(const char *s);
void val_free(RuntimeValue *v);
RuntimeValue val_clone(RuntimeValue v);

typedef struct EnvEntry EnvEntry;
typedef struct Env Env;

struct EnvEntry {
    char *name;
    EntryType type;
    union {
        RuntimeValue value;
        RuntimeFunc func;
    };
    EnvEntry *next;
};

struct Env {
    EnvEntry *head;
    Env *parent;
};

Env *env_new(Env *parent);
void env_free(Env *env);
void env_set_var(Env *env, const char *name, RuntimeValue value);
void env_set_func(Env *env, const char *name, RuntimeFunc func);
bool env_get_var(Env *env, const char *name, RuntimeValue *out);
RuntimeFunc env_get_func(Env *env, const char *name);

void entry_free(EnvEntry *entry);

typedef struct {
    Env          *env;
} EvalCtx;

EvalCtx *ctx_new(void);
void ctx_free(EvalCtx *ctx);

RuntimeValue eval(EvalCtx *ctx, ASTNode *node);
RuntimeValue eval_arith(char op, RuntimeValue left, RuntimeValue right);

RuntimeValue builtin_echo(RuntimeValue *args, size_t argc);

#endif