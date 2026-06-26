#define OAR_LANG_IMPLEMENTATION
#define OAR_USE_EXTERNAL_FUNCTION_SOURCE

#include "../lang/oar.h"

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <spawn.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

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
                if (&err != NULL && err.message != NULL && err.type != NULL) {
                    fprintf(stderr, "oar: %s: %s\n", err.type, err.message);
                }
                free(err.message);
            }
        }
    }
}

// Source - https://stackoverflow.com/a/4771038
// Posted by T.J. Crowder, modified by community. See post 'Timeline' for change history
// Retrieved 2026-06-26, License - CC BY-SA 4.0
bool startsWith(const char *pre, const char *str){
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? false : memcmp(pre, str, lenpre) == 0;
}

int is_regular_file(const char *path) {
    struct stat path_stat;
    
    if (stat(path, &path_stat) != 0) {
        return -1; 
    }
    
    return S_ISREG(path_stat.st_mode);
}

extern char **environ;

int spawn_program(char* name, char* path, RuntimeValue *args, size_t argc) {
    printf("%s, %s\n", name, path);
    // is it a file?, if so then spawn

    // convert runtime value args to strings

    int result = is_regular_file(path);
    if (result == 1) {
        printf("it's a file\n");

        pid_t pid;
        char *argv[] = {name /*, stringed_args*/, NULL};

        int status = posix_spawn(&pid, path, NULL, NULL, argv, environ);

        if (status == 0) {
            printf("Spawned process with pid '%d'\n", pid);
            waitpid(pid, &status, 0);
            return 0;
        } else {
            //perror(sprintf("oar: %s", name));
            fprintf(stderr, "oar: %s: %s\n", name, strerror(errno));
        }

        return 1; // return error code of spawned process?
    } else if (result == 0) {
        fprintf(stderr, "oar: runtime: '%s' is directory\n", path);
        return 1;
    } else {
        fprintf(stderr, "oar: runtime: '%s' does not exist\n", path);
        return 1;
    }
}

bool try_run_func_external(Env *env, const char *name, RuntimeValue *args, size_t argc) {
    // Try to spawn `name` as program from path...

    char path[PATH_MAX];

    if (startsWith("/", name) == true) { // absolute
        strcpy(path, name);
        printf("path: '%s'\n", path);
        if (spawn_program(name, path, args, argc) != 0) { // TODO: maybe prettify name in spawn program, so /bin/.../program becomes program or something..
            return false;
        }

        return true;
    } else if (startsWith("./", name) == true) { // relative
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            if (strlen(name) > 0) {
                char* old_name[strlen(name)];
                strcpy(old_name, name);

                memmove(name, name + 2, strlen(name));
                snprintf(path, sizeof(path), "%s/%s", cwd, name);
                
                printf("path: '%s'\n", path);
                if (spawn_program(old_name, path, args, argc) != 0) {
                    return false;
                }

                return true;
            } else {
                fprintf(stderr, "oar: runtime: string is too short, I don't think this should happen\n");
                return false;
            }
        } else {
            fprintf(stderr, "oar: runtime: could not get current working directory\n");
            return false;
        }
    } else { // not a path
            fprintf(stderr, "oar: runtime: non path env_get_func_external not implemented yet\n");
            return false;
    }

    // Plan
    // * First check if the name starts with / or ./ (also need to fix the lexer to support these as strings), also need to make lexer make basically anything into a string
    //     * If a path is supplied, then check if it exists then run that
    // * else path resolver, checks the path for `name` and runs that
    // * else error 

    fprintf(stderr, "oar: runtime: unknown function '%s'", name);
    return false;
}

int main() {
    EvalCtx *ctx = ctx_new(try_run_func_external);

    repl(ctx);
    
    ctx_free(ctx);
}