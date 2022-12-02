#ifndef AST_FILE_H
#define AST_FILE_H

#include "ast.h"
#include "common/list.h"
#include <stdlib.h>

typedef struct ASTFile ASTFile;

ASTFile* ast_file_malloc(SourceFile* source, List* statements);
ASTNode* ast_parse_file(List* tokens, unsigned int* i, SourceFile* source);
void ast_file_print(ASTNode* node, bool verbose, unsigned int indent);

#endif   // AST_FILE_H