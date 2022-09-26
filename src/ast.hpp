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

class GenerationContext
{
public:
    GenerationContext() : context(std::make_unique<llvm::LLVMContext>()),
                          irBuilder(std::make_unique<llvm::IRBuilder<>>(*context)),
                          module(std::make_unique<llvm::Module>("default-choco-module", *context)),
                          passManager(std::make_unique<llvm::legacy::FunctionPassManager>(module.get())),
                          globalModule(ModuleType("Global"))
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
    FunctionType *currentFunction;
    ModuleType globalModule;
};

enum class ASTNodeType
{
    OPERATOR,
    UNARY_OPERATOR,
    LITERAL_NUMBER,
    LITERAL_STRING,
    FUNCTION,
    DECLARATION,
    ASSIGNMENT,
    INVOCATION,
    SYMBOL,
    BRACKETS,
    FILE,
    RETURN,
    IF,
    BLOCK,
    FOR,
    TYPE,
    PARAMETER,
    STRUCT,
    STRUCT_FIELD,
    DEREFERENCE,
    DEREFERENCE_MEMBER,
    DEREFERENCE_INDEX,
    CAST,
    STRUCT_DECLARATION
};

std::string astNodeTypeToString(ASTNodeType type);

class ASTNode
{
public:
    ASTNode(ASTNodeType type) : type(type) {}
    ASTNodeType type;

    virtual std::string toString()
    {
        return astNodeTypeToString(this->type);
    }

    virtual void declareStaticNames(ModuleType *currentModule)
    {
    }

    virtual TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope)
    {
        std::cout << "WARNING: ASTNode::generateLLVM was called without any implementation\n";
        return NULL;
    }

    virtual bool isTerminating()
    {
        return false;
    }
};

class ASTUnaryOperator : public ASTNode
{
public:
    ASTUnaryOperator(const Token *operatorToken, ASTNode *operand) : ASTNode(ASTNodeType::UNARY_OPERATOR), operatorToken(operatorToken), operand(operand) {}
    const Token *operatorToken;
    ASTNode *operand;

    std::string toString() override
    {
        std::string str = this->operatorToken->value;
        str += this->operand->toString();
        return str;
    }

    TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope) override;
};

class ASTOperator : public ASTNode
{
public:
    ASTOperator(const Token *operatorToken, ASTNode *left, ASTNode *right) : ASTNode(ASTNodeType::OPERATOR), operatorToken(operatorToken), left(left), right(right) {}
    const Token *operatorToken;
    ASTNode *left;
    ASTNode *right;

    std::string toString() override
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

    TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope) override;
};

// class ASTTypeNode : public ASTNode
// {
// public:
//     ASTTypeNode(ASTNodeType type) : ASTNode(type) {}
//     virtual Type *getSpecifiedType(GenerationContext *context, FunctionScope *scope) = 0;
// };

class ASTCast : public ASTNode
{
public:
    ASTCast(ASTNode *targetType, ASTNode *value) : ASTNode(ASTNodeType::CAST), targetType(targetType), value(value) {}
    ASTNode *targetType;
    ASTNode *value;

    std::string toString() override
    {
        std::string str = "(";
        str += this->targetType->toString();
        str += ")";
        str += this->value->toString();
        return str;
    }

    TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope) override;
};

// class ASTTypeName : public ASTTypeNode
// {
// public:
//     ASTTypeName(const Token *nameToken) : ASTTypeNode(ASTNodeType::TYPE), nameToken(nameToken) {}

//     Type *getSpecifiedType() override
//     {
//         std::string name = this->nameToken->value;

//         if (name.rfind("Int", 0) == 0)
//         {
//             std::string bitsPart = name.substr(3);
//             int bits = std::stoi(bitsPart);
//             if (bits > 0)
//             {
//                 return new IntegerType(bits, true);
//             }
//         }
//         if (name.rfind("UInt", 0) == 0)
//         {
//             std::string bitsPart = name.substr(4);
//             int bits = std::stoi(bitsPart);
//             if (bits > 0)
//             {
//                 return new IntegerType(bits, false);
//             }
//         }
//         if (name == "Float32")
//         {
//             return new FloatType(32);
//         }
//         if (name == "Float64")
//         {
//             return new FloatType(64);
//         }
//         if (name == "Float128")
//         {
//             return new FloatType(128);
//         }

//         std::cout << "ERROR: type '" << name << "' not found\n";
//         return NULL;
//     }

//     TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope) override
//     {
//         return NULL;
//     }

//     std::string toString() override
//     {
//         return this->nameToken->value;
//     }

// private:
//     const Token *nameToken;
// };

