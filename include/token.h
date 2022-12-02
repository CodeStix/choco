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
    TOKEN_LET_KEYWORD,
    TOKEN_CONST_KEYWORD,
    TOKEN_RETURN_KEYWORD,
    TOKEN_BREAK_KEYWORD,
    TOKEN_IF_KEYWORD,
    TOKEN_WHILE_KEYWORD,
    TOKEN_STRUCT_KEYWORD,
    TOKEN_AS_KEYWORD,
    TOKEN_PANIC_KEYWORD,
    TOKEN_VALUE_KEYWORD,
    TOKEN_PACKED_KEYWORD,
    TOKEN_UNMANAGED_KEYWORD,
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
    TOKEN_EXCLAMATION,
    TOKEN_QUESTION,
    TOKEN_PERIOD,
    TOKEN_COMMA,
    TOKEN_HASHTAG
};
typedef enum TokenType TokenType;

SourceFile* sourcefile_malloc_read(char* source_path);
void sourcefile_free(SourceFile* src);
List* sourcefile_tokenize(SourceFile* src);
char* sourcefile_get_part_terminated(SourceFile* src, unsigned int i, unsigned int len);
void sourcefile_print(SourceFile* src, bool contents);
char* sourcefile_path(SourceFile* src);
void sourcefile_line_offset_at(SourceFile* src, unsigned int index, unsigned int* out_line, unsigned int* out_offset);
char* sourcefile_get_line(SourceFile* src, unsigned int line_index, unsigned int* out_length);
void sourcefile_highlight(SourceFile* src, unsigned int start_index, unsigned int end_index);

TokenType token_type(Token* tok);
char* token_value(Token* tok, unsigned int* out_len);
unsigned int token_start_index(Token* tok);
unsigned int token_length(Token* tok);
Token* token_malloc(TokenType type, SourceFile* src, unsigned int start, unsigned int len);
void token_print(Token* tok);
void token_print_list(List* token_list);
SourceFile* token_source(Token* tok);
void token_highlight(Token* start_tok, Token* end_tok_or_null);

char* tokentype_to_string(TokenType type);

#endif   // TOKEN_H
