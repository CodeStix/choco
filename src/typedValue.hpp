#pragma once
#include <iostream>
#include "llvm/IR/Value.h"
#include "llvm/IR/Constants.h"

enum class TypeCode
{
    BITS,
    INTEGER,
    FLOAT,
    ARRAY,
    STRUCT,
    ENUM,
    UNION,
    RANGE,
    STRING,
    FUNCTION,
    POINTER,
    MODULE,
};

class Type
{
public:
    Type(TypeCode typeCode) : typeCode(typeCode) {}

    TypeCode getTypeCode() const
    {
        return this->typeCode;
    }

    virtual llvm::Type *getLLVMType(llvm::LLVMContext &context) const = 0;
    virtual bool operator==(const Type &b) const = 0;

    bool operator!=(const Type &b)
    {
        return !(*this == b);
    }

    // Type *getDeepPointedType()
    // {
    //     if (this->typeCode == TypeCode::POINTER)
    //     {
    //         PointerType *pointerType = static_cast<PointerType *>(this);
    //         return pointerType->getDeepPointedType();
    //     }
    //     else
    //     {
    //         return this;
    //     }
    // }

    Type *getPointerToType();

private:
    TypeCode typeCode;
};

class ModuleType : public Type
{
public:
    ModuleType(ModuleType *parent = NULL) : Type(TypeCode::MODULE), parent(parent) {}

    bool addLazyValue(std::string name, ASTNode *node)
    {
        if (this->lazyNamedStatics[name])
        {
            return false;
        }
        else
        {
            this->lazyNamedStatics[name] = node;
            return true;
        }
    }

    bool addValue(std::string name, TypedValue *value)
    {
        if (this->namedStatics[name])
        {
            return false;
        }
        else
        {
            this->namedStatics[name] = value;
            return true;
        }
    }

    bool hasValue(std::string name)
    {
        return this->namedStatics[name] != NULL || this->lazyNamedStatics[name] != NULL;
    }

    TypedValue *getValue(std::string name, GenerationContext *context, FunctionScope *scope)
    {
        TypedValue *value = this->namedStatics[name];
        if (value != NULL)
        {
            return value;
        }
        else
        {
            ASTNode *lazyValue = this->lazyNamedStatics[name];
            if (lazyValue != NULL)
            {
                value = lazyValue->generateLLVM(context, scope);
                this->namedStatics[name] = value;
                return value;
            }
            else
            {
                if (parent != NULL)
                {
                    return parent->getValue(name, context, scope);
                }
                else
                {
                    return NULL;
                }
            }
        }
    }

    llvm::Type *getLLVMType(llvm::LLVMContext &context) const override
    {
        return NULL;
    }

    bool operator==(const Type &b) const override
    {
        return false;
    }

private:
    ModuleType *parent;
    std::map<std::string, ASTNode *> lazyNamedStatics;
    std::map<std::string, TypedValue *> namedStatics;
};

class FloatType : public Type
{
public:
    FloatType(int bitSize) : Type(TypeCode::FLOAT), bitSize(bitSize) {}

    bool operator==(const Type &b) const override
    {
        if (b.getTypeCode() == TypeCode::FLOAT)
        {
            auto f = static_cast<const FloatType &>(b);
            return f.bitSize == this->bitSize;
        }
        else
        {
            return false;
        }
    }

    int getBitSize() const
    {
        return this->bitSize;
    }

    llvm::Type *getLLVMType(llvm::LLVMContext &context) const override
    {
        switch (this->bitSize)
        {
        case 32:
            return llvm::Type::getFloatTy(context);
        case 64:
            return llvm::Type::getDoubleTy(context);
        case 128:
            return llvm::Type::getFP128Ty(context);

        default:
            std::cout << "ERROR: FloatType.getLLVMType() cannot create LLVM type with given bit size " << this->bitSize << "\n";
            return NULL;
        }
    }

private:
    int bitSize;
};

class IntegerType : public Type
{
public:
    IntegerType(int bitSize, bool isSigned) : Type(TypeCode::INTEGER), bitSize(bitSize), isSigned(isSigned) {}

    bool operator==(const Type &b) const override
    {
        if (b.getTypeCode() == TypeCode::INTEGER)
        {
            auto i = static_cast<const IntegerType &>(b);
            return i.isSigned == this->isSigned && i.bitSize == this->bitSize;
        }
        else
        {
            return false;
        }
    }

    int getBitSize() const
    {
        return this->bitSize;
    }

    bool getSigned() const
    {
        return this->isSigned;
    }

    llvm::Type *getLLVMType(llvm::LLVMContext &context) const override
    {
        return llvm::Type::getIntNTy(context, this->bitSize);
    }

private:
    bool isSigned;
    int bitSize;
};

class RangeType : public Type
{
public:
    RangeType(int startInclusive, int endExclusive) : Type(TypeCode::RANGE), startInclusive(startInclusive), endExclusive(endExclusive) {}

    bool operator==(const Type &b) const override
    {
        if (b.getTypeCode() == TypeCode::RANGE)
        {
            auto range = static_cast<const RangeType &>(b);
            return range.endExclusive == this->endExclusive && range.startInclusive == this->startInclusive;
        }
        else
        {
            return false;
        }
    }

