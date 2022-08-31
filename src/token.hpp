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
    OPERATOR_ASSIGNMENT,
    OPERATOR_ADDITION,
    OPERATOR_SUBSTRACTION,
    OPERATOR_DIVISION,
    OPERATOR_MULTIPLICATION,
    OPERATOR_LT,
    OPERATOR_GT,
    COMMA,
    IF_KEYWORD,
    ELSE_KEYWORD,
    FOR_KEYWORD,
    GOTO_KEYWORD,
    WHILE_KEYWORD,
    EXPORT_KEYWORD,
    COLON,
    SEMICOLON,
    OPERATOR_EQUALS,
    OPERATOR_NOT_EQUALS,
    OPERATOR_LTE,
    OPERATOR_GTE,
    OPERATOR_AND,
    OPERATOR_OR,
    OPERATOR_NOT,
    OPERATOR_XOR,
    OPERATOR_TILDE,
    OPERATOR_PERCENT,
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