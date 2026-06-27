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

char** create_argv(char* cmd, RuntimeValue *args, size_t argc) {
    char **argv = malloc((argc + 2) * sizeof(char*));
    if (argv == NULL) return NULL;

    argv[0] = strdup(cmd); 
    if (argv[0] == NULL) {
        free(argv);
        return NULL;
    }

    for (size_t i = 0; i < argc; i++) {
        size_t argv_idx = i + 1; 

        switch (args[i].type) {
            case VAL_NUM: {
                argv[argv_idx] = malloc(64);
                if (argv[argv_idx]) sprintf(argv[argv_idx], "%d", args[i].num_val);
                break;
            };
            case VAL_FLOAT: {
                argv[argv_idx] = malloc(64);
                if (argv[argv_idx]) sprintf(argv[argv_idx], "%g", args[i].float_val);
                break;
            };
            case VAL_STR: {
                argv[argv_idx] = strdup(args[i].str_val);
                break;
            };
            case VAL_VOID: {
                argv[argv_idx] = strdup("void"); 
                break;
            };
            default: {
                argv[argv_idx] = strdup("unknown");
                break;
            };
        }

        if (argv[argv_idx] == NULL) {
            for (size_t j = 0; j < argv_idx; j++) free(argv[j]);
            free(argv);
            return NULL;
        }
    }

    argv[argc + 1] = NULL;
    return argv;
}

void free_argv(char **argv, size_t argc) {
    if (argv == NULL) return;

    for (size_t i = 0; i < argc + 1; i++) {
        free(argv[i]);
    }
    free(argv);
}

extern char **environ;

bool spawn_program(char* cmd, char* path, int *exit_code, RuntimeValue *args, size_t argc) {
    int result = is_regular_file(path);
    if (result != 1) {
        if (result == 0) {
            fprintf(stderr, "oar: '%s': is directory\n", path);
        } else {
            fprintf(stderr, "oar: '%s': does not exist\n", path);
        }

        return false;
    }

    char** argv = create_argv(cmd, args, argc);
    if (argv == NULL) {
        fprintf(stderr, "oar: '%s': memory allocation failed\n", cmd);
        return false;
    }

    pid_t pid;
    int status = posix_spawn(&pid, path, NULL, NULL, argv, environ);

    if (status == 0) {
        int wait_status;
        waitpid(pid, &wait_status, 0);
        if (WIFEXITED(wait_status)) {
            *exit_code = WEXITSTATUS(wait_status);
        }
    } else {
        *exit_code = 1;
        fprintf(stderr, "oar: '%s': %s\n", cmd, strerror(status));
    }

    free_argv(argv, argc);
    return true;
}

int resolve_from_env_path(const char *cmd, char *resolved_path) {
    char *path_env = getenv("PATH");
    if (!path_env || strlen(path_env) == 0) {
        return 0;
    }

    char *path_copy = strdup(path_env);
    if (!path_copy) {
        return 0; 
    }

    int found = 0;
    char *dir = strtok(path_copy, ":");

    while (dir != NULL) {
        char candidate[PATH_MAX];
        
        const char *search_dir = (strlen(dir) == 0) ? "." : dir;

        int len = snprintf(candidate, sizeof(candidate), "%s/%s", search_dir, cmd);
        
        if (len > 0 && len < (int)sizeof(candidate)) {
            if (access(candidate, X_OK) == 0) {
                strncpy(resolved_path, candidate, PATH_MAX - 1);
                resolved_path[PATH_MAX - 1] = '\0';
                found = 1;
                break; 
            }
        }

        dir = strtok(NULL, ":");
    }

    free(path_copy);
    return found;
}

bool try_run_func_external(Env *env, const char *cmd, RuntimeValue *args, size_t argc) {
    char path[PATH_MAX];

    if (startsWith("/", cmd) == true) { // absolute
        strcpy(path, cmd);

        int exit_code;
        if (spawn_program(cmd, path, &exit_code, args, argc) != 0) { // TODO: maybe prettify name in spawn program, so /bin/.../program becomes program or something..
            return false;
        }

        printf("Exited with code %d\n", exit_code);

        return true;
    } else if (startsWith("./", cmd) == true) { // relative
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            if (strlen(cmd) > 0) {
                char* name[strlen(cmd)];
                strcpy(name, cmd);

                memmove(cmd, cmd + 2, strlen(cmd));
                snprintf(path, sizeof(path), "%s/%s", cwd, cmd);
                
                int exit_code;
                if (spawn_program(name, path, &exit_code, args, argc) != 0) {
                    return false;
                }

                printf("Exited with code %d\n", exit_code);

                return true;
            } else {
                fprintf(stderr, "oar: runtime: string is too short, I don't think this should happen\n");
                return false;
            }
        } else {
            fprintf(stderr, "oar: runtime: could not get current working directory\n");
            return false;
        }
    } else { // not a path, resolve using PATH
        if (resolve_from_env_path(cmd, path)) {
            int exit_code;
            if (spawn_program(cmd, path, &exit_code, args, argc) != 0) {
                return false;
            }

            printf("Exited with code %d\n", exit_code);
            return true;
        }
        
        fprintf(stderr, "oar: '%s': unknown function\n", cmd);
        return false;
    }
}

int main() {
    EvalCtx *ctx = ctx_new(try_run_func_external);

    repl(ctx);
    
    ctx_free(ctx);
}