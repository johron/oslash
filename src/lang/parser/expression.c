#include "../parser.h"
#include "expression.h"

#include <stdio.h>
#include <stdbool.h>

ASTNode* parse_primary_expr(Parser *p) {
    Token token = parser_peek(p);

    switch (token.type) {
        case TOK_NUM: parser_advance(p); return parser_create_member_node(NODE_VALUE_NUMBER, token.value);
        case TOK_STR: parser_advance(p); return parser_create_member_node(NODE_VALUE_STRING, token.value);

        case TOK_LPAREN: {
            parser_advance(p);
            ASTNode *node = parse_expr(p);
            parser_expect(p, TOK_RPAREN, "Expected ')' after expression");
            return node;
        };

        default: {
            fprintf(stderr, "Parser Error: Unexpected token.\n");
            exit(1);
        }
    }
}

ASTNode* parse_term_expr(Parser *p) {

}

ASTNode* parse_expr(Parser *p) {

}

