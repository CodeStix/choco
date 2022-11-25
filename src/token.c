#include "token.h"
#include "common/list.h"
#include "util.h"
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct SourceFile {
    char* source_path;
    char* contents;
    unsigned int length;
};

struct Token {
    TokenType type;
    SourceFile* source;
    unsigned int start;
    unsigned int length;
};

SourceFile* sourcefile_malloc_read(char* source_path) {
    unsigned long file_size;
    char* file_contents = file_read_all_contents(source_path, &file_size);

    SourceFile* src = (SourceFile*)malloc(sizeof(SourceFile));
    src->contents = file_contents;
    src->length = (unsigned int)file_size;
    src->source_path = source_path;
    return src;
}

void sourcefile_free(SourceFile* src) {
    free(src->contents);
    free(src);
}

Token* sourcefile_read_whitespace(SourceFile* src, unsigned int* i) {
    unsigned int start = *i, len = 0;
    while (true) {
        char c = src->contents[*i];

        assert(c != '\0' && "Reached end of file");

        if (isspace(c)) {
            len++;
        } else {
            return token_malloc(TOKEN_WHITESPACE, src, start, len);
        }

        (*i)++;
    }
}

Token* sourcefile_read_literal_number(SourceFile* src, unsigned int* i) {
    unsigned int start = *i, len = 0;
    while (true) {
        char c = src->contents[*i];

        assert(c != '\0' && "Reached end of file");

        if (isalnum(c) || c == '.' || c == '_') {
            len++;
        } else {
            return token_malloc(TOKEN_LITERAL_NUMBER, src, start, len);
        }

        (*i)++;
    }
}

char* sourcefile_path(SourceFile* src) {
    return src->source_path;
}

Token* sourcefile_read_symbol(SourceFile* src, unsigned int* i) {
    unsigned int start = *i, len = 0;
    while (true) {
        char c = src->contents[*i];

        assert(c != '\0' && "Reached end of file");

        if (isalnum(c) || c == '_') {
            len++;
        } else {
            Token* tok = token_malloc(TOKEN_SYMBOL, src, start, len);
            char* token_str = &tok->source->contents[tok->start];

            if (strncmp(token_str, "func", tok->length) == 0) {
                tok->type = TOKEN_FUNC_KEYWORD;
            } else if (strncmp(token_str, "export", tok->length) == 0) {
                tok->type = TOKEN_EXPORT_KEYWORD;
            } else if (strncmp(token_str, "extern", tok->length) == 0) {
                tok->type = TOKEN_EXTERN_KEYWORD;
            }

            return tok;
        }

        (*i)++;
    }
}

Token* sourcefile_read_literal_string(SourceFile* src, unsigned int* i) {
    assert(src->contents[(*i)++] == '"');   // Consume first "

    unsigned int start = *i, len = 0;
    while (true) {
        char c = src->contents[*i];

        assert(c != '\0' && "Reached end of file");

        if (c == '"') {
            (*i)++;   // Consume last "
            return token_malloc(TOKEN_LITERAL_STRING, src, start, len);
        } else {
            len++;
        }

        (*i)++;
    }
}

