#include "lang/lexer.h"
#include "lang/parser.h"
#include "lang/evaluator.h"

#include <stdio.h>
#include <string.h>

int main() {
    EvalCtx *ctx = ctx_new();

    while (true) {
        char buffer[8192];

        printf("$ ");
        if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
            buffer[strcspn(buffer, "\n")] = '\0';

            Lexer lexer = {
                .src = buffer,
                .pos = 0,
            };
            TokenArray tok_array = lex_all(&lexer);

            Parser parser = {
                .tok_array = tok_array,
                .pos = 0,
            };

            //ASTNode *ast = parse_block(&parser);

            size_t size = 0;
            size_t cap = 32;
            ASTNode *nodes = malloc(cap * sizeof(ASTNode));

            while (parser.pos < parser.tok_array.size && parser_peek(&parser).type != TOK_EOF && parser_peek(&parser).type != TOK_RBRACE) {
                ASTNode *node = parse_stmt(&parser);
                if (node != NULL) {
                    if (size >= cap) {
                        cap *= 2;
                        nodes = realloc(nodes, cap * sizeof(ASTNode));
                    }

                    nodes[size++] = *node;
                }
            }

            for (int i = 0; i < size; i++) {
                if (&nodes[i] != NULL) {
                    RuntimeValue value = eval(ctx, &nodes[i]);
                    parser_free_ast(&nodes[i]);
                }
            }

            free_tok_array(&tok_array);
        }
    }

    ctx_free(ctx);
}