#pragma once

#include <iostream>
#include <list>
#include <map>
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
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"
#include "llvm/Transforms/Utils.h"
#include "typedValue.hpp"

class GenerationContext
{
public:
    GenerationContext() : context(std::make_unique<llvm::LLVMContext>()),
                          irBuilder(std::make_unique<llvm::IRBuilder<>>(*context)),
                          module(std::make_unique<llvm::Module>("default-choco-module", *context)),
                          passManager(std::make_unique<llvm::legacy::FunctionPassManager>(module.get()))
    {
        passManager->add(llvm::createPromoteMemoryToRegisterPass());
        passManager->add(llvm::createInstructionCombiningPass());
        passManager->add(llvm::createReassociatePass());
        passManager->add(llvm::createGVNPass());
        passManager->add(llvm::createCFGSimplificationPass());
        passManager->add(llvm::createDeadCodeEliminationPass());
        passManager->doInitialization();
    };

    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::IRBuilder<>> irBuilder;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::legacy::FunctionPassManager> passManager;
    // std::map<std::string, llvm::Value *> staticNamedValues;
};

class FunctionScope
{
public:
    FunctionScope() {}
    FunctionScope(const FunctionScope &copyFrom)
    {
        for (auto const &p : copyFrom.namedValues)
        {
            this->namedValues[p.first] = p.second;
        }
    }

    // Returns false if the name already exists locally
    bool addValue(const std::string &name, TypedValue *value)
    {
        if (this->hasValue(name))
        {
            return false;
        }
        else
        {
            this->namedValues[name] = value;
            return true;
        }
    }

    bool hasValue(const std::string &name)
    {
        return this->getValue(name) != NULL;
    }

    TypedValue *getValue(const std::string &name)
    {
        return this->namedValues[name];
    }

    // private:
    std::map<std::string, TypedValue *> namedValues;
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
    FILE,
    RETURN,
    IF,
    BLOCK,
    FOR,
    TYPE,
    PARAMETER
};

class ASTNode
{
public:
    ASTNode(ASTNodeType type) : type(type) {}
    ASTNodeType type;

    virtual std::string toString() = 0;
    virtual TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope) = 0;
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

    virtual TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope);
};

class ASTType : public ASTNode
{
public:
    ASTType(const Token *nameToken) : ASTNode(ASTNodeType::TYPE), nameToken(nameToken) {}

    Type *getSpecifiedType()
    {
        std::string name = this->nameToken->value;

        if (name.rfind("Int", 0) == 0)
        {
            std::string bitsPart = name.substr(3);
            int bits = std::stoi(bitsPart);
            if (bits > 0)
            {
                return new IntegerType(bits, true);
            }
        }
        if (name.rfind("UInt", 0) == 0)
        {
            std::string bitsPart = name.substr(4);
            int bits = std::stoi(bitsPart);
            if (bits > 0)
            {
                return new IntegerType(bits, false);
            }
        }
        if (name == "Float32")
        {
            return new FloatType(32);
        }
        if (name == "Float64")
        {
            return new FloatType(64);
        }
        if (name == "Float128")
        {
            return new FloatType(128);
        }

        std::cout << "ERROR: type '" << name << "' not found\n";
        return NULL;
    }

    TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope) override
    {
        return NULL;
    }

    std::string toString() override
    {
        return this->nameToken->value;
    }

private:
    const Token *nameToken;
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

    virtual TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope);
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

    virtual TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope);
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

    virtual TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope);
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

    virtual TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope);
};

class ASTDeclaration : public ASTNode
{
public:
    ASTDeclaration(const Token *nameToken, ASTNode *value, ASTType *typeSpecifier) : ASTNode(ASTNodeType::DECLARATION), nameToken(nameToken), value(value), typeSpecifier(typeSpecifier) {}
    const Token *nameToken;
    ASTNode *value;
    ASTType *typeSpecifier;

    virtual std::string toString()
    {
        std::string str = "let ";
        str += this->nameToken->value;
        if (this->typeSpecifier != NULL)
        {
            str += ": ";
            str += this->typeSpecifier->toString();
        }
        if (this->value != NULL)
        {
            str += " = ";
            str += this->value->toString();
        }
        return str;
    }

    virtual TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope);
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

    virtual TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope);
};

class ASTReturn : public ASTNode
{
public:
    ASTReturn(ASTNode *value) : ASTNode(ASTNodeType::RETURN), value(value) {}
    ASTNode *value;

    virtual std::string toString()
    {
        std::string str = "return ";
        if (this->value != NULL)
        {
            str += this->value->toString();
        }
        return str;
    }

    virtual TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope);
};