Token* sourcefile_read_operator(SourceFile* src, unsigned int* i) {
    unsigned int start = *i, len = 0;
    while (true) {
        char c = src->contents[*i];

        assert(c != '\0' && "Reached end of file");

        if (is_operator(c)) {
            len++;
        } else {
            Token* tok = token_malloc(TOKEN_OPERATOR, src, start, len);
            char* token_str = &tok->source->contents[tok->start];

            if (strncmp(token_str, "+", tok->length) == 0) {
                tok->type = TOKEN_PLUS;
            } else if (strncmp(token_str, "-", tok->length) == 0) {
                tok->type = TOKEN_MINUS;
            } else if (strncmp(token_str, "*", tok->length) == 0) {
                tok->type = TOKEN_STAR;
            } else if (strncmp(token_str, "/", tok->length) == 0) {
                tok->type = TOKEN_SLASH;
            } else if (strncmp(token_str, "=", tok->length) == 0) {
                tok->type = TOKEN_EQUAL;
            } else if (strncmp(token_str, "==", tok->length) == 0) {
                tok->type = TOKEN_DOUBLE_EQUAL;
            } else if (strncmp(token_str, ">", tok->length) == 0) {
                tok->type = TOKEN_GREATER;
            } else if (strncmp(token_str, ">=", tok->length) == 0) {
                tok->type = TOKEN_GREATER_EQUAL;
            } else if (strncmp(token_str, "<", tok->length) == 0) {
                tok->type = TOKEN_LESS;
            } else if (strncmp(token_str, "<=", tok->length) == 0) {
                tok->type = TOKEN_LESS_EQUAL;
            } else if (strncmp(token_str, "!=", tok->length) == 0) {
                tok->type = TOKEN_NOT_EQUAL;
            } else {
                printf("Warning: unimplemented token for operator '%.*s'\n", tok->length, token_str);
            }

            return tok;
        }

        (*i)++;
    }
}

List* sourcefile_tokenize(SourceFile* src) {
    assert(src->contents != NULL);

    List* token_list = list_malloc(128);

    unsigned int i = 0;
    while (1) {
        char c = src->contents[i];
        if (c == '\0') {
            // Reached end of file
            break;
        }

        if (isspace(c)) {
            list_add(token_list, sourcefile_read_whitespace(src, &i));
        } else if (isdigit(c) || c == '.') {
            list_add(token_list, sourcefile_read_literal_number(src, &i));
        } else if (isalpha(c)) {
            list_add(token_list, sourcefile_read_symbol(src, &i));
        } else if (c == '"') {
            list_add(token_list, sourcefile_read_literal_string(src, &i));
        } else if (is_operator(c)) {
            list_add(token_list, sourcefile_read_operator(src, &i));
        } else if (c == ';') {
            list_add(token_list, token_malloc(TOKEN_SEMICOLON, src, i++, 1));
        } else if (c == ':') {
            list_add(token_list, token_malloc(TOKEN_COLON, src, i++, 1));
        } else if (c == '(') {
            list_add(token_list, token_malloc(TOKEN_BRACKET_OPEN, src, i++, 1));
        } else if (c == ')') {
            list_add(token_list, token_malloc(TOKEN_BRACKET_CLOSE, src, i++, 1));
        } else if (c == '[') {
            list_add(token_list, token_malloc(TOKEN_SQUARE_BRACKET_OPEN, src, i++, 1));
        } else if (c == ']') {
            list_add(token_list, token_malloc(TOKEN_SQUARE_BRACKET_CLOSE, src, i++, 1));
        } else if (c == '{') {
            list_add(token_list, token_malloc(TOKEN_CURLY_BRACKET_OPEN, src, i++, 1));
        } else if (c == '}') {
            list_add(token_list, token_malloc(TOKEN_CURLY_BRACKET_CLOSE, src, i++, 1));
        } else {
            printf("Invalid char: %c\n", c);
            assert(false && "Invalid character found");
        }
    }

    list_add(token_list, token_malloc(TOKEN_END_OF_FILE, src, i++, 1));

    return token_list;
}

char* sourcefile_get_part_terminated(SourceFile* src, unsigned int i, unsigned int len) {
    char* str = malloc(len + 1);   // +1 For \0
    strncpy(str, &src->contents[i], len);
    str[len] = '\0';
    return str;
}

void sourcefile_print(SourceFile* src, bool contents) {
    printf("SourceFile { len=%u, src=%s, content=\"%s\" }\n", src->length, src->source_path,
           contents ? src->contents : "...");
}

TokenType token_type(Token* tok) {
    return tok->type;
}

