#ifndef AST_H
#define AST_H

#include "common/list.h"
#include "token.h"
#include <stdbool.h>
#include <stdlib.h>

enum ASTNodeType
{
    AST_FILE,
    AST_BLOCK,
    AST_FUNCTION,
    AST_OPERATOR,
    AST_UNARY_OPERATOR,
    AST_BRACKETS,
    AST_LITERAL_NUMBER,
    AST_LITERAL_STRING,
    AST_SYMBOL,
    AST_OBJECT,
    AST_OBJECT_FIELD,
    AST_ARRAY,
    AST_ARRAY_SEGMENT,
    AST_MODIFIERS,
    AST_DECLARATION,
};

typedef enum ASTNodeType ASTNodeType;

typedef struct ASTNode ASTNode;

typedef struct TypedValue TypedValue;

// Quick and short utility functions
Token* peek(List* tokens, unsigned int* i);
Token* next(List* tokens, unsigned int* i);
void consume(List* tokens, unsigned int* i, TokenType t);

ASTNode* ast_build(List* tokens, SourceFile* source);
ASTNode* ast_node_malloc(ASTNodeType type, void* data, unsigned int start_token, unsigned int end_token);
ASTNodeType ast_node_type(ASTNode* node);
void* ast_node_data(ASTNode* node);
void ast_node_set_data(ASTNode* node, void* new_data);
void ast_node_free(ASTNode* node);
bool ast_node_to_string(ASTNode* node, char* output, size_t max_length);
void ast_node_print(ASTNode* node);

char* ast_node_type_to_string(ASTNodeType type);

#endif   // AST_H