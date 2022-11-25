#include "ast/file.h"
#include "ast/function.h"
#include "common/list.h"
#include "token.h"
#include <assert.h>
#include <stdio.h>

struct ASTFile {
    SourceFile* source;
    List* statements;
};

ASTFile* ast_file_malloc(SourceFile* source, List* statements) {
    ASTFile* f = malloc(sizeof(ASTFile));
    f->source = source;
    f->statements = statements;
    return f;
}

void ast_file_to_string(ASTNode* node, char* output, size_t max_length) {
    ASTFile* file = (ASTFile*)ast_node_data(node);

    snprintf(output, max_length, "File { path=%s, statements=%u }", sourcefile_path(file->source),
             list_length(file->statements));
}

ASTNode* ast_parse_file(List* tokens, unsigned int* i, SourceFile* source) {
    List* statements = list_malloc(16);

    while (true) {
        Token* tok = list_get(tokens, *i);
        TokenType type = token_type(tok);
        if (type == TOKEN_END_OF_FILE) {
            break;
        }

        ASTNode* file_statement = NULL;
        if (type == TOKEN_FUNC_KEYWORD || type == TOKEN_EXPORT_KEYWORD || type == TOKEN_EXTERN_KEYWORD) {
            file_statement = ast_parse_function(tokens, i);
        } else {
            printf("Error: unexpected %s in file at index %u\n", tokentype_to_string(type), *i);
            (*i)++;
            continue;
        }

        assert(file_statement != NULL);
        list_add(statements, file_statement);
    }

    return ast_node_malloc(AST_FILE, ast_file_malloc(source, statements), 0, *i);
}
