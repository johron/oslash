#ifndef PARSER_STATEMENT_H
#define PARSER_STATEMENT_H

#include "../parser.h"

typedef struct {
    ASTNode *nodes;
    size_t size;
    size_t cap;
} Block;

typedef struct {
    char* name;
    // arguments
    char* ret_type;
    Block block;
} FuncDecl;

ASTNode* parse_stmt(Parser *p);

ASTNode* parse_func_decl_stmt(Parser *p);

#endif