#ifndef PARSER_EXPRESSION_H
#define PARSER_EXPRESSION_H

#include "../parser.h"

ASTNode* parse_expr(Parser *p);
ASTNode* parse_term_expr(Parser *p);
ASTNode* parse_primary_expr(Parser *p);

#endif