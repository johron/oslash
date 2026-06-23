#include "lexer.h"
#include "parser.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

void parser_init_block(Block *block) {
    block->size = 0;
    block->cap = 32;
    block->nodes = malloc(block->cap * sizeof(ASTNode));

    if (block->nodes == NULL) {
        fprintf(stderr, "Could not allocate memory for nodes in block");
        exit(1);
    }
}

void parser_block_push_node(Block *block, ASTNode *node) {
    if (block->size >= block->cap) {
        block->cap *= 2;
        block->nodes = realloc(block->nodes, block->cap * sizeof(ASTNode));
    }

    block->nodes[block->size++] = *node;
}

ASTNode* parse_block(Parser *p) {
    Block block;
    parser_init_block(&block);

    while (p->pos < p->tok_array.size && parser_peek(p).type != TOK_EOF && parser_peek(p).type != TOK_RBRACE) {
        ASTNode *node = parse_stmt(p);
        if (node != NULL) {
            parser_block_push_node(&block, node);
        }
    }

    ASTNode *ast = malloc(sizeof(ASTNode));
    ast->type = NODE_BLOCK;
    ast->data.block = block;
    return ast;
}



Token parser_peek(Parser *p) {
    return p->tok_array.data[p->pos];
}

Token parser_advance(Parser *p) {
    if (parser_peek(p).type != TOK_EOF) {
        p->pos++;
    }

    return p->tok_array.data[p->pos - 1];
}

Token parser_expect(Parser *p, TokenType type, const char *message) {
    if (parser_peek(p).type == type) {
        return parser_advance(p);
    }
    fprintf(stderr, "Parser Error: %s, got %s\n", message, get_token_type_string(parser_peek(p).type));
    exit(1);
}

void parser_free_ast(ASTNode *node) {
    if (!node) return;

    if (node->type == NODE_BINARY_OP) {
        parser_free_ast(node->data.binary_op.left);
        parser_free_ast(node->data.binary_op.right);
    }
    free(node);
}

ASTNode* parser_create_member_node(NodeType type, TokenValue value) {
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node) {
        return NULL;
    }

    node->type = type;
    node->data.value = value;

    return node;
}

ASTNode* parser_create_binary_op_node(char op, ASTNode *left, ASTNode *right) {
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node) {
        return NULL;
    }

    node->type = NODE_BINARY_OP;
    node->data.binary_op.op = op;
    node->data.binary_op.left = left;
    node->data.binary_op.right = right;

    return node;
}

ASTNode* parse_stmt(Parser *p);

ASTNode* parse_stmt(Parser *p) {
    switch (p->tok_array.data[p->pos].type) {
        case TOK_STR: {
            Token tok = parser_peek(p);

            if (strcmp(tok.value.str_value, "fn") == false) { // fn x(...)
                return parse_func_decl_stmt(p);
            } else if (strcmp(tok.value.str_value, "let") == false) { // let $x = ... (;)
                
            } else { // treat as function call 
                return parse_func_call_stmt(p);
            }

            printf("parse_stmt() is unimplemented for TOK_STR\n");
            exit(1);
        };
        case TOK_DOLLAR: { // Variable reference
            printf("parse_stmt() is unimplemented for TOK_DOLLAR\n");
            exit(1);
        };
        case TOK_LPAREN: { // statement enclosure
            parser_advance(p);
            ASTNode *stmt = parse_stmt(p);
            parser_expect(p, TOK_RPAREN, "Expected parenthesis to close statement enclosure");
            return stmt;
        };
        default: { // TODO: add some expression statement stuff somewhere here
            fprintf(stderr, "Unexpected tokens found while parsing statement, got %s\n", get_token_type_string(p->tok_array.data[p->pos].type));
            exit(1);
        };
    }
}

ASTNode* parse_func_call_stmt(Parser *p) {
    ASTNode *node = malloc(sizeof(ASTNode));

    Token func_tok = parser_expect(p, TOK_STR, "Expected the string/identifier to call");

    Block args;
    parser_init_block(&args);

    if (parser_peek(p).type == TOK_LPAREN) { // parse as func(arg1, arg2, ...)
        parser_advance(p);
        while (parser_peek(p).type != TOK_RPAREN) {
            ASTNode *expr = parse_expr(p);

            printf("%s\n", get_token_type_string(parser_peek(p).type));
            if (parser_peek(p).type != TOK_RPAREN) {
                parser_expect(p, TOK_COMMA, "Expected comma to continue argument list");
            }

            parser_block_push_node(&args, expr);
        }

        parser_expect(p, TOK_RPAREN, "Expected closing parenthesis to close function call");
    } else { // parse as func arg1 arg2 ...;
        

        printf("parse_func_call_stmt() is unimplemented for function call type 2\n");
        exit(1);
    }

    FuncCall func_call = {
        .args = &args,
        .name = func_tok.value.str_value,
    };

    node->type = NODE_FUNC_CALL_STMT;
    node->data.func_call = func_call;

    return node;
}

ASTNode* parse_func_decl_stmt(Parser *p) {
    ASTNode *node = malloc(sizeof(ASTNode));

    parser_expect(p, TOK_STR, "Expected fn keyword to declare function");
    Token name_tok = parser_expect(p, TOK_STR, "Expected a string/identifier for function name");
    parser_expect(p, TOK_LPAREN, "Expected parenthesis to open argument definition for function declaration");
    // parse argument definition
    parser_expect(p, TOK_RPAREN, "Expected parenthesis to close argument definition for function declaration");

    parser_expect(p, TOK_ARROW, "Expected an arrow ('->') to specify return type for function declaration");
    Token ret_type_tok = parser_expect(p, TOK_STR, "Expected a string/identifier for function return type");

    parser_expect(p, TOK_LBRACE, "Expected left brace to open function body");
    ASTNode *body = parse_block(p);
    parser_expect(p, TOK_RBRACE, "Expected left brace to close function body");

    FuncDecl func_decl = {
        .name = name_tok.value.str_value,
        .ret_type = ret_type_tok.value.str_value,
        .body = body,
    };

    node->type = NODE_FUNC_DECL_STMT;
    node->data.func_decl = func_decl;

    return node;
}

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
    ASTNode *expr = parse_primary_expr(p);

    while (parser_peek(p).type == TOK_STAR || parser_peek(p).type == TOK_SLASH) {
        Token op_token = parser_peek(p);
        parser_advance(p);
        
        char op = (op_token.type == TOK_STAR) ? '*' : '/';
        
        ASTNode *right = parse_primary_expr(p);
        expr = parser_create_binary_op_node(op, expr, right);
    }

    return expr;
}


ASTNode* parse_expr(Parser *p) {
    bool enclosed = false;
    if (parser_peek(p).type == TOK_LPAREN) {
        parser_advance(p);
        enclosed = true;
    }

    ASTNode *expr = parse_term_expr(p);

    while (parser_peek(p).type == TOK_PLUS || parser_peek(p).type == TOK_MINUS) {
        Token op_token = parser_peek(p);
        parser_advance(p);
        
        char op = (op_token.type == TOK_PLUS) ? '+' : '-';
        
        ASTNode *right = parse_term_expr(p);
        expr = parser_create_binary_op_node(op, expr, right);
    }

    if (enclosed == true) {
        parser_expect(p, TOK_RPAREN, "Expected closing parenthesis to close expression enclosure");
    }

    return expr;
}
