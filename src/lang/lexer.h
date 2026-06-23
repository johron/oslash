#ifndef LEXER_H
#define LEXER_H

#include <stdlib.h>

typedef enum {
    TOK_STR,
    TOK_NUM,

    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_PERCENT,

    TOK_EQUAL,
    TOK_LESS,
    TOK_MORE,
    TOK_BANG,
    TOK_COMMA,
    TOK_PIPE,
    TOK_DOLLAR,
    TOK_ARROW,

    TOK_DB_MORE,
    TOK_DB_EQUAL,
    TOK_BANG_EQUAL,
    TOK_LESS_EQUAL,
    TOK_MORE_EQUAL,

    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACE,
    TOK_RBRACE,
    //TOK_LBRACKET,
    //TOK_RBRACKET,

    TOK_SEMICOLON,
    TOK_NEWLINE,
    TOK_EOF,
} TokenType;    

typedef union {
    int num_value;
    char* str_value;
} TokenValue;

typedef struct {
    TokenType type;
    TokenValue value;
} Token;

typedef struct {
    const char *src;
    size_t pos;
} Lexer;

typedef struct {
    Token *data;
    size_t size;
    size_t cap;
} TokenArray;

const char* get_token_type_string(TokenType type);

TokenArray lex_all(Lexer *l);
void free_tok_array(TokenArray *a);

#endif