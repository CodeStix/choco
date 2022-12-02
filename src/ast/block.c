#include "ast/block.h"
#include "ast.h"
#include "ast/operator.h"
#include "ast/value.h"
#include "common/list.h"
#include "token.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

struct ASTBlock {
    List* statements_nodes;
};

struct ASTDeclaration {
    Token* name_token;
    bool is_const;
    ASTNode* type_specifier;
    ASTNode* initial_value;
};

ASTDeclaration* ast_declaration_malloc(Token* name_token, ASTNode* type_specifier, ASTNode* initial_value,
                                       bool is_const) {
    ASTDeclaration* d = (ASTDeclaration*)malloc(sizeof(ASTDeclaration));
    d->name_token = name_token;
    d->is_const = is_const;
    d->type_specifier = type_specifier;
    d->initial_value = initial_value;
    return d;
}

ASTBlock* ast_block_malloc(List* statements) {
    ASTBlock* b = (ASTBlock*)malloc(sizeof(ASTBlock));
    b->statements_nodes = statements;
    return b;
}

ASTNode* ast_parse_declaration(List* tokens, unsigned int* i) {
    unsigned int start_token = *i;

    Token* tok = peek(tokens, i);
    assert(token_type(tok) == TOKEN_LET_KEYWORD || token_type(tok) == TOKEN_CONST_KEYWORD);

    bool is_const = token_type(tok) == TOKEN_CONST_KEYWORD;

    next(tokens, i);
    consume(tokens, i, TOKEN_WHITESPACE);
    tok = peek(tokens, i);

    assert(token_type(tok) == TOKEN_SYMBOL);
    Token* name_tok = tok;

    next(tokens, i);
    consume(tokens, i, TOKEN_WHITESPACE);
    tok = peek(tokens, i);
    ASTNode* type_specifier = NULL;
    if (token_type(tok) == TOKEN_COLON) {
        // Parse type specifier
        next(tokens, i);
        consume(tokens, i, TOKEN_WHITESPACE);

        type_specifier = ast_parse_value_or_operator(tokens, i);
        assert(type_specifier != NULL);
    }

    tok = peek(tokens, i);
    ASTNode* initial_value = NULL;
    if (token_type(tok) == TOKEN_EQUAL) {
        // Parse initial value
        next(tokens, i);
        consume(tokens, i, TOKEN_WHITESPACE);

        initial_value = ast_parse_value_or_operator(tokens, i);
        assert(initial_value != NULL);
    }

    return ast_node_malloc(AST_DECLARATION, ast_declaration_malloc(name_tok, type_specifier, initial_value, is_const),
                           start_token, *i);
}

void ast_declaration_print(ASTNode* node, bool verbose, unsigned int indent) {
    ASTDeclaration* decl = (ASTDeclaration*)ast_node_data(node);

    unsigned int tok_len = 0;
    char* tok_value = token_value(decl->name_token, &tok_len);

    printf("%*sDeclaration { name=%.*s, const=%s }\n", indent, "", tok_len, tok_value, decl->is_const ? "yes" : "no");

    if (verbose) {
        printf("%*sType specifier:\n", indent + 2, "");
        if (decl->type_specifier != NULL) {
            ast_node_print(decl->type_specifier, true, indent + 4);
        } else {
            printf("%*sNULL\n", indent + 4, "");
        }

        printf("%*sInitial value:\n", indent + 2, "");
        if (decl->initial_value != NULL) {
            ast_node_print(decl->initial_value, true, indent + 4);
        } else {
            printf("%*sNULL\n", indent + 4, "");
        }
    }
}

ASTNode* ast_parse_statement(List* tokens, unsigned int* i) {
    Token* tok = peek(tokens, i);
    switch (token_type(tok)) {
    case TOKEN_SYMBOL: {
        ASTNode* symbol_op = ast_parse_value_or_operator(tokens, i);   // Includes '=' operator
        assert(symbol_op != NULL);
        return symbol_op;
    }
    case TOKEN_LET_KEYWORD:
    case TOKEN_CONST_KEYWORD: {
        ASTNode* decl = ast_parse_declaration(tokens, i);
        assert(decl != NULL);
        return decl;
    }

    default:
        return NULL;
    }
}

void ast_block_print(ASTNode* node, bool verbose, unsigned int indent) {
    ASTBlock* body = (ASTBlock*)ast_node_data(node);

    printf("%*sBlock { statements=%u }\n", indent, "", list_length(body->statements_nodes));

    if (verbose) {
        for (unsigned int i = 0; i < list_length(body->statements_nodes); i++) {
            ASTNode* n = list_get(body->statements_nodes, i);
            ast_node_print(n, true, indent + 2);
        }
    }
}

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
            token_highlight(tok, tok);
            assert(false);
            return NULL;
        }

        list_add(statement_list, statement);

        tok = peek(tokens, i);
        if (token_type(tok) != TOKEN_SEMICOLON) {
            if (token_type(tok) != TOKEN_CURLY_BRACKET_CLOSE) {
                printf("Expected ; or }\n");
                token_highlight(tok, tok);
                assert(false);
                return NULL;
            }
        } else {
            next(tokens, i);
            consume(tokens, i, TOKEN_WHITESPACE);
        }
    }

    return ast_node_malloc(AST_BLOCK, ast_block_malloc(statement_list), start_token, *i);
}