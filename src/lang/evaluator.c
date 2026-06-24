#include "lexer.h"
#include "parser.h"
#include "evaluator.h"

#include <stdio.h>
#include <string.h>

RuntimeValue val_str(const char *s) {
    RuntimeValue v = { .type = VAL_STR };
    v.str_val = strdup(s);
    return v;
}

void val_free(RuntimeValue *v) {
    if (v && v->type == VAL_STR && v->str_val) {
        free(v->str_val);
        v->str_val = NULL;
    }
}

RuntimeValue val_clone(RuntimeValue v) {
    if (v.type == VAL_STR && v.str_val)
        return val_str(v.str_val);
    return v;
}

void entry_free(EnvEntry *entry) {
    if (entry->type == ENTRY_VAR) {
        if (entry->value.type == VAL_STR && entry->value.str_val) {
            free(entry->value.str_val);
            entry->value.str_val = NULL;
        }
    // } else if (entry->type == ENTRY_FUNC) { // not needed
    }
}

Env *env_new(Env *parent) {
    Env *e = malloc(sizeof(Env));
    e->head   = NULL;
    e->parent = parent;
    return e;
}

void env_free(Env *env) {
    EnvEntry *e = env->head;
    while (e) {
        EnvEntry *next = e->next;
        free(e->name);
        entry_free(e);
        free(e);
        e = next;
    }
    free(env);
}

void env_set_var(Env *env, const char *name, RuntimeValue value) {
    for (Env *scope = env; scope; scope = scope->parent) {
        for (EnvEntry *e = scope->head; e; e = e->next) {
            if (strcmp(e->name, name) == false) {
                if (e->type != ENTRY_VAR) {
                    fprintf(stderr, "Cannot redefine %s '%s' as variable\n", get_entry_type_string_pretty(e->type), name);
                    exit(1);
                }

                val_free(&e->value); // TODO: implement val_free
                e->value = val_clone(value);
                return;
            }
        }
    }

    EnvEntry *entry = malloc(sizeof(EnvEntry));
    entry->name = strdup(name);
    entry->type = ENTRY_VAR;
    entry->value = val_clone(value);
    entry->next = env->head;
    env->head = entry;
}

void env_set_func(Env *env, const char *name, RuntimeFunc func) {
    for (Env *scope = env; scope; scope = scope->parent) {
        for (EnvEntry *e = scope->head; e; e = e->next) {
            if (strcmp(e->name, name) == false) {
                if (e-> type != ENTRY_FUNC) {
                    fprintf(stderr, "Cannot redefine already defined %s '%s'\n", get_entry_type_string_pretty(e->type), name);
                    exit(1);
                }

                fprintf(stderr, "Cannot redefine already defined function '%s\n", name);
                exit(1);
            }
        }
    }

    EnvEntry *entry = malloc(sizeof(EnvEntry));
    entry->name = strdup(name);
    entry->type = ENTRY_FUNC;
    entry->func = func;
    entry->next = env->head;
    env->head = entry;
}

bool env_get_var(Env *env, const char *name, RuntimeValue *out) {
    for (Env *scope = env; scope; scope = scope->parent) {
        for (EnvEntry *e = scope->head; e; e = e->next) {
            if (strcmp(e->name, name) == false) {
                if (e->type != ENTRY_VAR) {
                    fprintf(stderr, "Tried to reference variable as %s\n", get_entry_type_string_pretty(e->type));
                    exit(1);
                }

                *out = val_clone(e->value);
                return true;
            }
        }
    }

    return false;
}

RuntimeFunc env_get_func(Env *env, const char *name) {
    for (Env *scope = env; scope; scope = scope->parent) {
        for (EnvEntry *e = scope->head; e; e = e->next) {
            if (strcmp(e->name, name) == false) {
                if (e->type != ENTRY_FUNC) {
                    fprintf(stderr, "Tried to reference function as %s\n", get_entry_type_string_pretty(e->type));
                    exit(1);
                }

                return e->func;
            }
        }
    }

    return NULL;
}

EvalCtx *ctx_new(void) {
    EvalCtx *ctx  = malloc(sizeof(EvalCtx));
    ctx->env      = env_new(NULL);

    env_set_func(ctx->env, "echo", builtin_echo);

    return ctx;
}

void ctx_free(EvalCtx *ctx) {
    env_free(ctx->env);
    free(ctx);
}

RuntimeValue eval_arith(char op, RuntimeValue left, RuntimeValue right) {
    if (left.type == VAL_FLOAT || right.type == VAL_FLOAT) {
        double l = (left.type == VAL_FLOAT) ? left.float_val : (double) left.num_val;
        double r = (right.type == VAL_FLOAT) ? right.float_val : (double) right.num_val;

        switch (op) {
            case '+': return val_float(l + r);
            case '-': return val_float(l - r);
            case '*': return val_float(l * r);
            case '/': {
                if (r == 0.0) {
                    fprintf(stderr, "Runtime error: division by zero\n");
                    exit(1);
                }
                return val_float(l / r);
            };
        }
    }

    long int l = left.num_val;
    long int r = right.num_val;
    
    switch (op) {
        case '+': return val_num(l + r);
        case '-': return val_num(l - r);
        case '*': return val_num(l * r);
        case '/': {
            if (r == 0) {
                fprintf(stderr, "Runtime error: division by zero\n");
                exit(1);
            }
            
            if (l % r == 0) {
                return val_num(l / r);
            } else {
                return val_float((double) l / (double) r);
            }
        };
    }

    fprintf(stderr, "Runtime error: unknown arithmetic operator '%c'\n", op);
    exit(1);
}

