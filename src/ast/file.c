#include "ast/file.h"
#include "ast/function.h"
#include "common/list.h"
#include "token.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

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

void ast_file_print(ASTNode* node, bool verbose, unsigned int indent) {
    ASTFile* file = (ASTFile*)ast_node_data(node);

    printf("%*sFile { path=\"%s\", statements=%u }\n", indent, "", sourcefile_path(file->source),
           list_length(file->statements));

    if (verbose) {
        for (unsigned int i = 0; i < list_length(file->statements); i++) {
            ASTNode* n = list_get(file->statements, i);
            ast_node_print(n, true, indent + 2);
        }
    }
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
        if (type == TOKEN_WHITESPACE) {
            (*i)++;
            continue;
        } else if (type == TOKEN_FUNC_KEYWORD || type == TOKEN_EXPORT_KEYWORD || type == TOKEN_EXTERN_KEYWORD) {
            file_statement = ast_parse_function(tokens, i);
        } else {
            printf("Invalid file statement\n");
            token_highlight(tok, tok);
            assert(false && "Unexpected file statement");
            return NULL;
        }

        assert(file_statement != NULL);
        list_add(statements, file_statement);
    }

    return ast_node_malloc(AST_FILE, ast_file_malloc(source, statements), 0, *i);
}
