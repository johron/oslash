#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>

// HEADER

#ifndef OAR_LANG_H
#define OAR_LANG_H

/* Misc */

typedef struct {
    char* type; // syntax, runtime..
    char* message;
} Error;

Error mkerr(char* type, char* message, ...);

//typedef enum {
//    RES_OK,
//    RES_ERR
//} ResultTag;
//
//typedef struct {
//    ResultTag tag;
//    char message[256];
//} Result;

/* Lexer */

typedef enum {
    TOK_STR,
    TOK_NUM,
    TOK_FLOAT,

    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_PERCENT,

    TOK_EQUAL,
    TOK_LESS,
    TOK_MORE,
    TOK_BANG,
    TOK_PIPE,
    TOK_DOLLAR,
    TOK_ARROW,

    TOK_DB_MORE,
    TOK_DB_EQUAL,
    TOK_BANG_EQUAL,
    TOK_LESS_EQUAL,
    TOK_MORE_EQUAL,

    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACE,
    TOK_RBRACE,
    //TOK_LBRACKET,
    //TOK_RBRACKET,

    TOK_SEMICOLON,
    TOK_EOF,
} TokenType;    

typedef union {
    long int num_value;
    double float_value;
    char* str_value;
} TokenValue;

typedef struct {
    TokenType type;
    TokenValue value;
} Token;

typedef struct {
    const char *src;
    size_t pos;
} Lexer;

typedef struct {
    Token *data;
    size_t size;
    size_t cap;
} TokenArray;

const char* get_token_type_string(TokenType type);

bool lex_all(Lexer *l, TokenArray *out, Error *err);
void free_tok_array(TokenArray *a);

/* Parser */

typedef struct {
    TokenArray tok_array;
    size_t pos;
} Parser;

typedef enum {
    NODE_BLOCK,
    NODE_BINARY_OP,
    NODE_UNARY_OP,

    NODE_VAR_DECL_STMT,
    NODE_FUNC_DECL_STMT,
    NODE_FUNC_CALL_STMT,

    NODE_VALUE_NUMBER,
    NODE_VALUE_FLOAT,
    NODE_VALUE_STRING,
    NODE_VALUE_VAR_REF,
} NodeType;

static inline char* get_node_type_string(NodeType type) {
    switch (type) {
        case NODE_BLOCK: return "NODE_BLOCK";
        case NODE_BINARY_OP: return "NODE_BINARY_OP";
        case NODE_UNARY_OP: return "NODE_UNARY_OP";
        case NODE_VAR_DECL_STMT: return "NODE_VAR_DECL_STMT";
        case NODE_FUNC_DECL_STMT: return "NODE_FUNC_DECL_STMT";
        case NODE_FUNC_CALL_STMT: return "NODE_FUNC_CALL_STMT";
        case NODE_VALUE_NUMBER: return "NODE_VALUE_NUMBER";
        case NODE_VALUE_FLOAT: return "NODE_VALUE_FLOAT";
        case NODE_VALUE_STRING: return "NODE_VALUE_STRING";
        case NODE_VALUE_VAR_REF: return "NODE_VALUE_VAR_REF";
        default: {
            return "Unknown NodeType";
        }
    }
};

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
    Block args;
} FuncCall;

typedef struct {
    char* name;
    ASTNode *expr;
} VarDecl;

struct ASTNode {
    NodeType type;
    union {
        Block block;
        VarDecl var_decl;
        FuncDecl func_decl;
        FuncCall func_call;

        struct {
            char op;
            ASTNode *left;
            ASTNode *right;
        } binary_op;
        struct {
            char op;
            ASTNode *operand; 
        } unary_op;
        TokenValue value;
    } data;
};

bool parse_stmt(Parser *p, ASTNode *out, Error *err);

bool parse_var_decl_stmt(Parser *p, ASTNode *out, Error *err);
ASTNode* parse_func_decl_stmt(Parser *p);
bool parse_func_call_stmt(Parser *p, ASTNode *out, Error *err);

bool parse_block(Parser *p, ASTNode *out, Error *err);

bool parser_init_block(Block *block, Error *err);
void parser_block_push_node(Block *block, ASTNode *node);

ASTNode* parser_create_member_node(NodeType type, TokenValue value);
ASTNode* parser_create_binary_op_node(char op, ASTNode *left, ASTNode *right);
bool parser_create_unary_op_node(char op, ASTNode *operand, ASTNode *out, Error *err);

Token parser_peek(Parser *p);
Token parser_advance(Parser *p);
bool parser_expect(Parser *p, TokenType type, const char *message, Token *out, Error *err);

bool parse_expr(Parser *p, ASTNode *out, Error *err);
bool parse_term_expr(Parser *p, ASTNode *out, Error *err);
bool parse_unary_expr(Parser *p, ASTNode *out, Error *err);
bool parse_primary_expr(Parser *p, ASTNode *out, Error *err);

void parser_free_ast(ASTNode *node);

