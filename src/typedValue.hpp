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
    FUNCTION
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

private:
    TypeCode typeCode;
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

class FunctionType : public Type
{

public:
    FunctionType() : Type(TypeCode::FUNCTION) {}

    bool operator==(const Type &b) const override
    {
        return false;
    }

    llvm::Type *getLLVMType(llvm::LLVMContext &context) const override
    {
        return llvm::FunctionType::get(llvm::Type::getVoidTy(context), false);
    }
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
        return llvm::ArrayType::get(this->innerType->getLLVMType(context), this->count)->getPointerTo();
    }

private:
    bool knownCount;
    uint64_t count;
    Type *innerType;
};

class TypedValue
{
public:
    TypedValue(llvm::Value *value, Type *type) : value(value), type(type), originVariableName("") {}

    inline llvm::Value *getValue()
    {
        return this->value;
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