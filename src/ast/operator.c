#include "ast/operator.h"
#include "ast.h"
#include "ast/value.h"
#include <assert.h>
#include <stdio.h>

struct ASTOperator {
    ASTNode* left;
    ASTNode* right;
    Token* op;
};

struct ASTUnaryOperator {
    ASTNode* value;
    Token* op;
};

ASTOperator* ast_operator_malloc(Token* op, ASTNode* left, ASTNode* right) {
    ASTOperator* o = malloc(sizeof(ASTOperator));
    o->op = op;
    o->left = left;
    o->right = right;
    return o;
}

ASTUnaryOperator* ast_unary_operator_malloc(Token* op, ASTNode* value) {
    ASTUnaryOperator* o = malloc(sizeof(ASTUnaryOperator));
    o->op = op;
    o->value = value;
    return o;
}

// Operator with higher importance will be calculated first (thus placed lower in the AST)
// Returns -1 if not an operator
int operator_importance(TokenType type) {
    switch (type) {
    case TOKEN_EQUAL:
        return 10;
    case TOKEN_DOUBLE_EQUAL:
    case TOKEN_NOT_EQUAL:
        return 80;
    case TOKEN_MINUS:
    case TOKEN_PLUS:
        return 90;
    case TOKEN_SLASH:
    case TOKEN_STAR:
        return 100;

    default:
        return -1;
    }
}

ASTNode* ast_parse_value_or_suffix(List* tokens, unsigned int* i) {
    unsigned int start_token = *i;
    ASTNode* value = ast_parse_value(tokens, i);

    Token* tok = peek(tokens, i);
    switch (token_type(tok)) {
    case TOKEN_BRACKET_OPEN:
        // TODO: invocation
        return NULL;
    case TOKEN_PERIOD:
        // TODO: member access
        return NULL;
    case TOKEN_SQUARE_BRACKET_OPEN:
        // TODO: array access (when 'value' is value) or array initializer (when 'value' is type)
        return NULL;

    default:
        return value;
    }
}

ASTNode* ast_parse_value_or_prefix(List* tokens, unsigned int* i) {
    unsigned int start_token = *i;
    Token* tok = peek(tokens, i);

    Token* op_tok = NULL;
    ASTNode* value = NULL;

    switch (token_type(tok)) {
    case TOKEN_MINUS:
    case TOKEN_PLUS:
    case TOKEN_EXCLAMATION:
        next(tokens, i);
        consume(tokens, i, TOKEN_WHITESPACE);
        op_tok = tok;
        value = ast_parse_value_or_prefix(tokens, i);
        break;

    default:
        value = ast_parse_value_or_suffix(tokens, i);
        break;
    }

    if (op_tok != NULL) {
        return ast_node_malloc(AST_UNARY_OPERATOR, ast_unary_operator_malloc(op_tok, value), start_token, *i);
    } else {
        return value;
    }
}

ASTNode* ast_parse_value_or_operator(List* tokens, unsigned int* i) {
    unsigned int start_token = *i;
    ASTNode* left = ast_parse_value_or_prefix(tokens, i);

    consume(tokens, i, TOKEN_WHITESPACE);

    Token* op_tok = peek(tokens, i);
    int importance = operator_importance(token_type(op_tok));
    if (importance < 0) {
        // Just return value, not an operator
        return left;
    }

    next(tokens, i);
    consume(tokens, i, TOKEN_WHITESPACE);

    ASTNode* right = ast_parse_value_or_operator(tokens, i);
    ASTOperator* top_op = ast_operator_malloc(op_tok, left, right);
    ASTNode* top = ast_node_malloc(AST_OPERATOR, top_op, start_token, *i);

    if (ast_node_type(right) == AST_OPERATOR) {
        // Right side also an operator, make sure the right precedence is used
        ASTOperator* right_op = (ASTOperator*)ast_node_data(right);

        if (importance > operator_importance(token_type(right_op->op))) {
            // The AST should be rotated to the left

            top_op->left = right_op->left;
            right_op->left = top;
            top_op = right_op;
            top = right;
            // ast_node_set_data(top, top_op);
        }
    }

    return top;
}

void ast_operator_print(ASTNode* node, bool verbose, unsigned int indent) {
    ASTOperator* op = (ASTOperator*)ast_node_data(node);

    unsigned int tok_len = 0;
    char* tok_value = token_value(op->op, &tok_len);

    printf("%*sOperator { op=\"%.*s\" }\n", indent, "", tok_len, tok_value);

    if (verbose) {
        printf("%*sLeft:\n", indent + 2, "");
        ast_node_print(op->left, true, indent + 4);
        printf("%*sRight:\n", indent + 2, "");
        ast_node_print(op->right, true, indent + 4);
    }
}

void ast_unary_operator_print(ASTNode* node, bool verbose, unsigned int indent) {
    ASTUnaryOperator* op = (ASTUnaryOperator*)ast_node_data(node);

    unsigned int tok_len = 0;
    char* tok_value = token_value(op->op, &tok_len);

    printf("%*sUnaryOperator { op=\"%.*s\" }\n", indent, "", tok_len, tok_value);

    if (verbose) {
        ast_node_print(op->value, true, indent + 2);
    }
}