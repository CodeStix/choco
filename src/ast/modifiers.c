#include "ast/modifiers.h"
#include "ast.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

struct ASTModifiers {
    Modifiers modifiers;
};

bool modifier_has(Modifiers value, Modifiers has) {
    return (value & has) == has;
}

bool ast_modifiers_has(ASTNode* modifiers_node, Modifiers has) {
    assert(ast_node_type(modifiers_node) == AST_MODIFIERS);
    ASTModifiers* m = (ASTModifiers*)ast_node_data(modifiers_node);
    return modifier_has(m->modifiers, has);
}

ASTModifiers* ast_modifiers_malloc(Modifiers modifiers) {
    ASTModifiers* m = (ASTModifiers*)malloc(sizeof(ASTModifiers));
    m->modifiers = modifiers;
    return m;
}

ASTNode* ast_parse_modifiers(List* tokens, unsigned int* i) {
    unsigned int start_token = *i;

    Modifiers modifiers = MODIFIER_NONE;
    while (1) {
        Token* tok = peek(tokens, i);

        Modifiers new_modifier = MODIFIER_NONE;
        switch (token_type(tok)) {
        case TOKEN_UNMANAGED_KEYWORD:
            new_modifier = MODIFIER_UNMANAGED;
            break;
        case TOKEN_PACKED_KEYWORD:
            new_modifier = MODIFIER_PACKED;
            break;
        case TOKEN_VALUE_KEYWORD:
            new_modifier = MODIFIER_VALUE;
            break;
        case TOKEN_EXPORT_KEYWORD:
            new_modifier = MODIFIER_EXPORT;
            break;
        case TOKEN_EXTERN_KEYWORD:
            new_modifier = MODIFIER_EXTERN;
            break;

        default:
            new_modifier = MODIFIER_NONE;
            break;
        }

        if (new_modifier == MODIFIER_NONE) {
            break;
        }

        if (modifier_has(modifiers, new_modifier)) {
            printf("Warning: duplicate modifier\n");
            token_highlight(tok, tok);
        }

        modifiers |= new_modifier;

        next(tokens, i);
        consume(tokens, i, TOKEN_WHITESPACE);
    }

    if (modifiers == MODIFIER_NONE) {
        return NULL;
    }

    return ast_node_malloc(AST_MODIFIERS, ast_modifiers_malloc(modifiers), start_token, *i);
}

char* modifier_to_string(Modifiers modifier) {
    switch (modifier) {
    case MODIFIER_NONE:
        return "MODIFIER_NONE";
    case MODIFIER_EXPORT:
        return "MODIFIER_EXPORT";
    case MODIFIER_EXTERN:
        return "MODIFIER_EXTERN";
    case MODIFIER_UNMANAGED:
        return "MODIFIER_UNMANAGED";
    case MODIFIER_PACKED:
        return "MODIFIER_PACKED";
    case MODIFIER_VALUE:
        return "MODIFIER_VALUE";
    default:
        assert(false && "modifier_to_string missing entry");
        return NULL;
    }
}
