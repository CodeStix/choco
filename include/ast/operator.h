#ifndef AST_OPERATOR_H
#define AST_OPERATOR_H

#include "ast.h"
#include "token.h"

typedef struct ASTOperator ASTOperator;
typedef struct ASTUnaryOperator ASTUnaryOperator;

int operator_importance(TokenType type);

ASTOperator* ast_operator_malloc(Token* op, ASTNode* left, ASTNode* right);
ASTUnaryOperator* ast_unary_operator_malloc(Token* op, ASTNode* value);
void ast_operator_print(ASTNode* node, bool verbose, unsigned int indent);

void ast_unary_operator_print(ASTNode* node, bool verbose, unsigned int indent);

ASTNode* ast_parse_value_or_suffix(List* tokens, unsigned int* i);
ASTNode* ast_parse_value_or_operator(List* tokens, unsigned int* i);

#endif   // AST_OPERATOR_H