/* Evaluator */

typedef enum {
    ENTRY_VAR,
    ENTRY_FUNC,
} EntryType;

static inline const char* get_entry_type_string(EntryType type) {
    switch (type) {
        case ENTRY_VAR: return "ENTRY_VAR";
        case ENTRY_FUNC: return "ENTRY_FUNC";
        default: return "Unknown EntryType";
    }
};

static inline const char* get_entry_type_string_pretty(EntryType type) {
    switch (type) {
        case ENTRY_VAR: return "variable";
        case ENTRY_FUNC: return "function";
        default: return "Unknown EntryType";
    }
};

typedef enum {
    VAL_NUM,
    VAL_FLOAT,
    VAL_STR,
    VAL_VOID,
} ValueType;

typedef struct {
    ValueType type;
    union {
        int num_val;
        double float_val;
        char *str_val;
    };
} RuntimeValue;

typedef RuntimeValue (*RuntimeFunc)(RuntimeValue *args, size_t argc);

static inline RuntimeValue val_num(int v) {
    return (RuntimeValue){ .type = VAL_NUM, .num_val = v };
}

static inline RuntimeValue val_float(double v) {
    return (RuntimeValue){ .type = VAL_FLOAT, .float_val = v };
}

static inline RuntimeValue val_void() {
    return (RuntimeValue){ .type = VAL_VOID };
}

RuntimeValue val_str(const char *s);
void val_free(RuntimeValue *v);
RuntimeValue val_clone(RuntimeValue v);

typedef struct EnvEntry EnvEntry;
typedef struct Env Env;

struct EnvEntry {
    char *name;
    EntryType type;
    union {
        RuntimeValue value;
        RuntimeFunc func;
    };
    EnvEntry *next;
};

struct Env {
    EnvEntry *head;
    Env *parent;
};

Env *env_new(Env *parent);
void env_free(Env *env);
void env_set_var(Env *env, const char *name, RuntimeValue value);
void env_set_func(Env *env, const char *name, RuntimeFunc func);
bool env_get_var(Env *env, const char *name, RuntimeValue *out);
RuntimeFunc env_get_func(Env *env, const char *name);

void entry_free(EnvEntry *entry);

typedef struct {
    Env          *env;
#ifdef OAR_USE_EXTERNAL_FUNCTION_SOURCE
    bool (*try_run_func_external)(Env *env, const char *name, RuntimeValue *args, size_t argc);
#endif
} EvalCtx;

EvalCtx *ctx_new(
    #ifdef OAR_USE_EXTERNAL_FUNCTION_SOURCE
        bool (*try_run_func_external)(Env *env, const char *name, RuntimeValue *args, size_t argc)
    #else
        void
    #endif
);

void ctx_free(EvalCtx *ctx);

bool eval(EvalCtx *ctx, ASTNode *node, RuntimeValue *out, Error *err);
bool eval_arith(char op, RuntimeValue left, RuntimeValue right, RuntimeValue *out, Error* err);

RuntimeValue builtin_echo(RuntimeValue *args, size_t argc);

#endif

// Implementation

#ifdef OAR_LANG_IMPLEMENTATION

/* Misc */

Error mkerr(char* type, char* message, ...) {
    va_list args1, args2;
    va_start(args1, message);
    va_copy(args2, args1);

    int length = vsnprintf(NULL, 0, message, args1);
    va_end(args1);

    if (length < 0) {
        va_end(args2);
        return (Error) {
            .type = "runtime",
            .message = "Could not make error struct, length less that zero?!"
        };
    }

    char *buffer = malloc(length + 1);
    if (buffer == NULL) {
        va_end(args2);
        return (Error) {
            .type = "runtime",
            .message = "Could not allocate memory for error message",
        };
    }

    vsnprintf(buffer, length + 1, message, args2);

    Error err = {
        .type = type,
        .message = buffer,
    };

    va_end(args2);

    return err;
}

/* Lexer */

const char* get_token_type_string(TokenType type) {
    switch (type) {
        case TOK_STR: return "TOK_STR";
        case TOK_NUM: return "TOK_NUM";
        case TOK_FLOAT: return "TOK_FLOAT";
        case TOK_PLUS: return "TOK_PLUS";
        case TOK_MINUS: return "TOK_MINUS";
        case TOK_STAR: return "TOK_STAR";
        case TOK_SLASH: return "TOK_SLASH";
        case TOK_PERCENT: return "TOK_PERCENT";
        case TOK_EQUAL: return "TOK_EQUAL";
        case TOK_LESS: return "TOK_LESS";
        case TOK_MORE: return "TOK_MORE";
        case TOK_BANG: return "TOK_BANG";
        case TOK_PIPE: return "TOK_PIPE";
        case TOK_DOLLAR: return "TOK_DOLLAR";
        case TOK_ARROW: return "TOK_ARROW";
        case TOK_DB_MORE: return "TOK_DB_MORE";
        case TOK_DB_EQUAL: return "TOK_DB_EQUAL";
        case TOK_BANG_EQUAL: return "TOK_BANG_EQUAL";
        case TOK_LESS_EQUAL: return "TOK_LESS_EQUAL";
        case TOK_MORE_EQUAL: return "TOK_MORE_EQUAL";
        case TOK_LPAREN: return "TOK_LPAREN";
        case TOK_RPAREN: return "TOK_RPAREN";
        case TOK_LBRACE: return "TOK_LBRACE";
        case TOK_RBRACE: return "TOK_RBRACE";
        case TOK_SEMICOLON: return "TOK_SEMICOLON";
        case TOK_EOF: return "TOK_EOF";
        default: return "Unknown TokenType";
    }
}

