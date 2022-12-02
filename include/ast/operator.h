#ifndef AST_OPERATOR_H
#define AST_OPERATOR_H

#include "ast.h"
#include "token.h"

typedef struct ASTOperator ASTOperator;
typedef struct ASTUnaryOperator ASTUnaryOperator;

ASTOperator* ast_operator_malloc(Token* op, ASTNode* left, ASTNode* right);
ASTOperator* ast_unary_operator_malloc(Token* op, ASTNode* value);

#endif   // AST_OPERATOR_H