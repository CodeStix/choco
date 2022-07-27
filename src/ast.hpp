#pragma once

#include <iostream>
#include <list>
#include "token.hpp"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

class GenerationContext
{
public:
    GenerationContext() : llvmIRBuilder(llvmContext), llvmCurrentModule("default-choco-module", llvmContext){};

    llvm::LLVMContext llvmContext;
    llvm::IRBuilder<> llvmIRBuilder;
    llvm::Module llvmCurrentModule;
    std::map<std::string, llvm::Value *> staticNamedValues;
};

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
    BRACKETS,
    FILE
};

class ASTNode
{
public:
    ASTNode(ASTNodeType type) : type(type) {}
    ASTNodeType type;

    virtual std::string toString() = 0;
    virtual llvm::Value *generateLLVM(GenerationContext *context) = 0;
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

    virtual llvm::Value *generateLLVM(GenerationContext *context);
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

    virtual llvm::Value *generateLLVM(GenerationContext *context);
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

    virtual llvm::Value *generateLLVM(GenerationContext *context);
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

    virtual llvm::Value *generateLLVM(GenerationContext *context);
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

    virtual llvm::Value *generateLLVM(GenerationContext *context);
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

    virtual llvm::Value *generateLLVM(GenerationContext *context);
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

    virtual llvm::Value *generateLLVM(GenerationContext *context);
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

    virtual llvm::Value *generateLLVM(GenerationContext *context);
};

class ASTFunction : public ASTNode
{
public:
    ASTFunction(const Token *nameToken, std::vector<const Token *> *arguments, std::list<ASTNode *> *statements) : ASTNode(ASTNodeType::FUNCTION), nameToken(nameToken), arguments(arguments), statements(statements) {}
    const Token *nameToken;
    std::vector<const Token *> *arguments;
    std::list<ASTNode *> *statements;

    virtual std::string toString()
    {
        std::string str = "func ";
        str += this->nameToken->value;
        str += "(";
        bool isFirst = true;
        for (const Token *arg : *this->arguments)
        {
            if (!isFirst)
                str += ", ";
            isFirst = false;
            str += arg->value;
        }
        str += ") {\n";
        if (this->statements != NULL)
        {
            for (ASTNode *statement : *this->statements)
            {
                str += "\t";
                str += statement->toString();
                str += "\n";
            }
        }
        str += "} ";
        return str;
    }

    virtual llvm::Value *generateLLVM(GenerationContext *context);
};

class ASTFile : public ASTNode
{
public:
    ASTFile(std::list<ASTNode *> *statements) : ASTNode(ASTNodeType::FILE), statements(statements) {}
    std::list<ASTNode *> *statements;

    virtual std::string toString()
    {
        std::string str = "";
        for (ASTNode *statement : *this->statements)
        {
            str += statement->toString();
            str += "\n";
        }
        return str;
    }

    virtual llvm::Value *generateLLVM(GenerationContext *context);
};

ASTDeclaration *parseDeclaration(std::list<const Token *> &tokens);
ASTNode *parseValueOrOperator(std::list<const Token *> &tokens);
ASTNode *parseValue(std::list<const Token *> &tokens);
ASTFunction *parseFunction(std::list<const Token *> &tokens);
ASTNode *parseSymbolOperation(std::list<const Token *> &tokens);
ASTFile *parseFile(std::list<const Token *> &tokens);