    llvm::Type *getLLVMType(llvm::LLVMContext &context) const override
    {
        return llvm::Type::getIntNTy(context, 32);
    }

private:
    int startInclusive;
    int endExclusive;
};

class PointerType : public Type
{
public:
    PointerType(Type *pointedType) : Type(TypeCode::POINTER), pointedType(pointedType)
    {
    }

    Type *getPointedType()
    {
        return this->pointedType;
    }

    bool operator==(const Type &b) const override
    {
        if (b.getTypeCode() == TypeCode::POINTER)
        {
            const PointerType &pointerType = static_cast<const PointerType &>(b);
            return *pointerType.pointedType == *this->pointedType;
        }
        else
        {
            return false;
        }
    }

    llvm::Type *getLLVMType(llvm::LLVMContext &context) const override
    {
        return llvm::PointerType::get(this->pointedType->getLLVMType(context), 0);
    }

private:
    Type *pointedType;
};

class FunctionParameter
{
public:
    FunctionParameter(Type *type, std::string name = "") : type(type), name(name)
    {
    }

    Type *type;
    std::string name;
};

class FunctionType : public Type
{
public:
    FunctionType(Type *returnType) : Type(TypeCode::FUNCTION), returnType(returnType) {}
    FunctionType(Type *returnType, std::vector<FunctionParameter> parameters) : Type(TypeCode::FUNCTION), returnType(returnType), parameters(parameters), isVarArg(false) {}

    bool operator==(const Type &b) const override
    {
        return false;
    }

    llvm::Type *getLLVMType(llvm::LLVMContext &context) const override
    {
        std::vector<llvm::Type *> parameters;
        for (auto &param : this->parameters)
        {
            parameters.push_back(param.type->getLLVMType(context));
        }

        llvm::Type *returnType;
        if (this->returnType == NULL)
        {
            returnType = llvm::Type::getVoidTy(context);
        }
        else
        {
            returnType = this->returnType->getLLVMType(context);
        }

        return llvm::FunctionType::get(returnType, parameters, this->isVarArg);
    }

    Type *getReturnType()
    {
        return this->returnType;
    }

    std::vector<FunctionParameter> &getParameters()
    {
        return this->parameters;
    }

    bool getIsVarArg()
    {
        return this->isVarArg;
    }

private:
    bool isVarArg;
    std::vector<FunctionParameter> parameters;
    Type *returnType;
};

class ArrayType : public Type
{
public:
    ArrayType(Type *innerType, uint64_t count) : Type(TypeCode::ARRAY), innerType(innerType), count(count), knownCount(true) {}

    bool operator==(const Type &b) const override
    {
        return false;
    }

    llvm::Type *getLLVMType(llvm::LLVMContext &context) const override
    {
        return llvm::ArrayType::get(this->innerType->getLLVMType(context), this->count);
    }

private:
    bool knownCount;
    uint64_t count;
    Type *innerType;
};

class StructTypeField
{
public:
    StructTypeField(Type *type, std::string name = "") : type(type), name(name)
    {
    }

    Type *type;
    std::string name;
};

class StructType : public Type
{
public:
    StructType(std::vector<StructTypeField> fields, bool managed = true, bool packed = false) : Type(TypeCode::STRUCT), fields(fields), managed(managed), packed(packed) {}

    bool operator==(const Type &b) const override
    {
        return false;
    }

    llvm::Type *getLLVMType(llvm::LLVMContext &context) const override
    {
        std::vector<llvm::Type *> fieldTypes;
        for (auto &field : this->fields)
        {
            fieldTypes.push_back(field.type->getLLVMType(context));
        }
        return llvm::StructType::get(context, fieldTypes, this->packed);
    }

    StructTypeField *getField(std::string name)
    {
        for (auto &field : this->fields)
        {
            if (field.name == name)
            {
                return &field;
            }
        }
        return NULL;
    }

    int getFieldIndex(std::string name)
    {
        int index = 0;
        for (auto &field : this->fields)
        {
            if (field.name == name)
            {
                return index;
            }
            index++;
        }
        return -1;
    }

    int getMaxIndex()
    {
        return this->fields.size();
    }

private:
    std::vector<StructTypeField> fields;
    bool managed;
    bool packed;
};

class TypedValue
{
public:
    TypedValue(llvm::Value *value, Type *type) : value(value), type(type), originVariableName("") {}

    inline llvm::Value *getValue()
    {
        return this->value;
    }

    inline bool isType()
    {
        return this->value == NULL;
    }

    inline Type *getType()
    {
        return this->type;
    }

    inline TypeCode getTypeCode()
    {
        return this->type->getTypeCode();
    }

    void setOriginVariable(std::string name)
    {
        this->originVariableName = name;
    }

    std::string getOriginVariable()
    {
        return this->originVariableName;
    }

    bool operator==(TypedValue &b)
    {
        return this->value == b.getValue() && this->type == b.getType();
    }

    bool operator!=(TypedValue &b)
    {
        return !(*this == b);
    }

private:
    llvm::Value *value;
    Type *type;
    std::string originVariableName;
};

extern IntegerType BYTE_TYPE;
extern IntegerType CHAR_TYPE;
extern IntegerType BOOL_TYPE;