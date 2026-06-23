#include "lang/lexer.h"
#include "lang/parser.h"
#include "lang/evaluator.h"

#include <stdio.h>

int main() {
    char input[] = "let $x = 2; let $y = ($x + 2);";

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

    RuntimeValue value = evaluate(ast);

    free_tok_array(&tok_array);

    if (ast != NULL) {
        parser_free_ast(ast);
    }
}