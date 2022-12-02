#ifndef AST_VALUE_H
#define AST_VALUE_H

#include "ast.h"

typedef struct ASTBrackets ASTBrackets;
typedef struct ASTLiteralNumber ASTLiteralNumber;
typedef struct ASTLiteralString ASTLiteralString;
typedef struct ASTSymbol ASTSymbol;

typedef struct ASTObjectField ASTObjectField;
typedef struct ASTObject ASTObject;

typedef struct ASTArraySegment ASTArraySegment;
typedef struct ASTArray ASTArray;

ASTNode* ast_parse_value(List* tokens, unsigned int* i);
ASTNode* ast_parse_brackets(List* tokens, unsigned int* i);

#endif   // AST_VALUE_H