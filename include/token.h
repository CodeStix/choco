#ifndef TOKEN_H
#define TOKEN_H

#include "common/list.h"
#include <stdbool.h>

struct Token;
typedef struct Token Token;

struct SourceFile;
typedef struct SourceFile SourceFile;

enum TokenType
{
    TOKEN_SYMBOL,
    TOKEN_FUNC_KEYWORD,
    TOKEN_EXPORT_KEYWORD,
    TOKEN_EXTERN_KEYWORD,
    TOKEN_WHITESPACE,
    TOKEN_LITERAL_NUMBER,
    TOKEN_LITERAL_STRING,
    TOKEN_NEWLINE,
    TOKEN_OPERATOR,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_EQUAL,
    TOKEN_DOUBLE_EQUAL,
    TOKEN_SEMICOLON,
    TOKEN_COLON,
    TOKEN_BRACKET_OPEN,
    TOKEN_BRACKET_CLOSE,
    TOKEN_CURLY_BRACKET_OPEN,
    TOKEN_CURLY_BRACKET_CLOSE,
    TOKEN_SQUARE_BRACKET_OPEN,
    TOKEN_SQUARE_BRACKET_CLOSE,
    TOKEN_END_OF_FILE,
    TOKEN_GREATER,
    TOKEN_LESS,
    TOKEN_GREATER_EQUAL,
    TOKEN_LESS_EQUAL,
    TOKEN_NOT_EQUAL,
};
typedef enum TokenType TokenType;

SourceFile* sourcefile_malloc_read(char* source_path);
void sourcefile_free(SourceFile* src);
List* sourcefile_tokenize(SourceFile* src);
char* sourcefile_get_part_terminated(SourceFile* src, unsigned int i, unsigned int len);
void sourcefile_print(SourceFile* src, bool contents);
char* sourcefile_path(SourceFile* src);

TokenType token_type(Token* tok);
char* token_value(Token* tok, unsigned int* out_len);
unsigned int token_start_index(Token* tok);
unsigned int token_length(Token* tok);
Token* token_malloc(TokenType type, SourceFile* src, unsigned int start, unsigned int len);
void token_print(Token* tok);
void token_print_list(List* token_list);

char* tokentype_to_string(TokenType type);

#endif   // TOKEN_H
