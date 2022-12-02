#include "ast.h"
#include "ast/block.h"
#include "ast/file.h"
#include "ast/function.h"
#include "ast/modifiers.h"
#include "ast/operator.h"
#include "ast/value.h"
#include "common/list.h"
#include "token.h"
#include <assert.h>
#include <llvm-c/Core.h>
#include <llvm-c/Types.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// struct TypedValue {
//     LLVMValueRef value;
// };
// typedef TypedValue* (*LLVMGeneratorFunc)(ASTNode* node);

struct ASTNode {
    ASTNodeType type;
    union {
        void* data;
        ASTFile* file_data;
        ASTFunction* function_data;
        ASTBlock* block_data;
        ASTDeclaration* decl_data;
        ASTOperator* operator_data;
        ASTUnaryOperator* unary_operator_data;
        ASTObject* object_data;
        ASTObjectField* object_field_data;
        ASTArray* array_data;
        ASTArraySegment* array_segment_data;
        ASTLiteralNumber* number_data;
        ASTLiteralString* string_data;
        ASTModifiers* modifiers_data;
    };
    unsigned int start_token;
    unsigned int end_token;
};

struct ASTPrintEntry {
    ASTNodeType type;
    void (*print)(ASTNode* node, bool verbose, unsigned int indent);
};
typedef struct ASTPrintEntry ASTPrintEntry;

static ASTPrintEntry node_to_string[] = {{AST_FILE, ast_file_print},
                                         {AST_FUNCTION, ast_function_print},
                                         {AST_BLOCK, ast_block_print},
                                         {AST_DECLARATION, ast_declaration_print},
                                         {AST_OPERATOR, ast_operator_print},
                                         {AST_UNARY_OPERATOR, ast_unary_operator_print},
                                         {AST_SYMBOL, ast_symbol_print},
                                         {AST_BRACKETS, ast_brackets_print},
                                         {AST_LITERAL_NUMBER, ast_literal_number_print},
                                         {AST_LITERAL_STRING, ast_literal_string_print},
                                         {AST_OBJECT, ast_object_print},
                                         {AST_OBJECT_FIELD, ast_object_field_print},
                                         {AST_ARRAY, ast_array_print},
                                         {AST_ARRAY_SEGMENT, ast_array_segment_print},
                                         {AST_MODIFIERS, ast_modifiers_print}};

inline Token* peek(List* tokens, unsigned int* i) {
    return list_get(tokens, *i);
}

inline Token* next(List* tokens, unsigned int* i) {
    return list_get(tokens, ++(*i));
}

inline void consume(List* tokens, unsigned int* i, TokenType t) {
    if (token_type(list_get(tokens, *i)) == t) {
        next(tokens, i);
    }
}

ASTNode* ast_node_malloc(ASTNodeType type, void* data, unsigned int start_token, unsigned int end_token) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = type;
    node->data = data;
    node->start_token = start_token;
    node->end_token = end_token;
    return node;
}

void ast_node_print(ASTNode* node, bool verbose, unsigned int indent) {
    for (int i = 0; i < sizeof(node_to_string) / sizeof(node_to_string[0]); i++) {
        ASTPrintEntry* entry = &node_to_string[i];
        if (entry->type == node->type) {
            entry->print(node, verbose, indent);
            return;
        }
    }

    printf("%*s(no print implementation for type %s)\n", indent, "", ast_node_type_to_string(ast_node_type(node)));
}

ASTNodeType ast_node_type(ASTNode* node) {
    return node->type;
}

void* ast_node_data(ASTNode* node) {
    return node->data;
}

void ast_node_set_data(ASTNode* node, void* new_data) {
    node->data = new_data;
}

void ast_node_free(ASTNode* node) {
    if (node->data != NULL) {
        free(node->data);
    }
    free(node);
}

ASTNode* ast_build(List* tokens, SourceFile* source) {
    unsigned int i = 0;
    return ast_parse_file(tokens, &i, source);
}

char* ast_node_type_to_string(ASTNodeType type) {
    switch (type) {
    case AST_FILE:
        return "AST_FILE";
    case AST_BLOCK:
        return "AST_BLOCK";
    case AST_FUNCTION:
        return "AST_FUNCTION";
    case AST_OPERATOR:
        return "AST_OPERATOR";
    case AST_UNARY_OPERATOR:
        return "AST_UNARY_OPERATOR";
    case AST_BRACKETS:
        return "AST_BRACKETS";
    case AST_LITERAL_NUMBER:
        return "AST_LITERAL_NUMBER";
    case AST_LITERAL_STRING:
        return "AST_LITERAL_STRING";
    case AST_SYMBOL:
        return "AST_SYMBOL";
    case AST_OBJECT:
        return "AST_OBJECT";
    case AST_OBJECT_FIELD:
        return "AST_OBJECT_FIELD";
    case AST_ARRAY:
        return "AST_ARRAY";
    case AST_ARRAY_SEGMENT:
        return "AST_ARRAY_SEGMENT";
    case AST_MODIFIERS:
        return "AST_MODIFIERS";
    case AST_DECLARATION:
        return "AST_DECLARATION";
    default:
        assert(false && "ast_node_type_to_string ASTNodeType is not implemented");
        break;
    }
}
