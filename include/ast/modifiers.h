#ifndef AST_MODIFIERS_H
#define AST_MODIFIERS_H

#include "ast.h"
#include <stdbool.h>

typedef struct ASTModifiers ASTModifiers;

typedef enum Modifiers Modifiers;
enum Modifiers
{
    MODIFIER_NONE = 0,
    MODIFIER_EXPORT = (1 << 0),
    MODIFIER_EXTERN = (1 << 1),
    MODIFIER_UNMANAGED = (1 << 2),
    MODIFIER_PACKED = (1 << 3),
    MODIFIER_VALUE = (1 << 4),
};

char* modifier_to_string(Modifiers modifier);
bool modifier_has(Modifiers value, Modifiers has);

bool ast_modifiers_has(ASTNode* modifiers_node, Modifiers has);
ASTModifiers* ast_modifiers_malloc(Modifiers modifiers);
ASTNode* ast_parse_modifiers(List* tokens, unsigned int* i);

#endif   // AST_MODIFIERS_H