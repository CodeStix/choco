#include "ast/block.h"
#include "ast.h"
#include "common/list.h"
#include "token.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

struct ASTBlock {
    List* statements_nodes;
};

ASTBlock* ast_block_malloc(List* statements) {
    ASTBlock* b = (ASTBlock*)malloc(sizeof(ASTBlock));
    b->statements_nodes = statements;
    return b;
}

ASTNode* ast_parse_statement(List* tokens, unsigned int* i) {
    return NULL;
};

ASTNode* ast_parse_block(List* tokens, unsigned int* i) {
    unsigned int start_token = *i;

    Token* tok = peek(tokens, i);
    if (token_type(tok) != TOKEN_CURLY_BRACKET_OPEN) {
        // This isn't a block
        return NULL;
    }
    next(tokens, i);

    consume(tokens, i, TOKEN_WHITESPACE);

    List* statement_list = list_malloc(4);

    while (true) {
        consume(tokens, i, TOKEN_WHITESPACE);

        tok = peek(tokens, i);
        if (token_type(tok) == TOKEN_CURLY_BRACKET_CLOSE) {
            next(tokens, i);
            consume(tokens, i, TOKEN_WHITESPACE);
            break;
        }

        ASTNode* statement = ast_parse_statement(tokens, i);
        if (statement == NULL) {
            printf("Invalid statement\n");
            token_highlight(tok);
            assert(false);
            return NULL;
        }

        list_add(statement_list, statement);

        tok = peek(tokens, i);
        if (token_type(tok) != TOKEN_SEMICOLON && token_type(tok) != TOKEN_CURLY_BRACKET_CLOSE) {
            printf("Expected ; or }\n");
            token_highlight(tok);
            assert(false);
            return NULL;
        }

        next(tokens, i);
    }

    return ast_node_malloc(AST_BLOCK, ast_block_malloc(statement_list), start_token, *i);
}