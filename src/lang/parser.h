#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "parser/statement.h"
#include "parser/expression.h"

#include <stdlib.h>

typedef struct {
    TokenArray tok_array;
    size_t pos;
} Parser;

typedef enum {
    NODE_BLOCK,
    NODE_BINARY_OP,

    NODE_VAR_DECL_STMT,
    NODE_FUNC_DECL_STMT,
    NODE_FUNC_CALL_STMT,

    NODE_VALUE_NUMBER,
    NODE_VALUE_STRING,
    NODE_VALUE_IDENT,
} NodeType;

typedef union {
    int number_value;
    char* str_value;
    char* ident_value;
} NodeValue;

typedef struct ASTNode ASTNode;

struct ASTNode {
    NodeType type;
    union {
        Block block;
        FuncDecl func_decl;
        struct {
            char op;
            ASTNode *left;
            ASTNode *right;
        } binary_op;
        TokenValue value;
    } data;
};

ASTNode* parse_all(Parser *l);
ASTNode* parser_create_member_node(NodeType type, TokenValue value);
ASTNode* parser_create_binary_node(char op, ASTNode *left, ASTNode *right);

Token parser_peek(Parser *p);
Token parser_advance(Parser *p);
Token parser_expect(Parser *p, TokenType type, const char *message);

void parser_free_ast(ASTNode *node);

#endif