char* token_value(Token* tok, unsigned int* out_len) {
    *out_len = tok->length;
    return &tok->source->contents[tok->start];
}

unsigned int token_start_index(Token* tok) {
    return tok->start;
}

unsigned int token_length(Token* tok) {
    return tok->length;
}

Token* token_malloc(TokenType type, SourceFile* src, unsigned int start, unsigned int len) {
    Token* tok = (Token*)malloc(sizeof(Token));
    tok->type = type;
    tok->source = src;
    tok->start = start;
    tok->length = len;
    return tok;
}

void token_print(Token* tok) {
    printf("Token { type=%s, start=%u, len=%u, src=%s value=\"%.*s\" }\n", tokentype_to_string(tok->type), tok->start,
           tok->length, tok->source->source_path, tok->length, &tok->source->contents[tok->start]);
}

void token_print_list(List* token_list) {
    unsigned int len = list_length(token_list);

    printf("List of %u tokens:\n", len);

    if (len == 0) {
        printf("\t(no tokens)\n");
        return;
    }

    for (unsigned int i = 0; i < len; i++) {
        Token* tok = (Token*)list_get(token_list, i);
        token_print(tok);
    }
}

char* tokentype_to_string(TokenType type) {
    switch (type) {
    case TOKEN_SYMBOL:
        return "TOKEN_SYMBOL";
    case TOKEN_FUNC_KEYWORD:
        return "TOKEN_FUNC_KEYWORD";
    case TOKEN_WHITESPACE:
        return "TOKEN_WHITESPACE";
    case TOKEN_LITERAL_NUMBER:
        return "TOKEN_LITERAL_NUMBER";
    case TOKEN_LITERAL_STRING:
        return "TOKEN_LITERAL_STRING";
    case TOKEN_NEWLINE:
        return "TOKEN_NEWLINE";
    case TOKEN_OPERATOR:
        return "TOKEN_OPERATOR";
    case TOKEN_PLUS:
        return "TOKEN_PLUS";
    case TOKEN_MINUS:
        return "TOKEN_MINUS";
    case TOKEN_STAR:
        return "TOKEN_STAR";
    case TOKEN_SLASH:
        return "TOKEN_SLASH";
    case TOKEN_EQUAL:
        return "TOKEN_EQUAL";
    case TOKEN_DOUBLE_EQUAL:
        return "TOKEN_DOUBLE_EQUAL";
    case TOKEN_COLON:
        return "TOKEN_COLON";
    case TOKEN_SEMICOLON:
        return "TOKEN_SEMICOLON";
    case TOKEN_BRACKET_OPEN:
        return "TOKEN_BRACKET_OPEN";
    case TOKEN_BRACKET_CLOSE:
        return "TOKEN_BRACKET_CLOSE";
    case TOKEN_CURLY_BRACKET_OPEN:
        return "TOKEN_CURLY_BRACKET_OPEN";
    case TOKEN_CURLY_BRACKET_CLOSE:
        return "TOKEN_CURLY_BRACKET_CLOSE";
    case TOKEN_SQUARE_BRACKET_OPEN:
        return "TOKEN_SQUARE_BRACKET_OPEN";
    case TOKEN_SQUARE_BRACKET_CLOSE:
        return "TOKEN_SQUARE_BRACKET_CLOSE";
    case TOKEN_END_OF_FILE:
        return "TOKEN_END_OF_FILE";
    case TOKEN_GREATER:
        return "TOKEN_GREATER";
    case TOKEN_LESS:
        return "TOKEN_LESS";
    case TOKEN_GREATER_EQUAL:
        return "TOKEN_GREATER_EQUAL";
    case TOKEN_LESS_EQUAL:
        return "TOKEN_LESS_EQUAL";
    case TOKEN_NOT_EQUAL:
        return "TOKEN_NOT_EQUAL";
    default:
        assert(false && "tokentype_to_string TokenType is not implemented");
        break;
    }
}