RuntimeValue eval(EvalCtx *ctx, ASTNode *node) {
    if (!node) return val_num(0);

    switch (node->type) {
        case NODE_VALUE_NUMBER: return val_num(node->data.value.num_value);
        case NODE_VALUE_FLOAT: return val_float(node->data.value.float_value);
        case NODE_VALUE_STRING: return val_str(node->data.value.str_value);

        case NODE_VALUE_VAR_REF: {
            RuntimeValue v;
            if (!env_get_var(ctx->env, node->data.value.str_value, &v)) {
                fprintf(stderr, "Runtime error: undefined variable '$%s'\n", node->data.value.str_value);
                exit(1);
            }
            return v;
        };

        case NODE_BINARY_OP: {
            RuntimeValue left = eval(ctx, node->data.binary_op.left);
            RuntimeValue right = eval(ctx, node->data.binary_op.right);
            char op = node->data.binary_op.op;

            RuntimeValue result;
            if (op == '+' && (left.type == VAL_STR || right.type == VAL_STR)) {
                char lbuf[64], rbuf[64];
                const char *left_side = (left.type == VAL_STR) ? left.str_val : (snprintf(lbuf, sizeof lbuf,
                    left.type == VAL_FLOAT ? "%g" : "%f",
                    left.type == VAL_FLOAT ? left.float_val : left.num_val), lbuf);
                const char *right_side = (right.type == VAL_STR) ? right.str_val : (snprintf(rbuf, sizeof rbuf,
                    right.type == VAL_FLOAT ? "%g" : "%f",
                    right.type == VAL_FLOAT ? right.float_val : right.num_val), rbuf);
                
                size_t len = strlen(left_side) + strlen(right_side) + 1;
                char *buf = malloc(len);
                snprintf(buf, len, "%s%s", left_side, right_side);
                result = (RuntimeValue) {
                    .type = VAL_STR,
                    .str_val = buf,
                };
            } else {
                result = eval_arith(op, left, right);
            }

            val_free(&left);
            val_free(&right);
            return result;
        };

        case NODE_UNARY_OP: {
            RuntimeValue v = eval(ctx, node->data.unary_op.operand);
            char op = node->data.unary_op.op;
            if (op == '-') {
                if (v.type == VAL_FLOAT) return val_float(-v.float_val);
                return val_num(-v.num_val);
            }
            if (op == '+') return v;
            if (op == '!') return val_num(!v.num_val);
            fprintf(stderr, "Runtime error: unknown unary op '%c'\n", op);
            exit(1);
        };

        case NODE_VAR_DECL_STMT: {
            RuntimeValue v = eval(ctx, node->data.var_decl.expr);
            env_set_var(ctx->env, node->data.var_decl.name, v);
            val_free(&v);
            return val_void();
        };

        case NODE_FUNC_CALL_STMT: {
            const char *name = node->data.func_call.name;
            Block *args_block = &node->data.func_call.args;

            size_t argc = args_block ? args_block->size : 0;
            RuntimeValue *args = argc ? malloc(argc * sizeof(RuntimeValue)) : NULL;
            for (size_t i = 0; i < argc; i++)
                args[i] = eval(ctx, &args_block->nodes[i]);

            RuntimeValue result = val_void();

            RuntimeFunc func = env_get_func(ctx->env, name);
            if (func != NULL) {
                result = func(args, argc);
            } else {
                fprintf(stderr, "Runtime error: unknown function '%s'\n", name);
                exit(1);
            }

            for (size_t i = 0; i < argc; i++) val_free(&args[i]);
            free(args);
            return result;
        };

        case NODE_FUNC_DECL_STMT: {
            fprintf(stderr, "func decl unimplemented");
            exit(1);
        };

        case NODE_BLOCK: {
            RuntimeValue last = val_void();
            Env *child = env_new(ctx->env);
            Env *saved = ctx->env;
            ctx->env   = child;

            for (size_t i = 0; i < node->data.block.size; i++) {
                val_free(&last);
                last = eval(ctx, &node->data.block.nodes[i]);
            }

            ctx->env = saved;
            env_free(child);
            return last;
        }

        default: {
            fprintf(stderr, "Runtime error: unhandled node type %s\n", get_node_type_string(node->type));
            exit(1);
        };
    }
}

RuntimeValue builtin_echo(RuntimeValue *args, size_t argc) {
    for (size_t i = 0; i < argc; i++) {
        if (i > 0) printf(" ");
        
        RuntimeValue v = args[i];
        switch (v.type) {
            case VAL_NUM:   printf("%d",  v.num_val);   break;
            case VAL_FLOAT: printf("%g",  v.float_val); break;
            case VAL_STR:   printf("%s",  v.str_val);   break;
            case VAL_VOID:  printf("void"); break;
        }
    }
    printf("\n");
    return val_void();
}