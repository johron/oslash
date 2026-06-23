#include "lang/lexer.h"
#include "lang/parser.h"

#include <stdio.h>

int main() {
    char input[] = "echo 1 \"hihihi\" 3";

    Lexer lexer = {
        .src = input,
        .pos = 0,
    };
    TokenArray tok_array = lex_all(&lexer);

    Parser parser = {
        .tok_array = tok_array,
        .pos = 0,
    };

    ASTNode *ast = parse_block(&parser);
    printf("%d\n", ast->type);

    free_tok_array(&tok_array);

    if (ast != NULL) {
        parser_free_ast(ast);
    }
}