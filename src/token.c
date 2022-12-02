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

        if (isspace(c) && c != '\0') {
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

bool token_string_matches(Token* tok, char* cmp) {
    if (tok->length != strlen(cmp)) {
        return false;
    }

    char* token_str = &tok->source->contents[tok->start];
    return strncmp(token_str, cmp, tok->length) == 0;
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

            if (token_string_matches(tok, "func")) {
                tok->type = TOKEN_FUNC_KEYWORD;
            } else if (token_string_matches(tok, "export")) {
                tok->type = TOKEN_EXPORT_KEYWORD;
            } else if (token_string_matches(tok, "extern")) {
                tok->type = TOKEN_EXTERN_KEYWORD;
            } else if (token_string_matches(tok, "if")) {
                tok->type = TOKEN_IF_KEYWORD;
            } else if (token_string_matches(tok, "while")) {
                tok->type = TOKEN_WHILE_KEYWORD;
            } else if (token_string_matches(tok, "struct")) {
                tok->type = TOKEN_STRUCT_KEYWORD;
            } else if (token_string_matches(tok, "let")) {
                tok->type = TOKEN_LET_KEYWORD;
            } else if (token_string_matches(tok, "const")) {
                tok->type = TOKEN_CONST_KEYWORD;
            } else if (token_string_matches(tok, "break")) {
                tok->type = TOKEN_BREAK_KEYWORD;
            } else if (token_string_matches(tok, "return")) {
                tok->type = TOKEN_RETURN_KEYWORD;
            } else if (token_string_matches(tok, "as")) {
                tok->type = TOKEN_AS_KEYWORD;
            } else if (token_string_matches(tok, "panic")) {
                tok->type = TOKEN_PANIC_KEYWORD;
            } else if (token_string_matches(tok, "unmanaged")) {
                tok->type = TOKEN_UNMANAGED_KEYWORD;
            } else if (token_string_matches(tok, "value")) {
                tok->type = TOKEN_VALUE_KEYWORD;
            } else if (token_string_matches(tok, "packed")) {
                tok->type = TOKEN_PACKED_KEYWORD;
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

            if (token_string_matches(tok, "+")) {
                tok->type = TOKEN_PLUS;
            } else if (token_string_matches(tok, "-")) {
                tok->type = TOKEN_MINUS;
            } else if (token_string_matches(tok, "*")) {
                tok->type = TOKEN_STAR;
            } else if (token_string_matches(tok, "/")) {
                tok->type = TOKEN_SLASH;
            } else if (token_string_matches(tok, "=")) {
                tok->type = TOKEN_EQUAL;
            } else if (token_string_matches(tok, "==")) {
                tok->type = TOKEN_DOUBLE_EQUAL;
            } else if (token_string_matches(tok, ">")) {
                tok->type = TOKEN_GREATER;
            } else if (token_string_matches(tok, ">=")) {
                tok->type = TOKEN_GREATER_EQUAL;
            } else if (token_string_matches(tok, "<")) {
                tok->type = TOKEN_LESS;
            } else if (token_string_matches(tok, "<=")) {
                tok->type = TOKEN_LESS_EQUAL;
            } else if (token_string_matches(tok, "!=")) {
                tok->type = TOKEN_NOT_EQUAL;
            } else if (token_string_matches(tok, "?")) {
                tok->type = TOKEN_QUESTION;
            } else if (token_string_matches(tok, "!")) {
                tok->type = TOKEN_EXCLAMATION;
            } else if (token_string_matches(tok, "!")) {
                tok->type = TOKEN_HASHTAG;
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
        if (i >= src->length) {
            // Reached end of file
            break;
        }

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
        } else if (c == '.') {
            list_add(token_list, token_malloc(TOKEN_PERIOD, src, i++, 1));
        } else if (c == ',') {
            list_add(token_list, token_malloc(TOKEN_COMMA, src, i++, 1));
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

SourceFile* token_source(Token* tok) {
    return tok->source;
}

unsigned int token_start_index(Token* tok) {
    return tok->start;
}

unsigned int token_length(Token* tok) {
    return tok->length;
}

void token_highlight(Token* start_tok, Token* end_tok) {
    if (end_tok != NULL) {
        assert(strcmp(start_tok->source->source_path, end_tok->source->source_path) == 0);
    } else {
        end_tok = start_tok;
    }

    SourceFile* src = start_tok->source;
    printf("In %s:\n", src->source_path);
    sourcefile_highlight(src, start_tok->start, end_tok->start + end_tok->length);
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

void sourcefile_line_offset_at(SourceFile* src, unsigned int index, unsigned int* out_line, unsigned int* out_offset) {
    *out_line = 0;
    *out_offset = 0;
    for (unsigned int i = 0; i < index; i++) {
        if (src->contents[i] == '\n') {
            (*out_line)++;
            *out_offset = 0;
        } else {
            (*out_offset)++;
        }
    }
}

char* sourcefile_get_line(SourceFile* src, unsigned int line_index, unsigned int* out_length) {
    unsigned int line_start_offset = 0;
    unsigned int line = 0;

    for (unsigned int i = 0; i < src->length; i++) {
        if (src->contents[i] == '\n' || src->contents[i] == '\0') {
            if (line == line_index) {
                *out_length = i - line_start_offset;
                return &src->contents[line_start_offset];
            }

            line++;

            if (line == line_index) {
                line_start_offset = i + 1;
            }
        }
    }

    return NULL;
}

void sourcefile_highlight(SourceFile* src, unsigned int start_index, unsigned int end_index) {

    assert(end_index >= start_index);

    unsigned int start_line, end_line;
    unsigned int start_offset, end_offset;
    sourcefile_line_offset_at(src, start_index, &start_line, &start_offset);
    sourcefile_line_offset_at(src, end_index, &end_line, &end_offset);

    assert(end_line >= start_line || end_offset >= start_offset);

    const int EXTRA_LINES = 2;

    for (int i = (int)start_line - EXTRA_LINES; i <= (int)end_line + EXTRA_LINES; i++) {
        unsigned int line_len;
        char* line_str = sourcefile_get_line(src, i, &line_len);
        if (line_str == NULL) {
            continue;
        }

        printf("%4d: %.*s\n", i + 1, line_len, line_str);

        if (i >= start_line && i <= end_line) {
            printf("      ");
            unsigned int start = i == start_line ? start_offset : 0;
            unsigned int end = i == end_line ? end_offset : line_len;
            for (unsigned int j = 0; j < line_len; j++) {
                putchar(j >= start && j < end ? '~' : ' ');
            }
            putchar('\n');
        }
    }
}

char* tokentype_to_string(TokenType type) {
    switch (type) {
    case TOKEN_SYMBOL:
        return "TOKEN_SYMBOL";
    case TOKEN_FUNC_KEYWORD:
        return "TOKEN_FUNC_KEYWORD";
    case TOKEN_EXPORT_KEYWORD:
        return "TOKEN_EXPORT_KEYWORD";
    case TOKEN_EXTERN_KEYWORD:
        return "TOKEN_EXTERN_KEYWORD";
    case TOKEN_WHITESPACE:
        return "TOKEN_WHITESPACE";
    case TOKEN_LET_KEYWORD:
        return "TOKEN_LET_KEYWORD";
    case TOKEN_CONST_KEYWORD:
        return "TOKEN_CONST_KEYWORD";
    case TOKEN_RETURN_KEYWORD:
        return "TOKEN_RETURN_KEYWORD";
    case TOKEN_BREAK_KEYWORD:
        return "TOKEN_BREAK_KEYWORD";
    case TOKEN_IF_KEYWORD:
        return "TOKEN_IF_KEYWORD";
    case TOKEN_WHILE_KEYWORD:
        return "TOKEN_WHILE_KEYWORD";
    case TOKEN_STRUCT_KEYWORD:
        return "TOKEN_STRUCT_KEYWORD";
    case TOKEN_AS_KEYWORD:
        return "TOKEN_AS_KEYWORD";
    case TOKEN_PANIC_KEYWORD:
        return "TOKEN_PANIC_KEYWORD";
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
    case TOKEN_EXCLAMATION:
        return "TOKEN_EXCLAMATION";
    case TOKEN_QUESTION:
        return "TOKEN_QUESTION";
    case TOKEN_PERIOD:
        return "TOKEN_PERIOD";
    case TOKEN_COMMA:
        return "TOKEN_COMMA";
    case TOKEN_HASHTAG:
        return "TOKEN_HASHTAG";
    case TOKEN_VALUE_KEYWORD:
        return "TOKEN_VALUE_KEYWORD";
    case TOKEN_PACKED_KEYWORD:
        return "TOKEN_PACKED_KEYWORD";
    case TOKEN_UNMANAGED_KEYWORD:
        return "TOKEN_UNMANAGED_KEYWORD";
    default:
        assert(false && "tokentype_to_string TokenType is not implemented");
        break;
    }
}