// class ASTStructTypeField : public ASTTypeNode
// {
// public:
//     ASTStructTypeField(const Token *fieldNameToken, ASTTypeNode *type, bool hidden = false) : ASTTypeNode(ASTNodeType::STRUCT_TYPE_FIELD), fieldNameToken(fieldNameToken), type(type), hidden(hidden) {}

//     Type *getSpecifiedType(GenerationContext *context, FunctionScope *scope) override
//     {
//         return this->type->getSpecifiedType(context, scope);
//     }

//     std::string getName()
//     {
//         if (this->fieldNameToken != NULL)
//         {
//             return this->fieldNameToken->value;
//         }
//         else
//         {
//             return NULL;
//         }
//     }

//     std::string toString() override
//     {
//         std::string str = "";
//         if (this->fieldNameToken != NULL)
//         {
//             str += this->fieldNameToken->value;
//         }
//         str += ": ";
//         str += this->type->toString();
//         str += "\n";
//         return str;
//     }

// private:
//     const Token *fieldNameToken;
//     ASTTypeNode *type;
//     bool hidden;
// };

// class ASTStructType : public ASTTypeNode
// {
// public:
//     ASTStructType(const Token *nameToken, std::vector<ASTStructTypeField *> fields, bool managed = true, bool packed = false) : ASTTypeNode(ASTNodeType::STRUCT_TYPE), nameToken(nameToken), fields(fields), managed(managed), packed(packed) {}

//     Type *getSpecifiedType(GenerationContext *context, FunctionScope *scope) override
//     {
//         std::vector<StructTypeField> fields;
//         for (auto &field : this->fields)
//         {
//             fields.push_back(StructTypeField(field->getSpecifiedType(context, scope), field->getName()));
//         }
//         return new StructType(fields, this->managed, this->packed);
//     }

//     std::string toString() override
//     {
//         std::string str = "";
//         if (!this->managed)
//         {
//             str += "unmanaged ";
//         }
//         if (this->packed)
//         {
//             str += "packed ";
//         }
//         str += "{";
//         for (auto &field : this->fields)
//         {
//             str += field->toString();
//         }
//         str += "}";
//         return str;
//     }

// private:
//     bool managed;
//     bool packed;
//     const Token *nameToken;
//     std::vector<ASTStructTypeField *> fields;
// };

class ASTStructField : public ASTNode
{
public:
    ASTStructField(const Token *nameToken, ASTNode *value) : ASTNode(ASTNodeType::STRUCT_FIELD), nameToken(nameToken), value(value) {}

    std::string getName()
    {
        return this->nameToken->value;
    }

    std::string toString() override
    {
        std::string str = "";
        if (this->nameToken != NULL)
        {
            str += this->nameToken->value;
        }
        str += ": ";
        str += this->value->toString();
        str += "\n";
        return str;
    }

    TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope) override;

private:
    const Token *nameToken;
    ASTNode *value;
};

class ASTStruct : public ASTNode
{
public:
    ASTStruct(std::vector<ASTStructField *> fields, bool managed = true, bool packed = false, bool value = false) : ASTNode(ASTNodeType::STRUCT), fields(fields), managed(managed), packed(packed), value(value) {}

    TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope) override;

    std::string toString() override
    {
        std::string str = "";
        if (!this->managed)
        {
            str += "unmanaged ";
        }
        if (this->packed)
        {
            str += "packed ";
        }
        if (this->value)
        {
            str += "value ";
        }
        str += "{";
        for (auto &field : this->fields)
        {
            str += field->toString();
        }
        str += "}";
        return str;
    }

private:
    std::vector<ASTStructField *> fields;
    bool managed = true;
    bool packed = false;
    bool value = false;
};

class ASTBrackets : public ASTNode
{
public:
    ASTBrackets(ASTNode *inner) : ASTNode(ASTNodeType::BRACKETS), inner(inner) {}
    ASTNode *inner;

    std::string toString() override
    {
        std::string str = "(";
        str += this->inner->toString();
        str += ")";
        return str;
    }

    TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope) override;
};

class ASTSymbol : public ASTNode
{
public:
    ASTSymbol(const Token *nameToken) : ASTNode(ASTNodeType::SYMBOL), nameToken(nameToken) {}
    const Token *nameToken;

    std::string toString() override
    {
        return this->nameToken->value;
    }

    TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope) override;
};

// class ASTDereference : public ASTNode
// {
// public:
//     ASTDereference(const Token *nameToken, std::vector<ASTNode *> dereferences) : ASTNode(ASTNodeType::DEREFERENCE), nameToken(nameToken), dereferences(dereferences) {}