char lexer_peek(Lexer *l) {
    return l->src[l->pos];
}

char lexer_advance(Lexer *l) {
    return l->src[l->pos++];
}

void lexer_skip_ws(Lexer *l) {
    while (isspace(lexer_peek(l))) lexer_advance(l);
}

bool lexer_next_token(Lexer *l, Token *out, Error *err) {
    lexer_skip_ws(l);

    char c = lexer_peek(l);

    if (isdigit(c)) {
        long int num = 0;

        while (isdigit(lexer_peek(l))) {
            num = num * 10 + (lexer_advance(l) - '0');
        }

        double decimal = 0.0;
        if (lexer_peek(l) == '.') {
            lexer_advance(l);
            double place = 0.1;
            while (isdigit(lexer_peek(l))) {
                decimal += (lexer_advance(l) - '0') * place;
                place *= 0.1f;
            }

            *out = (Token){
                .type = TOK_FLOAT,
                .value = {
                    .float_value = num + decimal,
                }
            };
            return true;
        }

        *out = (Token){
            .type = TOK_NUM,
            .value = {
                .num_value = num,
            }
        };
        return true;
    }


    if (isalpha((unsigned char)c)) {
        size_t cap = 128;
        char *str = malloc(cap);
        size_t i = 0;

        while ((c = lexer_peek(l)), isalnum((unsigned char)c)) {
            if (i >= cap) {
                cap *= 2;
                char *next_str = realloc(str, cap);
                if (next_str == NULL) {
                    free(str);

                    *err = mkerr("lexer", "could not reallocate string");
                    return false;
                }
                str = next_str;
            }
            
            lexer_advance(l);
            str[i++] = c;
        }

        str[i] = '\0';

        *out = (Token){
            .type = TOK_STR,
            .value = {
                .str_value = str,
            }
        };
        return true;
    }

    if (c == '"') {
        lexer_advance(l);
        size_t cap = 128;
        char *str = malloc(cap);
        size_t i = 0;

        while (lexer_peek(l) != '"' && lexer_peek(l) != '\0') {
            c = lexer_advance(l);
            if (i >= cap - 1) {
                cap *= 2;
                char *next_str = realloc(str, cap);
                if (next_str == NULL) {
                    free(str);
                    *err = mkerr("lexer", "could not reallocate string");
                    return false;
                }
                str = next_str;
            }
            
            str[i++] = c;
        }

        str[i] = '\0';

        if (lexer_peek(l) == '"') {
            lexer_advance(l);
        } else {
            free(str);
            *err = mkerr("lexer", "missing '\"' to close string");
            return false;
        }

        *out = (Token){
            .type = TOK_STR,
            .value = {
                .str_value = str,
            }
        };
        return true;
    }

    lexer_advance(l);

    switch (c) {
        case '+': *out = (Token) { .type = TOK_PLUS }; return true;
        case '*': *out = (Token) { .type = TOK_STAR }; return true;
        case '/': *out = (Token) { .type = TOK_SLASH }; return true;
        case '%': *out = (Token) { .type = TOK_PERCENT }; return true;

        case '-': {
            if (lexer_peek(l) == '>') {
                lexer_advance(l);
                *out = (Token) { .type = TOK_ARROW };
                return true;
            } else {
                *out = (Token) { .type = TOK_MINUS};
                return true;
            }
        };
        case '=': {
            if (lexer_peek(l) == '=') {
                lexer_advance(l);
                *out = (Token) { .type = TOK_DB_EQUAL }; return true;
            } else {
                *out = (Token) { .type = TOK_EQUAL }; return true;
            }
        };
        case '<': {
            if (lexer_peek(l) == '=') {
                lexer_advance(l);
                *out = (Token) { .type = TOK_LESS_EQUAL }; return true;
            } else {
                *out = (Token) { .type = TOK_LESS }; return true;
            }
        };
        case '>': {
            if (lexer_peek(l) == '=') {
                lexer_advance(l);
                *out = (Token) { .type = TOK_MORE_EQUAL }; return true;
            } else if (lexer_peek(l) == '>') {
                lexer_advance(l);
                *out = (Token) { .type = TOK_DB_MORE }; return true;
            } else {
                *out = (Token) { .type = TOK_MORE }; return true;
            }
        };
        case '!': {
            if (lexer_peek(l) == '=') {
                lexer_advance(l);
                *out = (Token) { .type = TOK_BANG_EQUAL }; return true;
            } else {
                *out = (Token) { .type = TOK_BANG }; return true;
            }
        };
        
        case '|': *out = (Token) { .type = TOK_PIPE }; return true;
        case '$': *out = (Token) { .type = TOK_DOLLAR }; return true;
 
        case '(': *out = (Token) { .type = TOK_LPAREN }; return true;
        case ')': *out = (Token) { .type = TOK_RPAREN }; return true;
        case '{': *out = (Token) { .type = TOK_LBRACE }; return true;
        case '}': *out = (Token) { .type = TOK_RBRACE }; return true;

        case ';': *out = (Token){ .type = TOK_SEMICOLON }; return true;
        case '\0': *out = (Token){ .type = TOK_EOF }; return true;

        default:
            *err = mkerr("lexer", "unknown character '%c'", c);
            return false;
    }
}

