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

struct ASTNodeToStringEntry {
    ASTNodeType type;
    void (*to_string)(ASTNode* node, char* output, size_t max_length);
};
typedef struct ASTNodeToStringEntry ASTNodeToStringEntry;

static ASTNodeToStringEntry node_to_string[] = {{AST_FILE, ast_file_to_string}, {AST_FUNCTION, ast_function_to_string}};

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

bool ast_node_to_string(ASTNode* node, char* output, size_t max_length) {
    for (int i = 0; i < sizeof(node_to_string) / sizeof(node_to_string[0]); i++) {
        ASTNodeToStringEntry* entry = &node_to_string[i];
        if (entry->type == node->type) {
            entry->to_string(node, output, max_length);
            return true;
        }
    }

    printf("Warning: no to_string implementation for type %s\n", ast_node_type_to_string(node->type));
    return false;
}

void ast_node_print(ASTNode* node) {
    char buff[250];
    assert(ast_node_to_string(node, buff, sizeof(buff)) == true);
    puts(buff);
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
    default:
        assert(false && "ast_node_type_to_string ASTNodeType is not implemented");
        break;
    }
}
