#include "ast/function.h"
#include "ast.h"
#include "ast/block.h"
#include "ast/modifiers.h"
#include "common/list.h"
#include "token.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

struct ASTFunction {
    Token* name_token;
    ASTNode* modifiers;
    ASTNode* parameter_type;
    ASTNode* return_type;
    ASTNode* body;
};

ASTFunction* ast_function_malloc(Token* name_token, ASTNode* parameter_type, ASTNode* return_type, ASTNode* body,
                                 ASTNode* modifiers) {
    ASTFunction* f = malloc(sizeof(ASTFunction));
    f->name_token = name_token;
    f->modifiers = modifiers;
    f->parameter_type = parameter_type;
    f->return_type = return_type;
    f->body = body;
    return f;
}

void ast_function_to_string(ASTNode* node, char* output, size_t max_length) {
    ASTFunction* func = (ASTFunction*)ast_node_data(node);

    unsigned int tok_len = 0;
    char* tok_value = token_value(func->name_token, &tok_len);

    snprintf(output, max_length, "Function { name=%.*s, exported=%s, extern=%s }", tok_len, tok_value,
             ast_modifiers_has(func->modifiers, MODIFIER_EXPORT) ? "yes" : "no",
             ast_modifiers_has(func->modifiers, MODIFIER_EXTERN) ? "yes" : "no");
}

ASTNode* ast_parse_function(List* tokens, unsigned int* i) {
    unsigned int start_token = *i;

    ASTNode* modifiers = ast_parse_modifiers(tokens, i);

    Token* tok = peek(tokens, i);
    if (token_type(tok) != TOKEN_FUNC_KEYWORD) {
        // This wasn't a function, return
        *i = start_token;
        return NULL;
    }

    tok = next(tokens, i);
    assert(token_type(tok) == TOKEN_WHITESPACE);

    tok = next(tokens, i);
    Token* func_name = NULL;
    if (token_type(tok) != TOKEN_SYMBOL) {
        printf("Warning: function does not have name\n");
    } else {
        func_name = tok;
        tok = next(tokens, i);
    }

    consume(tokens, i, TOKEN_WHITESPACE);

    // Parameters opening bracket
    tok = peek(tokens, i);
    assert(token_type(tok) == TOKEN_BRACKET_OPEN);
    next(tokens, i);

    // TODO read parameters
    ASTNode* parameter_type = NULL;
    consume(tokens, i, TOKEN_WHITESPACE);

    tok = peek(tokens, i);
    assert(token_type(tok) == TOKEN_BRACKET_CLOSE);
    next(tokens, i);

    // TODO read return type
    ASTNode* return_type = NULL;
    consume(tokens, i, TOKEN_WHITESPACE);

    ASTNode* body = ast_parse_block(tokens, i);
    assert(body != NULL);

    return ast_node_malloc(AST_FUNCTION, ast_function_malloc(func_name, parameter_type, return_type, body, modifiers),
                           start_token, *i);
}