bool init_tok_array(TokenArray *a, Error *err) {
    a->size = 0;
    a->cap = 32;
    a->data = malloc(a->cap * sizeof(Token));

    if (a->data == NULL) {
        *err = mkerr("lexer", "could not allocate memory for token array data");
        return false;
    }

    return true;
}

void push_token(TokenArray *a, Token t) {
    if (a->size >= a->cap) {
        a->cap *= 2;
        a->data = realloc(a->data, a->cap * sizeof(Token));
    }

    a->data[a->size++] = t;
}

bool lex_all(Lexer *l, TokenArray *out, Error *err) {
    TokenArray arr;

    if (init_tok_array(&arr, err) == false) {
        return false;
    }

    while (true) {
        Token t;
        if (lexer_next_token(l, &t, err) == false) {
            return false;
        }

        push_token(&arr, t);

        if (t.type == TOK_EOF)
            break;
    }

    *out = arr;
    return true;
}

void free_tok_array(TokenArray *a) {
    if (a == NULL) return;

    for (size_t i = 0; i < a->size; i++) {
        switch (a->data[i].type) {
            case TOK_STR: {
                if (a->data[i].value.str_value != NULL) {
                    free(a->data[i].value.str_value);
                    a->data[i].value.str_value = NULL;
                }
                break;
            };
            default: {};
            // case TOK_NUM: {}; // This tok has data, but not in heap
        }
    }

    free(a->data);
    a->data = NULL;

    a->size = 0;
    a->cap = 0;
}

/* Parser */

bool parser_init_block(Block *block, Error *err) {
    block->size = 0;
    block->cap = 32;
    block->nodes = malloc(block->cap * sizeof(ASTNode));

    if (block->nodes == NULL) {
        *err = mkerr("parser", "could not allocate memory for nodes in block");
        return false;
    }

    return true;
}

void parser_block_push_node(Block *block, ASTNode *node) {
    if (block->size >= block->cap) {
        block->cap *= 2;
        block->nodes = realloc(block->nodes, block->cap * sizeof(ASTNode));
    }

    block->nodes[block->size++] = *node;
}

