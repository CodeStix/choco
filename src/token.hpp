#pragma once

#include <string>
#include <list>
#include <iostream>
#include <vector>

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
    OPERATOR_DOUBLE_AND,
    OPERATOR_OR,
    OPERATOR_DOUBLE_OR,
    OPERATOR_EXCLAMATION,
    OPERATOR_CARET,
    OPERATOR_TILDE,
    OPERATOR_PERCENT,
    OPERATOR_DOUBLE_LT,
    OPERATOR_DOUBLE_GT,
    INTERFACE_KEYWORD,
    PACKED_KEYWORD,
    UNMANAGED_KEYWORD,
    AS_KEYWORD,
    VALUE_KEYWORD,
    PERIOD,
    STRUCT_KEYWORD,
    WHITESPACE,
    NEWLINE,
    OPERATOR_HASHTAG,
    OPERATOR_QUESTION_MARK,
};

class Token
{
public:
    Token(int position, TokenType type, std::string value) : position(position), type(type), value(value){};

    int position;
    TokenType type;
    std::string value;
};

class TokenStream
{
public:
    TokenStream(std::vector<const Token *> tokens) : tokens(tokens), position(0) {}

    int getPosition()
    {
        return this->position;
    }

    void setPosition(int p)
    {
        this->position = p;
    }

    bool isEndOfFile()
    {
        return this->position >= this->tokens.size();
    }

    const Token *next()
    {
        if (this->isEndOfFile())
        {
            std::cout << "FATAL: Token::next reached end of file\n";
            exit(-1);
            return NULL;
        }
        return this->tokens[this->position++];
    }

    int size()
    {
        return this->tokens.size();
    }

    const Token *peek()
    {
        if (this->isEndOfFile())
        {
            std::cout << "FATAL: Token::peek reached end of file\n";
            exit(-1);
            return NULL;
        }
        return this->tokens[this->position];
    }

    const Token *consume(TokenType type)
    {
        if (this->isEndOfFile())
        {
            std::cout << "FATAL: Token::consume reached end of file\n";
            exit(-1);
            return NULL;
        }
        const Token *tok = this->tokens[this->position];
        if (tok->type == type)
        {
            this->position++;
            return tok;
        }
        else
        {
            return NULL;
        }
    }

private:
    int position;
    std::vector<const Token *> tokens;
};

const char *getTokenTypeName(TokenType type);
int getTokenOperatorImportance(TokenType type);
void parseString(std::string &input, std::vector<const Token *> &tokenList);
