#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

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

typedef struct {
    ASTNode *nodes;
    size_t size;
    size_t cap;
} Block;

typedef struct {
    char* name;
    // arguments
    char* ret_type;
    ASTNode *body; // NodeType = NODE_BLOCK
} FuncDecl;

typedef struct {
    char* name;
    Block *args;
} FuncCall;

struct ASTNode {
    NodeType type;
    union {
        Block block;
        FuncDecl func_decl;
        FuncCall func_call;
        struct {
            char op;
            ASTNode *left;
            ASTNode *right;
        } binary_op;
        TokenValue value;
    } data;
};

ASTNode* parse_stmt(Parser *p);

ASTNode* parse_func_decl_stmt(Parser *p);
ASTNode* parse_func_call_stmt(Parser *p);

ASTNode* parse_block(Parser *l);
ASTNode* parser_create_member_node(NodeType type, TokenValue value);
ASTNode* parser_create_binary_op_node(char op, ASTNode *left, ASTNode *right);

Token parser_peek(Parser *p);
Token parser_advance(Parser *p);
Token parser_expect(Parser *p, TokenType type, const char *message);

ASTNode* parse_expr(Parser *p);
ASTNode* parse_term_expr(Parser *p);
ASTNode* parse_primary_expr(Parser *p);


void parser_free_ast(ASTNode *node);

#endif