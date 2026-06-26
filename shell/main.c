#define OAR_LANG_IMPLEMENTATION
#define OAR_USE_EXTERNAL_FUNCTION_SOURCE

#include "../lang/oar.h"

#include <stdio.h>
#include <string.h>

bool exec_input(EvalCtx *ctx, char* input, Error *err) {
    Lexer lexer = {
        .src = input,
        .pos = 0,
    };
    TokenArray tok_array;
    Error tok_arr_err = {0};
    
    if (lex_all(&lexer, &tok_array, &tok_arr_err) == false) {
        *err = tok_arr_err;
        return false;
    } 

    Parser parser = {
        .tok_array = tok_array,
        .pos = 0,
    };

    size_t size = 0;
    size_t cap = 32;
    ASTNode *nodes = malloc(cap * sizeof(ASTNode));

    while (parser.pos < parser.tok_array.size && parser_peek(&parser).type != TOK_EOF && parser_peek(&parser).type != TOK_RBRACE) {
        ASTNode node;
        if (parse_stmt(&parser, &node, err) == false) {
            return false;
        }

        if (&node != NULL) {
            if (size >= cap) {
                cap *= 2;
                nodes = realloc(nodes, cap * sizeof(ASTNode));
            }

            nodes[size++] = node;
        }
    }

    for (size_t i = 0; i < size; i++) {
        RuntimeValue v;
        Error v_err = {0};

        if (eval(ctx, &nodes[i], &v, &v_err) == false) {
            *err = v_err;
            return false;
        }

        parser_free_ast(&nodes[i]);
    }

    free_tok_array(&tok_array);

    return true;
}

void repl(EvalCtx *ctx) {
    while (true) {
        char buffer[8192];

        printf("$ ");
        if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
            buffer[strcspn(buffer, "\n")] = '\0';

            Error err = {0};
            if (exec_input(ctx, buffer, &err) == false) {
                fprintf(stderr, "oar: %s: %s\n", err.type, err.message);
                free(err.message);
            }
        }
    }
}

RuntimeFunc env_get_func_external(Env *env, const char *name) {
    // Try to spawn `name` as program from path...

    // Plan
    // * First check if the name starts with / or ./ (also need to fix the lexer to support these as strings), also need to make lexer make basically anything into a string
    //     * If a path is supplied, then check if it exists then run that
    // * else path resolver, checks the path for `name` and runs that
    // * else error 

    return NULL;
}

int main() {
    EvalCtx *ctx = ctx_new(env_get_func_external);

    repl(ctx);
    
    ctx_free(ctx);
}