//     std::string toString() override
//     {
//         return this->nameToken->value;
//     }

//     TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope) override;

// private:
//     const Token *nameToken;
//     std::vector<ASTNode *> dereferences;
// };

class ASTIndexDereference : public ASTNode
{
public:
    ASTIndexDereference(ASTNode *toIndex, ASTNode *index) : ASTNode(ASTNodeType::DEREFERENCE_INDEX), toIndex(toIndex), index(index) {}

    std::string toString() override
    {
        std::string str = this->toIndex->toString();
        str += "[" + this->index->toString();
        str += "]";
        return str;
    }

    TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope) override;

private:
    ASTNode *toIndex;
    ASTNode *index;
};

class ASTMemberDereference : public ASTNode
{
public:
    ASTMemberDereference(ASTNode *toIndex, const Token *nameToken) : ASTNode(ASTNodeType::DEREFERENCE_MEMBER), nameToken(nameToken), toIndex(toIndex) {}

    std::string toString() override
    {
        std::string str = this->toIndex->toString();
        str += "." + this->nameToken->value;
        return str;
    }

    TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope) override;

private:
    ASTNode *toIndex;
    const Token *nameToken;
};

class ASTLiteralString : public ASTNode
{
public:
    ASTLiteralString(const Token *value) : ASTNode(ASTNodeType::LITERAL_STRING), valueToken(value) {}
    const Token *valueToken;

    std::string toString() override
    {
        std::string str = "\"";
        str += this->valueToken->value;
        str += "\"";
        return str;
    }

    TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope) override;
};

class ASTLiteralNumber : public ASTNode
{
public:
    ASTLiteralNumber(const Token *value) : ASTNode(ASTNodeType::LITERAL_NUMBER), valueToken(value) {}
    const Token *valueToken;

    std::string toString() override
    {
        std::string str = this->valueToken->value;
        return str;
    }

    TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope) override;
};

class ASTDeclaration : public ASTNode
{
public:
    ASTDeclaration(const Token *nameToken, ASTNode *value, ASTNode *typeSpecifier) : ASTNode(ASTNodeType::DECLARATION), nameToken(nameToken), value(value), typeSpecifier(typeSpecifier) {}
    const Token *nameToken;
    ASTNode *value;
    ASTNode *typeSpecifier;

    std::string toString() override
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

    TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope) override;
};

class ASTAssignment : public ASTNode
{
public:
    ASTAssignment(ASTNode *pointerValue, ASTNode *value) : ASTNode(ASTNodeType::ASSIGNMENT), pointerValue(pointerValue), value(value) {}
    ASTNode *pointerValue;
    ASTNode *value;

    std::string toString() override
    {
        std::string str = this->pointerValue->toString();
        str += " = ";
        str += this->value->toString();
        return str;
    }

    TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope) override;
};

class ASTReturn : public ASTNode
{
public:
    ASTReturn(ASTNode *value) : ASTNode(ASTNodeType::RETURN), value(value) {}
    ASTNode *value;

    std::string toString() override
    {
        std::string str = "return ";
        if (this->value != NULL)
        {
            str += this->value->toString();
        }
        return str;
    }

    bool isTerminating() override
    {
        return true;
    }

    TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope) override;
};

class ASTBlock : public ASTNode
{
public:
    ASTBlock(std::list<ASTNode *> *statements) : ASTNode(ASTNodeType::BLOCK), statements(statements) {}
    std::list<ASTNode *> *statements;

    std::string toString() override
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

    bool isTerminating() override
    {
        for (ASTNode *statement : *this->statements)
        {
            if (statement->isTerminating())
            {
                return true;
            }
        }
        return false;
    }

    TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope) override;
};

class ASTParameter : public ASTNode
{
public:
    ASTParameter(const Token *nameToken, ASTNode *typeSpecifier = NULL) : ASTNode(ASTNodeType::PARAMETER), nameToken(nameToken), typeSpecifier(typeSpecifier)
    {
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

    TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope) override;

    // Type *getSpecifiedType(GenerationContext *context, FunctionScope *scope)
    // {
    //     if (this->typeSpecifier != NULL)
    //     {
    //         return this->typeSpecifier->getSpecifiedType(context, scope);
    //     }
    //     else
    //     {
    //         std::cout << "WARNING: ASTParameter::getSpecifiedType() was called without a type specifier";
    //         return NULL;
    //     }
    // }

    std::string getParameterName()
    {
        return this->nameToken->value;
    }

private:
    const Token *nameToken;
    ASTNode *typeSpecifier;
};

