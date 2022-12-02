#include "ast/operator.h"
#include "ast.h"
#include "ast/value.h"
#include <assert.h>

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

ASTOperator* ast_unary_operator_malloc(Token* op, ASTNode* value) {
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
        break;
    case TOKEN_PERIOD:
        // TODO: member access
        break;
    case TOKEN_SQUARE_BRACKET_OPEN:
        // TODO: array access (when 'value' is value) or array initializer (when 'value' is type)
        break;

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
    int importance = operator_importance(op_tok);
    if (importance < 0) {
        // Just return value, not an operator
        return left;
    }

    next(tokens, i);
    consume(tokens, i, TOKEN_WHITESPACE);

    ASTNode* right = ast_parse_value_or_operator(tokens, i);
    ASTOperator* top = ast_operator_malloc(op_tok, left, right);

    if (ast_node_type(right) == AST_OPERATOR) {
        // Right side also an operator, make sure the right precedence is used
        ASTOperator* right_op = (ASTOperator*)ast_node_data(right);

        if (importance > operator_importance(right_op->op)) {
            // The AST should be rotated to the left

            top->left = right_op->left;
            right_op->left = top;
            top = right_op;
        }
    }

    return ast_node_malloc(AST_OPERATOR, top, start_token, *i);
}