bool parse_block(Parser *p, ASTNode *out, Error *err) {
    Block block;
    if (parser_init_block(&block, err) == false) return false;

    while (p->pos < p->tok_array.size &&
           parser_peek(p).type != TOK_EOF &&
           parser_peek(p).type != TOK_RBRACE) {
        ASTNode node = {0};
        if (parse_stmt(p, &node, err) == false) return false;
        parser_block_push_node(&block, &node);
    }

    out->type = NODE_BLOCK;
    out->data.block = block;
    return true;
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

bool parser_expect(Parser *p, TokenType type, const char *message, Token *out, Error *err) {
    if (parser_peek(p).type == type) {
        *out = parser_advance(p);
        return true;
    }

    *err = mkerr("parser", "%s, got %s", message, get_token_type_string(parser_peek(p).type));
    return false;
}

void parser_free_ast(ASTNode *node) {
    if (!node) return;

    if (node->type == NODE_BINARY_OP) {
        parser_free_ast(node->data.binary_op.left);
        parser_free_ast(node->data.binary_op.right);
    } else if (node->type == NODE_UNARY_OP) {
        parser_free_ast(node->data.unary_op.operand);
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

bool parser_create_unary_op_node(char op, ASTNode *operand, ASTNode *out, Error *err) {
    ASTNode *node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) {
        *err = mkerr("parser", "allocation failed for unary node");
        return false;
    }
    
    node->type = NODE_UNARY_OP;
    node->data.unary_op.op = op;
    node->data.unary_op.operand = operand;
    
    *out = *node;
    return true;
}


//bool parse_stmt(Parser *p, ASTNode *out, Error *err);

bool parse_stmt(Parser *p, ASTNode *out, Error *err) {
    switch (p->tok_array.data[p->pos].type) {
        case TOK_STR: {
            Token tok = parser_peek(p);

            if (strcmp(tok.value.str_value, "fn") == 0) { // fn x(...)
                fprintf(stderr, "Convert parse_func_decl_stmt() to new error system, also implement function declarations in parser and eval\n");
                exit(1);
                //return parse_func_decl_stmt(p);
            } else if (strcmp(tok.value.str_value, "let") == 0) { // let $x = ... (;)
                if (parse_var_decl_stmt(p, out, err) == false) return false;
            } else { // treat as function call 
                if (parse_func_call_stmt(p, out, err) == false) return false;
            }

            return true;
        };
        case TOK_LPAREN: { // statement enclosure
            parser_advance(p);
            if (parse_stmt(p, out, err) == false) return false;
            if (parser_expect(p, TOK_RPAREN, "Expected parenthesis to close statement enclosure", out, err) == false) return false;
            return true;
        };
        default: { // TODO: add some expression statement stuff somewhere here
            *err = mkerr("parser", "unexpected tokens found while parsing statement, got '%s'", get_token_type_string(p->tok_array.data[p->pos].type));
            return false;
        };
    }
}

bool parse_var_decl_stmt(Parser *p, ASTNode *out, Error *err) {
    Token trash, var_tok;

    if (parser_expect(p, TOK_STR,    "Expected 'let'",      &trash,   err) == false) return false;
    if (parser_expect(p, TOK_DOLLAR, "Expected '$'",         &trash,   err) == false) return false;
    if (parser_expect(p, TOK_STR,    "Expected var name",    &var_tok, err) == false) return false;
    if (parser_expect(p, TOK_EQUAL,  "Expected '='",         &trash,   err) == false) return false;

    ASTNode *expr = malloc(sizeof(ASTNode));
    if (parse_expr(p, expr, err) == false) return false;

    if (parser_peek(p).type != TOK_EOF) {
        if (parser_expect(p, TOK_SEMICOLON, "Expected ';'", &trash, err) == false) return false;
    }

    out->type = NODE_VAR_DECL_STMT;
    out->data.var_decl.name = var_tok.value.str_value;
    out->data.var_decl.expr = expr;
    return true;
}

bool parse_func_call_stmt(Parser *p, ASTNode *out, Error *err) {
    Token trash, func_tok;
    if (parser_expect(p, TOK_STR, "Expected function name", &func_tok, err) == false) return false;

    Block args;
    if (parser_init_block(&args, err) == false) return false;

    while (parser_peek(p).type != TOK_EOF && parser_peek(p).type != TOK_SEMICOLON) {
        ASTNode *expr = malloc(sizeof(ASTNode));
        if (parser_peek(p).type == TOK_LPAREN) {
            parser_advance(p);
            if (parse_expr(p, expr, err) == false) return false;
            if (parser_expect(p, TOK_RPAREN, "Expected ')'", &trash, err) == false) return false;
        } else {
            if (parse_unary_expr(p, expr, err) == false) return false;
        }
        parser_block_push_node(&args, expr);
    }

    if (parser_peek(p).type != TOK_EOF) {
        if (parser_expect(p, TOK_SEMICOLON, "Expected ';'", &trash, err) == false) return false;
    }

    out->type = NODE_FUNC_CALL_STMT;
    out->data.func_call.name = func_tok.value.str_value;
    out->data.func_call.args = args;
    return true;
}

ASTNode* parse_func_decl_stmt(Parser *p) {
    /*ASTNode *node = malloc(sizeof(ASTNode));

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

    return node;*/
}

bool parse_primary_expr(Parser *p, ASTNode *out, Error *err) {
    Token token = parser_peek(p);

    Token trash;

    switch (token.type) {
        case TOK_NUM: parser_advance(p); *out = *parser_create_member_node(NODE_VALUE_NUMBER, token.value); return true;
        case TOK_FLOAT: parser_advance(p); *out = *parser_create_member_node(NODE_VALUE_FLOAT, token.value); return true;
        case TOK_STR: parser_advance(p); *out = *parser_create_member_node(NODE_VALUE_STRING, token.value); return true;

        case TOK_DOLLAR: {
            parser_advance(p);
            Token var_tok;
            if (parser_expect(p, TOK_STR, "Expected variable name for variable reference", &var_tok, err) == false) return false;

            ASTNode *node = parser_create_member_node(NODE_VALUE_VAR_REF, var_tok.value);
            *out = *node;

            return true;
        };

        case TOK_LPAREN: {
            parser_advance(p);
            ASTNode node;
            if (parse_expr(p, &node, err) == false) return false;
            if (parser_expect(p, TOK_RPAREN, "Expected ')' after expression", &trash, err) == false) return false;

            *out = node;
            return true;
        };

        default: {
            *err = mkerr("parser", "unexpected token in primary expression %s", get_token_type_string(token.type));
            return false;
        }
    }
}

bool parse_unary_expr(Parser *p, ASTNode *out, Error *err) {
    Token token = parser_peek(p);

    if (token.type == TOK_MINUS || token.type == TOK_PLUS || token.type == TOK_BANG) {
        parser_advance(p);
        
        char op;
        if (token.type == TOK_MINUS) op = '-';
        else if (token.type == TOK_PLUS) op = '+';
        else if (token.type == TOK_BANG) op = '!';
        else {
            *err = mkerr("parser", "invalid operator for unary expression '%c'", op);
            return false;
        }

        ASTNode *operand;
        if (parse_unary_expr(p, &operand, err) == false) return false;

        if (parser_create_unary_op_node(op, operand, out, err) == false) return false;

        return true;
    }

    if (parse_primary_expr(p, out, err) == false) return false;

    return true;
}

bool parse_term_expr(Parser *p, ASTNode *out, Error *err) {
    ASTNode expr;
    if (parse_unary_expr(p, &expr, err) == false) return false; 

    while (parser_peek(p).type == TOK_STAR || parser_peek(p).type == TOK_SLASH) {
        Token op_token = parser_peek(p);
        parser_advance(p);
        
        char op = (op_token.type == TOK_STAR) ? '*' : '/';
        
        ASTNode *right; 
        if (parse_unary_expr(p, &right, err) == false) return false; 
        expr = *parser_create_binary_op_node(op, &expr, right);
    }

    *out = expr;
    return true;
}

bool parse_expr(Parser *p, ASTNode *out, Error *err) {
    ASTNode left;
    if (parse_term_expr(p, &left, err) == false) return false;

    while (parser_peek(p).type == TOK_PLUS || parser_peek(p).type == TOK_MINUS) {
        Token op_token = parser_advance(p);
        char op = (op_token.type == TOK_PLUS) ? '+' : '-';

        ASTNode right;
        if (parse_term_expr(p, &right, err) == false) return false;

        ASTNode *lhs = malloc(sizeof(ASTNode));
        ASTNode *rhs = malloc(sizeof(ASTNode));
        *lhs = left;
        *rhs = right;

        left = *parser_create_binary_op_node(op, lhs, rhs);
    }

    *out = left;
    return true;
}

/* Evaluator */

RuntimeValue val_str(const char *s) {
    RuntimeValue v = { .type = VAL_STR };
    v.str_val = strdup(s);
    return v;
}

void val_free(RuntimeValue *v) {
    if (v && v->type == VAL_STR && v->str_val) {
        free(v->str_val);
        v->str_val = NULL;
    }
}

RuntimeValue val_clone(RuntimeValue v) {
    if (v.type == VAL_STR && v.str_val)
        return val_str(v.str_val);
    return v;
}

void entry_free(EnvEntry *entry) {
    if (entry->type == ENTRY_VAR) {
        if (entry->value.type == VAL_STR && entry->value.str_val) {
            free(entry->value.str_val);
            entry->value.str_val = NULL;
        }
    // } else if (entry->type == ENTRY_FUNC) { // not needed
    }
}

Env *env_new(Env *parent) {
    Env *e = malloc(sizeof(Env));
    e->head   = NULL;
    e->parent = parent;
    return e;
}

void env_free(Env *env) {
    EnvEntry *e = env->head;
    while (e) {
        EnvEntry *next = e->next;
        free(e->name);
        entry_free(e);
        free(e);
        e = next;
    }
    free(env);
}

void env_set_var(Env *env, const char *name, RuntimeValue value) {
    for (Env *scope = env; scope; scope = scope->parent) {
        for (EnvEntry *e = scope->head; e; e = e->next) {
            if (strcmp(e->name, name) == 0) {
                if (e->type != ENTRY_VAR) {
                    fprintf(stderr, "Cannot redefine %s '%s' as variable\n", get_entry_type_string_pretty(e->type), name);
                    exit(1);
                }

                val_free(&e->value); // TODO: implement val_free
                e->value = val_clone(value);
                return;
            }
        }
    }

    EnvEntry *entry = malloc(sizeof(EnvEntry));
    entry->name = strdup(name);
    entry->type = ENTRY_VAR;
    entry->value = val_clone(value);
    entry->next = env->head;
    env->head = entry;
}

void env_set_func(Env *env, const char *name, RuntimeFunc func) {
    for (Env *scope = env; scope; scope = scope->parent) {
        for (EnvEntry *e = scope->head; e; e = e->next) {
            if (strcmp(e->name, name) == 0) {
                if (e-> type != ENTRY_FUNC) {
                    fprintf(stderr, "Cannot redefine already defined %s '%s'\n", get_entry_type_string_pretty(e->type), name);
                    exit(1);
                }

                fprintf(stderr, "Cannot redefine already defined function '%s\n", name);
                exit(1);
            }
        }
    }

    EnvEntry *entry = malloc(sizeof(EnvEntry));
    entry->name = strdup(name);
    entry->type = ENTRY_FUNC;
    entry->func = func;
    entry->next = env->head;
    env->head = entry;
}

bool env_get_var(Env *env, const char *name, RuntimeValue *out) {
    for (Env *scope = env; scope; scope = scope->parent) {
        for (EnvEntry *e = scope->head; e; e = e->next) {
            if (strcmp(e->name, name) == 0) {
                if (e->type != ENTRY_VAR) {
                    fprintf(stderr, "Tried to reference variable as %s\n", get_entry_type_string_pretty(e->type));
                    exit(1);
                }

                *out = val_clone(e->value);
                return true;
            }
        }
    }

    return false;
}

RuntimeFunc env_get_func(Env *env, const char *name) {
    for (Env *scope = env; scope; scope = scope->parent) {
        for (EnvEntry *e = scope->head; e; e = e->next) {
            if (strcmp(e->name, name) == 0) {
                if (e->type != ENTRY_FUNC) {
                    fprintf(stderr, "Tried to reference function as %s\n", get_entry_type_string_pretty(e->type));
                    exit(1);
                }

                return e->func;
            }
        }
    }

    return NULL;
}

EvalCtx *ctx_new(
    #ifdef OAR_USE_EXTERNAL_FUNCTION_SOURCE
        bool (*try_run_func_external)(Env *env, const char *name, RuntimeValue *args, size_t argc)
    #else
        void
    #endif
) {
    EvalCtx *ctx  = malloc(sizeof(EvalCtx));
    ctx->env      = env_new(NULL);

    #ifdef OAR_USE_EXTERNAL_FUNCTION_SOURCE
    if (try_run_func_external != NULL) {
        ctx->try_run_func_external = try_run_func_external;
    }
    #endif

    env_set_func(ctx->env, "echo", builtin_echo);

    return ctx;
}

void ctx_free(EvalCtx *ctx) {
    env_free(ctx->env);
    free(ctx);
}

bool eval_arith(char op, RuntimeValue left, RuntimeValue right, RuntimeValue *out, Error* err) {
    if (left.type == VAL_FLOAT || right.type == VAL_FLOAT) {
        double l = (left.type == VAL_FLOAT) ? left.float_val : (double) left.num_val;
        double r = (right.type == VAL_FLOAT) ? right.float_val : (double) right.num_val;

        switch (op) {
            case '+': {
                RuntimeValue v = val_float(l + r);
                *out = v;
                return true;
            };
            case '-': {
                RuntimeValue v = val_float(l - r);
                *out = v;
                return true;
            };
            case '*': {
                RuntimeValue v = val_float(l * r);
                *out = v;
                return true;
            };
            case '/': {
                if (r == 0.0) {
                    *err = mkerr("runtime", "division by zero");
                    return false;
                }
                RuntimeValue v = val_float(l / r);
                *out = v;
                return true;
            };
        }
    }

    long int l = left.num_val;
    long int r = right.num_val;
    
    switch (op) {
        case '+': {
            RuntimeValue v = val_num(l + r);
            *out = v;
            return true;
        };
        case '-': {
            RuntimeValue v = val_num(l - r);
            *out = v;
            return true;
        };
        case '*': {
            RuntimeValue v = val_num(l * r);
            *out = v;
            return true;
        };
        case '/': {
            if (r == 0.0) {
                *err = mkerr("runtime", "division by zero");
                return false;
            }
            
            RuntimeValue v;
            if (l % r == 0) {
                v = val_num(l / r);
            } else {
                v = val_float((double) l / (double) r);
            }

            *out = v;
            return true;
        };
    }

    *err = mkerr("runtime", "unknown arithmetic operator '%c'", op);
    return false;
}

bool eval(EvalCtx *ctx, ASTNode *node, RuntimeValue *out, Error *err) {
    if (!node) {
        RuntimeValue v = val_num(0);
        *out = v;
        return true;
    }

    switch (node->type) {
        case NODE_VALUE_NUMBER: {
            RuntimeValue v = val_num(node->data.value.num_value);
            *out = v;
            return true;
        };
        case NODE_VALUE_FLOAT: {
            RuntimeValue v = val_float(node->data.value.float_value);
            *out = v;
            return true;
        };
        case NODE_VALUE_STRING: {
            RuntimeValue v = val_str(node->data.value.str_value);
            *out = v;
            return true;
        };

        case NODE_VALUE_VAR_REF: {
            RuntimeValue v;
            if (!env_get_var(ctx->env, node->data.value.str_value, &v)) {
                *err = mkerr("runtime", "undefined variable '$%s'", node->data.value.str_value);
                return false;
            }
            *out = v;
            return true;
        };

        case NODE_BINARY_OP: {
            RuntimeValue left, right;

            if (eval(ctx, node->data.binary_op.left, &left, err) == 0) {
                return false;
            }
            if (eval(ctx, node->data.binary_op.right, &right, err) == 0) {
                return false;
            }

            char op = node->data.binary_op.op;

            RuntimeValue result;
            if (op == '+' && (left.type == VAL_STR || right.type == VAL_STR)) {
                const char *left_side;
                char lbuf[64];
                if (left.type == VAL_STR) {
                    left_side = left.str_val;
                } else if (left.type == VAL_FLOAT) {
                    snprintf(lbuf, sizeof lbuf, "%g", left.float_val);
                    left_side = lbuf;
                } else {
                    snprintf(lbuf, sizeof lbuf, "%ld", (long)left.num_val);
                    left_side = lbuf;
                }

                const char *right_side;
                char rbuf[64];
                if (right.type == VAL_STR) {
                    right_side = right.str_val;
                } else if (right.type == VAL_FLOAT) {
                    snprintf(rbuf, sizeof rbuf, "%g", right.float_val);
                    right_side = rbuf;
                } else {
                    snprintf(rbuf, sizeof rbuf, "%ld", (long)right.num_val);
                    right_side = rbuf;
                }
                
                size_t len = strlen(left_side) + strlen(right_side) + 1;
                char *buf = malloc(len);
                snprintf(buf, len, "%s%s", left_side, right_side);
                result = (RuntimeValue) {
                    .type = VAL_STR,
                    .str_val = buf,
                };
            } else {
                RuntimeValue v;

                if (eval_arith(op, left, right, &v, err) == 0) {
                    return false;
                }

                result = v;
            }

            val_free(&left);
            val_free(&right);

            *out = result;
            return true;
        };

        case NODE_UNARY_OP: {
            RuntimeValue v;

            if (eval(ctx, node->data.unary_op.operand, &v, err) == 0) {
                return false;
            }

            char op = node->data.unary_op.op;
            if (op == '-') {
                if (v.type == VAL_FLOAT) {
                    v = val_float(-v.float_val);
                    *out = v;
                    return true;
                }

                v = val_num(-v.num_val);
                *out = v;
                return true;
            }
            if (op == '+') {
                *out = v;
                return true;
            }
            if (op == '!') {
                v = val_num(!v.num_val);
                *out = v;
                return true;
            }

            *err = mkerr("runtime", "unknown unary op '%c'", op);
            return false;
        };

        case NODE_VAR_DECL_STMT: {
            RuntimeValue v;

            if (eval(ctx, node->data.var_decl.expr, &v, err) == false) {
                return false;
            }

            env_set_var(ctx->env, node->data.var_decl.name, v);
            val_free(&v);
            v = val_void();
            *out = v;
            return true;
        };

        case NODE_FUNC_CALL_STMT: {
            const char *name = node->data.func_call.name;
            Block *args_block = &node->data.func_call.args;

            size_t argc = args_block ? args_block->size : 0;
            RuntimeValue *args = argc ? malloc(argc * sizeof(RuntimeValue)) : NULL;
            for (size_t i = 0; i < argc; i++) {
                RuntimeValue v;
                if (eval(ctx, &args_block->nodes[i], &v, err) == false) {
                    return false;
                }

                args[i] = v;
            }

            RuntimeValue result = val_void();

            RuntimeFunc func = env_get_func(ctx->env, name);
            if (func != NULL) {
                result = func(args, argc);
            } else {
                #ifdef OAR_USE_EXTERNAL_FUNCTION_SOURCE
                bool extern_func = ctx->try_run_func_external(ctx->env, name, args, argc);
                if (!extern_func) {
                    return false;
                }
                #else
                *err = mkerr("runtime", "unknown function '%s'", name);
                return false;
                #endif
            }

            for (size_t i = 0; i < argc; i++) val_free(&args[i]);
            free(args);

            *out = result;
            return true;
        };

        case NODE_FUNC_DECL_STMT: {
            *err = mkerr("runtime", "function declaration evaluation not implemented");
            return false;
        };

        case NODE_BLOCK: {
            RuntimeValue last = val_void();
            Env *child = env_new(ctx->env);
            Env *saved = ctx->env;
            ctx->env   = child;

            for (size_t i = 0; i < node->data.block.size; i++) {
                val_free(&last);

                RuntimeValue v;
                if (eval(ctx, &node->data.block.nodes[i], &v, err) == false) {
                    return false;
                }

                last = v;
            }

            ctx->env = saved;
            env_free(child);

            *out = last;
            return true;
        };

        default: {
            *err = mkerr("runtime", "unhandled node type %s (%d)", get_node_type_string(node->type), node->type);
            return false;
        };
    }
}

RuntimeValue builtin_echo(RuntimeValue *args, size_t argc) {
    for (size_t i = 0; i < argc; i++) {
        if (i > 0) printf(" ");
        
        RuntimeValue v = args[i];
        switch (v.type) {
            case VAL_NUM:   printf("%d",  v.num_val);   break;
            case VAL_FLOAT: printf("%g",  v.float_val); break;
            case VAL_STR:   printf("%s",  v.str_val);   break;
            case VAL_VOID:  printf("void"); break;
        }
    }
    printf("\n");
    return val_void();
}

#endif