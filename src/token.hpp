#pragma once

#include <string>
#include <list>
#include <iostream>

enum class TokenType
{
    SYMBOL,
    FUNC_KEYWORD,
    RETURN_KEYWORD,
    EXTERN_KEYWORD,
    LET_KEYWORD,
    CONST_KEYWORD,
    LITERAL_STRING,
    LITERAL_CHAR,
    LITERAL_NUMBER,
    BRACKET_OPEN,
    BRACKET_CLOSE,
    SQUARE_BRACKET_OPEN,
    SQUARE_BRACKET_CLOSE,
    CURLY_BRACKET_OPEN,
    CURLY_BRACKET_CLOSE,
    ASSIGNMENT_OPERATOR,
    ADDITION_OPERATOR,
    SUBSTRACTION_OPERATOR,
    DIVISION_OPERATOR,
    MULTIPLICATION_OPERATOR,
    LT_OPERATOR,
    GT_OPERATOR,
    COMMA,
    IF_KEYWORD,
    ELSE_KEYWORD,
    FOR_KEYWORD,
    GOTO_KEYWORD,
    WHILE_KEYWORD,
    EXPORT_KEYWORD,
};

class Token
{
public:
    Token(int position, TokenType type, std::string value) : position(position), type(type), value(value){};

    int position;
    TokenType type;
    std::string value;
};

const char *getTokenTypeName(TokenType type);
int getTokenOperatorImportance(TokenType type);
void parseString(std::string &input, std::list<const Token *> &tokenList);