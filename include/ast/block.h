#ifndef AST_BLOCK_H
#define AST_BLOCK_H

#include "ast.h"
#include "common/list.h"

typedef struct ASTBlock ASTBlock;
typedef struct ASTDeclaration ASTDeclaration;

ASTBlock* ast_block_malloc(List* statements);
ASTNode* ast_parse_block(List* tokens, unsigned int* i);
void ast_block_print(ASTNode* node, bool verbose, unsigned int indent);

void ast_declaration_print(ASTNode* node, bool verbose, unsigned int indent);

#endif   // AST_BLOCK_H