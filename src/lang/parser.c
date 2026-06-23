#include "lexer.h"
#include "parser.h"

#include "parser/statement.h"
#include "parser/expression.h"

#include <stdio.h>

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

ASTNode* parse_all(Parser *p) {
    Block block;
    parser_init_block(&block);

    while (p->pos < p->tok_array.size) {
        ASTNode *node = parse_stmt(p);
        if (node != NULL) {
            parser_block_push_node(&block, node);
        }
    }

    ASTNode *ast = malloc(sizeof(ASTNode));
    ast->type = NODE_BLOCK;
    ast->data.block = block;
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

ASTNode* parser_create_binary_node(char op, ASTNode *left, ASTNode *right) {
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