#ifndef AST_FUNCTION_H
#define AST_FUNCTION_H

#include "ast.h"
#include "list.h"
#include "token.h"
#include <stdbool.h>
#include <stdlib.h>

typedef struct ASTFunction ASTFunction;

ASTFunction* ast_function_malloc(Token* name_token, ASTNode* parameter_type, ASTNode* return_type, ASTNode* body,
                                 bool is_exported, bool is_extern);

void ast_function_to_string(ASTNode* node, char* output, size_t max_length);
ASTNode* ast_parse_function(List* tokens, unsigned int* i);

#endif   // AST_FUNCTION_H