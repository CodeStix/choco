#ifndef AST_BLOCK_H
#define AST_BLOCK_H

#include "ast.h"
#include "common/list.h"

typedef struct ASTBlock ASTBlock;

ASTBlock* ast_block_malloc(List* statements);
ASTNode* ast_parse_block(List* tokens, unsigned int* i);

#endif   // AST_BLOCK_H