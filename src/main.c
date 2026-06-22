#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

typedef enum {
    TOK_STR,
    TOK_NUM,
    TOK_KEYWORD,

    //TOK_PLUS,
    //TOK_MINUS,
    //TOK_STAR,
    //TOK_SLASH,

    TOK_TERM,
    TOK_EOF,
} TokenType;

typedef struct {
    TokenType type;
    int num_value;
    char* str_value;
} Token;

typedef struct {
    const char *src;
    size_t pos;
} Lexer;

char peek(Lexer *l) {
    return l->src[l->pos];
}

char advance(Lexer *l) {
    return l->src[l->pos++];
}

void skip_ws(Lexer *l) {
    while (isspace(peek(l)) && peek(l) != '\n') advance(l);
}

Token next_token(Lexer *l) {
    skip_ws(l);

    char c = peek(l);

    if (isdigit(c)) {
        int num = 0;

        while (isdigit(peek(l))) {
            num = num * 10 + (advance(l) - '0');
        }

        return (Token){
            .type = TOK_NUM,
            .num_value = num,
        };
    }


    if (isalpha((unsigned char)c)) {
        size_t cap = 128;
        char *str = malloc(cap);
        size_t i = 0;

        while ((c = advance(l)), isalnum((unsigned char)c)) {
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
            
            str[i++] = c;
        }

        str[i] = '\0';

        return (Token){
            .type = TOK_KEYWORD,
            .str_value = str,
        };
    }

    if (c == '"') {
        advance(l);
        size_t cap = 128;
        char *str = malloc(cap);
        size_t i = 0;

        while ((c = advance(l)), c != '"') {
            if (i >= cap) {
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

        if (c != '"') {
            printf("??\n");
            free(str);
            fprintf(stderr, "Error: missing '\"' to close string\n");
            exit(1);
        }

        return (Token){
            .type = TOK_STR,
            .str_value = str,
        };
    }

    advance(l);

    switch (c) {
        case ';': return (Token){TOK_TERM};
        case '\n': return (Token){TOK_TERM};
        case '\0': return (Token){TOK_EOF};
        default:
            printf("Unknown character: %c\n", c);
            exit(1);
    }
}

typedef struct {
    Token *data;
    size_t size;
    size_t cap;
} TokenArray;

void init_array(TokenArray *a) {
    a->size = 0;
    a->cap = 32;
    a->data = malloc(a->cap * sizeof(Token));
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
    init_array(&arr);

    while (true) {
        Token t = next_token(l);
        push_token(&arr, t);

        if (t.type == TOK_EOF)
            break;
    }

    return arr;
}

int main() {
    char input[] = "echo \"Hello, world!!\"";

    Lexer lex = {
        .src = input,
        .pos = 0,
    };
    TokenArray tok_array = lex_all(&lex);

    for (size_t i = 0; i < tok_array.size; i++) {
        printf("Token type: %d\n", tok_array.data[i].type);
        printf("    value: '%s'\n", tok_array.data[i].str_value);
        printf("    value: '%d'\n", tok_array.data[i].num_value);
    }
}