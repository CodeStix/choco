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
    INVOCATION,
    READ_VARIABLE,
    BRACKETS
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
    ASTBrackets(ASTNode *inner) : ASTNode(ASTNodeType::BRACKETS), inner(inner) {}
    ASTNode *inner;

    virtual std::string toString()
    {
        std::string str = "(";
        str += this->inner->toString();
        str += ")";
        return str;
    }
};

class ASTReadVariable : public ASTNode
{
public:
    ASTReadVariable(const Token *nameToken) : ASTNode(ASTNodeType::READ_VARIABLE), nameToken(nameToken) {}
    const Token *nameToken;

    virtual std::string toString()
    {
        std::string str = this->nameToken->value;
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
        if (this->value != NULL)
        {
            str += " = ";
            str += this->value->toString();
        }
        return str;
    }
};

class ASTAssignment : public ASTNode
{
public:
    ASTAssignment(const Token *nameToken, ASTNode *value) : ASTNode(ASTNodeType::ASSIGNMENT), nameToken(nameToken), value(value) {}
    const Token *nameToken;
    ASTNode *value;

    virtual std::string toString()
    {
        std::string str = this->nameToken->value;
        str += " = ";
        str += this->value->toString();
        return str;
    }
};

class ASTInvocation : public ASTNode
{
public:
    ASTInvocation(const Token *functionNameToken, std::list<ASTNode *> *parameterValues) : ASTNode(ASTNodeType::INVOCATION), functionNameToken(functionNameToken), parameterValues(parameterValues) {}
    const Token *functionNameToken;
    std::list<ASTNode *> *parameterValues;

    virtual std::string toString()
    {
        std::string str = this->functionNameToken->value;
        str += "(";
        bool isFirst = true;
        for (ASTNode *parameterValue : *this->parameterValues)
        {
            if (!isFirst)
                str += ", ";
            isFirst = false;

            str += parameterValue->toString();
        }
        str += ")";
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
            str += "\t";
            str += statement->toString();
            str += "\n";
        }
        str += "} ";
        return str;
    }
};

ASTDeclaration *parseDeclaration(std::list<const Token *> &tokens);
ASTNode *parseValueOrOperator(std::list<const Token *> &tokens);
ASTNode *parseValue(std::list<const Token *> &tokens);
ASTFunction *parseFunction(std::list<const Token *> &tokens);
ASTNode *parseSymbolOperation(std::list<const Token *> &tokens);