#ifndef AST_FILE_H
#define AST_FILE_H

#include "ast.h"
#include "common/list.h"
#include <stdlib.h>

typedef struct ASTFile ASTFile;

ASTFile* ast_file_malloc(SourceFile* source, List* statements);
ASTNode* ast_parse_file(List* tokens, unsigned int* i, SourceFile* source);
void ast_file_to_string(ASTNode* node, char* output, size_t max_length);

#endif   // AST_FILE_H