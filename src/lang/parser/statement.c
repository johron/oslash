#include "statement.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

ASTNode* parse_stmt(Parser *p) {
    switch (p->tok_array.data[p->pos].type) {
        case TOK_STR: {
            Token tok = parser_expect(p, TOK_STR, "Expected a string/identifier while parsing statement");

            if (strcmp(tok.value.str_value, "fn") == false) { // fn x(...)
                return parse_func_decl_stmt(p);
            } else if (strcmp(tok.value.str_value, "let") == false) { // let $x = ...

            } else { // treat as function call

            }

            printf("parse_stmt() is unimplemented for TOK_STR\n");
            exit(1);
        };
        case TOK_DOLLAR: { // Variable reference
            printf("parse_stmt() is unimplemented for TOK_DOLLAR\n");
            exit(1);
        };
        default: { // TODO: add some expression statement stuff somewhere here
            fprintf(stderr, "Unexpected tokens found while parsing statement, got %s\n", get_token_type_string(p->tok_array.data[p->pos].type));
            exit(1);
        };
    }
}

ASTNode* parse_func_decl_stmt(Parser *p) {
    ASTNode *node = malloc(sizeof(ASTNode));

    Token name_tok = parser_expect(p, TOK_STR, "Expected a string/identifier for function name");
    parser_expect(p, TOK_LPAREN, "Expected parenthesis to open argument definition for function declaration");
    // parse argument definition
    parser_expect(p, TOK_RPAREN, "Expected parenthesis to close argument definition for function declaration");

    parser_expect(p, TOK_ARROW, "Expected an arrow ('->') to specify return type for function declaration");
    Token ret_type_tok = parser_expect(p, TOK_STR, "Expected a string/identifier for function return type");

    // parse block

    FuncDecl func_decl = {
        .name = name_tok.value.str_value,
        .ret_type = ret_type_tok.value.str_value,
        //.block = 
    };

    node->type = NODE_FUNC_DECL_STMT;
    node->data.func_decl = func_decl;

    return node;
}