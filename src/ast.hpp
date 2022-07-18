#pragma once

#include <iostream>
#include <list>
#include "token.hpp"

enum class ASTNodeType
{
    OPERATOR,
    LITERAL_NUMBER,
    LITERAL_STRING,
    FUNCTION,
    DECLARATION,
    ASSIGNMENT,
};

class ASTNode
{
public:
    ASTNode(ASTNodeType type) : type(type) {}
    ASTNodeType type;

    virtual std::string toString() = 0;
};

class ASTOperator : public ASTNode
{
public:
    ASTOperator(const Token *operatorToken, ASTNode *left, ASTNode *right) : ASTNode(ASTNodeType::OPERATOR), operatorToken(operatorToken), left(left), right(right) {}
    const Token *operatorToken;
    ASTNode *left;
    ASTNode *right;

    virtual std::string toString()
    {
        std::string str = "(";
        str += this->left->toString();
        str += " ";
        str += this->operatorToken->value;
        str += " ";
        str += this->right->toString();
        str += ")";
        return str;
    }
};

class ASTBrackets : public ASTNode
{
public:
    ASTBrackets(ASTNode *inner) : ASTNode(ASTNodeType::OPERATOR), inner(inner) {}
    ASTNode *inner;

    virtual std::string toString()
    {
        std::string str = "(";
        str += this->inner->toString();
        str += ")";
        return str;
    }
};

class ASTLiteralString : public ASTNode
{
public:
    ASTLiteralString(const Token *value) : ASTNode(ASTNodeType::LITERAL_STRING), valueToken(value) {}
    const Token *valueToken;

    virtual std::string toString()
    {
        std::string str = "\"";
        str += this->valueToken->value;
        str += "\"";
        return str;
    }
};

class ASTLiteralNumber : public ASTNode
{
public:
    ASTLiteralNumber(const Token *value) : ASTNode(ASTNodeType::LITERAL_NUMBER), valueToken(value) {}
    const Token *valueToken;

    virtual std::string toString()
    {
        std::string str = this->valueToken->value;
        return str;
    }
};

class ASTDeclaration : public ASTNode
{
public:
    ASTDeclaration(const Token *nameToken, ASTNode *value) : ASTNode(ASTNodeType::DECLARATION), nameToken(nameToken), value(value) {}
    const Token *nameToken;
    ASTNode *value;

    virtual std::string toString()
    {
        std::string str = "const ";
        str += this->nameToken->value;
        str += " = ";
        str += this->value->toString();
        return str;
    }
};

class ASTFunction : public ASTNode
{
public:
    ASTFunction(const Token *nameToken, std::list<ASTNode *> *statements) : ASTNode(ASTNodeType::FUNCTION), nameToken(nameToken), statements(statements) {}
    const Token *nameToken;
    std::list<ASTNode *> *statements;

    virtual std::string toString()
    {
        std::string str = "func ";
        str += this->nameToken->value;
        str += " {\n";
        for (ASTNode *statement : *this->statements)
        {
            str += statement->toString();
            str += "\n";
        }
        str += "} ";
        return str;
    }
};

ASTDeclaration *parseConstDeclaration(std::list<Token *> &tokens);
ASTNode *parseValueOrOperator(std::list<Token *> &tokens);
ASTNode *parseValue(std::list<Token *> &tokens);
ASTFunction *parseFunction(std::list<Token *> &tokens);