class ASTInvocation : public ASTNode
{
public:
    ASTInvocation(ASTNode *functionPointerValue, std::vector<ASTNode *> *parameterValues) : ASTNode(ASTNodeType::INVOCATION), functionPointerValue(functionPointerValue), parameterValues(parameterValues) {}
    ASTNode *functionPointerValue;
    std::vector<ASTNode *> *parameterValues;

    std::string toString() override
    {
        std::string str = this->functionPointerValue->toString();
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

    TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope) override;
};

class ASTFunction : public ASTNode
{
public:
    ASTFunction(const Token *nameToken, std::vector<ASTParameter *> *parameters, ASTNode *returnType, ASTBlock *body, bool exported = false) : ASTNode(ASTNodeType::FUNCTION), nameToken(nameToken), parameters(parameters), returnType(returnType), body(body), exported(exported) {}
    const Token *nameToken;
    std::vector<ASTParameter *> *parameters;
    ASTNode *returnType;
    ASTBlock *body;
    bool exported;

    std::string toString() override
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

    void declareStaticNames(ModuleType *currentModule) override
    {
        currentModule->addLazyValue(this->nameToken->value, this);
    }

    TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope) override;
};

class ASTFile : public ASTNode
{
public:
    ASTFile(std::list<ASTNode *> *statements) : ASTNode(ASTNodeType::FILE), statements(statements) {}
    std::list<ASTNode *> *statements;

    virtual std::string toString() override
    {
        std::string str = "";
        for (ASTNode *statement : *this->statements)
        {
            str += statement->toString();
            str += "\n";
        }
        return str;
    }

    void declareStaticNames(ModuleType *currentModule) override
    {
        for (ASTNode *statement : *this->statements)
        {
            statement->declareStaticNames(currentModule);
        }
    }

    TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope) override;
};

class ASTStructDeclaration : public ASTNode
{
public:
    ASTStructDeclaration(const Token *nameToken, ASTStruct *structNode) : ASTNode(ASTNodeType::STRUCT_DECLARATION), nameToken(nameToken), structNode(structNode) {}

    virtual std::string toString() override
    {
        std::string str = "struct ";
        str += this->nameToken->value;
        str += " ";
        str += this->structNode->toString();
        str += "\n";
        return str;
    }

    void declareStaticNames(ModuleType *currentModule) override
    {
        currentModule->addLazyValue(this->nameToken->value, this);
    }

    TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope) override;

private:
    const Token *nameToken;
    ASTStruct *structNode;
};

class ASTIfStatement : public ASTNode
{
public:
    ASTIfStatement(ASTNode *condition, ASTNode *thenBody, ASTNode *elseBody) : ASTNode(ASTNodeType::IF), condition(condition), thenBody(thenBody), elseBody(elseBody) {}
    ASTNode *condition;
    ASTNode *thenBody;
    ASTNode *elseBody;

    std::string toString() override
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

    TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope) override;

    bool isTerminating() override
    {
        if (this->elseBody != NULL)
        {
            return this->thenBody->isTerminating() && this->elseBody->isTerminating();
        }
        else
        {
            return false;
        }
    }
};

class ASTWhileStatement : public ASTNode
{
public:
    ASTWhileStatement(ASTNode *condition, ASTNode *loopBody, ASTNode *elseBody) : ASTNode(ASTNodeType::IF), condition(condition), loopBody(loopBody), elseBody(elseBody) {}
    ASTNode *condition;
    ASTNode *loopBody;
    ASTNode *elseBody;

    std::string toString() override
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

    bool isTerminating() override
    {
        if (this->elseBody != NULL)
        {
            // TODO: check if break is used
            return this->loopBody->isTerminating() && this->elseBody->isTerminating();
        }
        else
        {
            return false;
        }
    }

    TypedValue *generateLLVM(GenerationContext *context, FunctionScope *scope) override;
};

ASTNode *parseIfStatement(TokenStream *tokens);
ASTNode *parseWhileStatement(TokenStream *tokens);
ASTDeclaration *parseDeclaration(TokenStream *tokens);
ASTNode *parseValueOrOperator(TokenStream *tokens);
ASTNode *parseValueOrType(TokenStream *tokens);
ASTFunction *parseFunction(TokenStream *tokens);
ASTNode *parseSymbolOperation(TokenStream *tokens);
ASTFile *parseFile(TokenStream *tokens);
ASTReturn *parseReturn(TokenStream *tokens);
ASTParameter *parseParameter(TokenStream *tokens);
ASTNode *parseValueAndSuffix(TokenStream *tokens);
ASTStruct *parseStruct(TokenStream *tokens);
ASTNode *parseInlineType(TokenStream *tokens);