class ASTBlock : public ASTNode
{
public:
    ASTBlock(std::list<ASTNode *> *statements) : ASTNode(ASTNodeType::BLOCK), statements(statements) {}
    std::list<ASTNode *> *statements;

    virtual std::string toString()
    {
        std::string str = "{\n";
        for (ASTNode *statement : *this->statements)
        {
            str += "\t";
            str += statement->toString();
            str += "\n";
        }
        str += "}";
        return str;
    }

    virtual TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope);
};

class ASTParameter : public ASTNode
{
public:
    ASTParameter(const Token *nameToken, ASTType *typeSpecifier = NULL) : ASTNode(ASTNodeType::PARAMETER), nameToken(nameToken), typeSpecifier(typeSpecifier)
    {
    }

    TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope) override
    {
        return NULL;
    }

    std::string toString() override
    {
        std::string str = this->nameToken->value;
        if (this->typeSpecifier != NULL)
        {
            str += ": ";
            str += this->typeSpecifier->toString();
        }
        return str;
    }

    Type *getSpecifiedType()
    {
        if (this->typeSpecifier != NULL)
        {
            return this->typeSpecifier->getSpecifiedType();
        }
        else
        {
            std::cout << "WARNING: ASTParameter::getSpecifiedType() was called without a type specifier";
            return NULL;
        }
    }

    const std::string getParameterName()
    {
        return this->nameToken->value;
    }

private:
    const Token *nameToken;
    ASTType *typeSpecifier;
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

    virtual TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope);
};

class ASTFunction : public ASTNode
{
public:
    ASTFunction(const Token *nameToken, std::vector<ASTParameter *> *parameters, ASTType *returnType, ASTNode *body, bool exported = false) : ASTNode(ASTNodeType::FUNCTION), nameToken(nameToken), parameters(parameters), returnType(returnType), body(body), exported(exported) {}
    const Token *nameToken;
    std::vector<ASTParameter *> *parameters;
    ASTType *returnType;
    ASTNode *body;
    bool exported;

    virtual std::string toString()
    {
        std::string str = "";
        if (this->exported)
        {
            str += "export ";
        }
        if (this->body == NULL)
        {
            str += "extern ";
        }
        str += "func ";
        str += this->nameToken->value;
        str += "(";
        bool isFirst = true;
        for (ASTParameter *arg : *this->parameters)
        {
            if (!isFirst)
                str += ", ";
            isFirst = false;
            str += arg->toString();
        }
        str += ") ";
        if (this->returnType != NULL)
        {
            str += ": ";
            str += this->returnType->toString();
        }
        if (this->body != NULL)
        {
            str += this->body->toString();
        }
        return str;
    }

    virtual TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope);
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

    virtual TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope);
};

class ASTIfStatement : public ASTNode
{
public:
    ASTIfStatement(ASTNode *condition, ASTNode *thenBody, ASTNode *elseBody) : ASTNode(ASTNodeType::IF), condition(condition), thenBody(thenBody), elseBody(elseBody) {}
    ASTNode *condition;
    ASTNode *thenBody;
    ASTNode *elseBody;

    virtual std::string toString()
    {
        std::string str = "if ";
        str += this->condition->toString();
        str += this->thenBody->toString();
        if (this->elseBody != NULL)
        {
            str += "else ";
            str += this->elseBody->toString();
        }
        return str;
    }

    virtual TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope);
};

class ASTWhileStatement : public ASTNode
{
public:
    ASTWhileStatement(ASTNode *condition, ASTNode *loopBody, ASTNode *elseBody) : ASTNode(ASTNodeType::IF), condition(condition), loopBody(loopBody), elseBody(elseBody) {}
    ASTNode *condition;
    ASTNode *loopBody;
    ASTNode *elseBody;

    virtual std::string toString()
    {
        std::string str = "while ";
        str += this->condition->toString();
        str += this->loopBody->toString();
        if (this->elseBody != NULL)
        {
            str += "else ";
            str += this->elseBody->toString();
        }
        return str;
    }

    virtual TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope);
};

ASTNode *parseIfStatement(std::list<const Token *> &tokens);
ASTNode *parseWhileStatement(std::list<const Token *> &tokens);
ASTDeclaration *parseDeclaration(std::list<const Token *> &tokens);
ASTNode *parseValueOrOperator(std::list<const Token *> &tokens);
ASTNode *parseValue(std::list<const Token *> &tokens);
ASTFunction *parseFunction(std::list<const Token *> &tokens);
ASTNode *parseSymbolOperation(std::list<const Token *> &tokens);
ASTFile *parseFile(std::list<const Token *> &tokens);
ASTReturn *parseReturn(std::list<const Token *> &tokens);
ASTType *parseType(std::list<const Token *> &tokens);
ASTParameter *parseParameter(std::list<const Token *> &tokens);