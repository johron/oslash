#include "lexer.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

const char* get_token_type_string(TokenType type) {
    switch (type) {
        case TOK_STR: return "TOK_STR";
        case TOK_NUM: return "TOK_NUM";
        case TOK_PLUS: return "TOK_PLUS";
        case TOK_MINUS: return "TOK_MINUS";
        case TOK_STAR: return "TOK_STAR";
        case TOK_SLASH: return "TOK_SLASH";
        case TOK_PERCENT: return "TOK_PERCENT";
        case TOK_EQUAL: return "TOK_EQUAL";
        case TOK_LESS: return "TOK_LESS";
        case TOK_MORE: return "TOK_MORE";
        case TOK_BANG: return "TOK_BANG";
        case TOK_COMMA: return "TOK_COMMA";
        case TOK_PIPE: return "TOK_PIPE";
        case TOK_DOLLAR: return "TOK_DOLLAR";
        case TOK_ARROW: return "TOK_ARROW";
        case TOK_DB_MORE: return "TOK_DB_MORE";
        case TOK_DB_EQUAL: return "TOK_DB_EQUAL";
        case TOK_BANG_EQUAL: return "TOK_BANG_EQUAL";
        case TOK_LESS_EQUAL: return "TOK_LESS_EQUAL";
        case TOK_MORE_EQUAL: return "TOK_MORE_EQUAL";
        case TOK_LPAREN: return "TOK_LPAREN";
        case TOK_RPAREN: return "TOK_RPAREN";
        case TOK_LBRACE: return "TOK_LBRACE";
        case TOK_RBRACE: return "TOK_RBRACE";
        case TOK_SEMICOLON: return "TOK_SEMICOLON";
        case TOK_EOF: return "TOK_EOF";
        default: return "Unknown TokenType";
    }
}

char lexer_peek(Lexer *l) {
    return l->src[l->pos];
}

char lexer_advance(Lexer *l) {
    return l->src[l->pos++];
}

void lexer_skip_ws(Lexer *l) {
    while (isspace(lexer_peek(l))) lexer_advance(l);
}

Token lexer_next_token(Lexer *l) {
    lexer_skip_ws(l);

    char c = lexer_peek(l);

    if (isdigit(c)) {
        int num = 0;

        while (isdigit(lexer_peek(l))) {
            num = num * 10 + (lexer_advance(l) - '0');
        }

        return (Token){
            .type = TOK_NUM,
            .value = {
                .num_value = num,
            }
        };
    }


    if (isalpha((unsigned char)c)) {
        size_t cap = 128;
        char *str = malloc(cap);
        size_t i = 0;

        while ((c = lexer_peek(l)), isalnum((unsigned char)c)) {
            if (i >= cap) {
                cap *= 2;
                char *next_str = realloc(str, cap);
                if (next_str == NULL) {
                    free(str);
                    fprintf(stderr, "Error: out of memory");
                    exit(1);                    
                }
                str = next_str;
            }
            
            lexer_advance(l);
            str[i++] = c;
        }

        str[i] = '\0';

        return (Token){
            .type = TOK_STR,
            .value = {
                .str_value = str,
            }
        };
    }

    if (c == '"') {
        lexer_advance(l);
        size_t cap = 128;
        char *str = malloc(cap);
        size_t i = 0;

        while (lexer_peek(l) != '"' && lexer_peek(l) != '\0') {
            c = lexer_advance(l);
            if (i >= cap - 1) {
                cap *= 2;
                char *next_str = realloc(str, cap);
                if (next_str == NULL) {
                    free(str);
                    fprintf(stderr, "Error: out of memory\n");
                    exit(1);                    
                }
                str = next_str;
            }
            
            str[i++] = c;
        }

        str[i] = '\0';

        if (lexer_peek(l) == '"') {
            lexer_advance(l);
        } else {
            free(str);
            fprintf(stderr, "Error: missing '\"' to close string\n");
            exit(1);
        }

        return (Token){
            .type = TOK_STR,
            .value = {
                .str_value = str,
            }
        };
    }

    lexer_advance(l);

    switch (c) {
        case '+': return (Token) { .type = TOK_PLUS };
        case '*': return (Token) { .type = TOK_STAR };
        case '/': return (Token) { .type = TOK_SLASH };
        case '%': return (Token) { .type = TOK_PERCENT };

        case '-': {
            if (lexer_peek(l) == '>') {
                lexer_advance(l);
                return (Token) { .type = TOK_ARROW };
            } else {
                return (Token) { .type = TOK_MINUS};
            }
        };
        case '=': {
            if (lexer_peek(l) == '=') {
                lexer_advance(l);
                return (Token) {.type = TOK_DB_EQUAL };
            } else {
                return (Token) { .type = TOK_EQUAL };
            }
        };
        case '<': {
            if (lexer_peek(l) == '=') {
                lexer_advance(l);
                return (Token) {.type = TOK_LESS_EQUAL };
            } else {
                return (Token) { .type = TOK_LESS };
            }
        };
        case '>': {
            if (lexer_peek(l) == '=') {
                lexer_advance(l);
                return (Token) { .type = TOK_MORE_EQUAL };
            } else if (lexer_peek(l) == '>') {
                lexer_advance(l);
                return (Token) { .type = TOK_DB_MORE };
            } else {
                return (Token) { .type = TOK_MORE };
            }
        };
        case '!': {
            if (lexer_peek(l) == '=') {
                lexer_advance(l);
                return (Token) {.type = TOK_BANG_EQUAL };
            } else {
                return (Token) { .type = TOK_BANG };
            }
        };
        
        case ',': return (Token) { .type = TOK_COMMA };
        case '|': return (Token) { .type = TOK_PIPE };
        case '$': return (Token) { .type = TOK_DOLLAR };
 
        case '(': return (Token) { .type = TOK_LPAREN };
        case ')': return (Token) { .type = TOK_RPAREN };
        case '{': return (Token) { .type = TOK_LBRACE };
        case '}': return (Token) { .type = TOK_RBRACE };

        case ';': return (Token){ .type = TOK_SEMICOLON };
        case '\0': return (Token){ .type = TOK_EOF };

        default:
            printf("Unknown character: '%c'\n", c);
            exit(1);
    }
}

void init_tok_array(TokenArray *a) {
    a->size = 0;
    a->cap = 32;
    a->data = malloc(a->cap * sizeof(Token));

    if (a->data == NULL) {
        fprintf(stderr, "Could not allocate memory for token array data");
        exit(1);
    }
}

void push_token(TokenArray *a, Token t) {
    if (a->size >= a->cap) {
        a->cap *= 2;
        a->data = realloc(a->data, a->cap * sizeof(Token));
    }

    a->data[a->size++] = t;
}

TokenArray lex_all(Lexer *l) {
    TokenArray arr;
    init_tok_array(&arr);

    while (true) {
        Token t = lexer_next_token(l);
        push_token(&arr, t);

        if (t.type == TOK_EOF)
            break;
    }

    return arr;
}

void free_tok_array(TokenArray *a) {
    if (a == NULL) return;

    for (size_t i = 0; i < a->size; i++) {
        switch (a->data[i].type) {
            case TOK_STR: {
                if (a->data[i].value.str_value != NULL) {
                    free(a->data[i].value.str_value);
                    a->data[i].value.str_value = NULL;
                }
                break;
            };
            default: {};
            // case TOK_NUM: {}; // This tok has data, but not in heap
        }
    }

    free(a->data);
    a->data = NULL;

    a->size = 0;
    a->cap = 0;
}