#pragma once
#include <iostream>
#include <map>
#include <list>
#include "llvm/IR/Value.h"
#include "llvm/IR/Constants.h"

llvm::Type *getRefCountType(llvm::LLVMContext &context);
class GenerationContext;
class FunctionScope;
class ASTNode;

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

    virtual std::string toString()
    {
        return "<Unknown>";
    }

    Type *getUnmanagedPointerToType(bool byValue);

private:
    TypeCode typeCode;
};

class TypedValue
{
public:
    TypedValue(llvm::Value *value, Type *type, std::string originVariable = "") : value(value), type(type), originVariableName(originVariable) {}

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

class ModuleType : public Type
{
public:
    ModuleType(std::string name, ModuleType *parent = NULL) : Type(TypeCode::MODULE), name(name), parent(parent) {}

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

    TypedValue *getValue(std::string name, GenerationContext *context, FunctionScope *scope);

    TypedValue *getValueCascade(std::string name, GenerationContext *context, FunctionScope *scope)
    {
        TypedValue *value = this->getValue(name, context, scope);
        if (value != NULL)
        {
            return value;
        }
        else
        {
            if (parent != NULL)
            {
                return parent->getValueCascade(name, context, scope);
            }
            else
            {
                return NULL;
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

    std::string getName()
    {
        return this->name;
    }

    std::string getFullName()
    {
        if (this->parent != NULL)
        {
            return this->parent->getFullName() + "." + this->name;
        }
        else
        {
            return this->name;
        }
    }

    std::string toString() override
    {
        std::string str = "module ";
        str += this->name;
        str += " { ... }";
        return str;
    }

private:
    std::string name;
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

    std::string toString() override
    {
        std::string str = "Float";
        str += std::to_string(this->bitSize);
        return str;
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

    std::string toString() override
    {
        std::string str = this->isSigned ? "Int" : "UInt";
        str += std::to_string(this->bitSize);
        return str;
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

    std::string toString() override
    {
        std::string str = "";
        str += this->startInclusive;
        str += "..";
        str += this->endExclusive;
        return str;
    }

private:
    int startInclusive;
    int endExclusive;
};

class PointerType : public Type
{
public:
    // byValue contains whether the pointed value should be passed by value
    PointerType(Type *pointedType, bool byValue, bool managed) : Type(TypeCode::POINTER), pointedType(pointedType), byValue(byValue), managed(managed)
    {
        assert(!(byValue && managed));
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
            return pointerType.managed == this->managed && *pointerType.pointedType == *this->pointedType;
        }
        else
        {
            return false;
        }
    }

    llvm::Type *getLLVMType(llvm::LLVMContext &context) const override
    {
        llvm::Type *pointedType = this->pointedType->getLLVMType(context);
        if (this->managed)
        {
            // Wrap containing value in a struct where the first value contains the reference count
            std::vector<llvm::Type *> fields;
            fields.push_back(getRefCountType(context));
            fields.push_back(pointedType);
            pointedType = llvm::StructType::get(context, fields, false);
        }
        return llvm::PointerType::get(pointedType, 0);
    }

    std::string toString() override
    {
        std::string str = this->managed ? "m&" : (this->byValue ? "#" : "&");
        str += this->pointedType->toString();
        return str;
    }

    bool isByValue()
    {
        return this->byValue;
    }

    bool isManaged()
    {
        return this->managed;
    }

private:
    bool managed;
    bool byValue;
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

        llvm::Type *returnType;
        if (this->returnType == NULL)
        {
            returnType = llvm::Type::getVoidTy(context);
        }
        else
        {
            returnType = this->returnType->getLLVMType(context);
        }

        for (auto &param : this->parameters)
        {
            parameters.push_back(param.type->getLLVMType(context));
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

    std::string toString() override
    {
        std::string str = "func(";

        bool first = true;
        for (auto &param : this->parameters)
        {
            if (first)
            {
                first = false;
            }
            else
            {
                str += ", ";
            }

            str += param.name;
            str += ": ";
            str += param.type->toString();
        }

        str += ")";

        if (this->returnType != NULL)
        {
            str += " ";
            str += this->returnType->toString();
        }

        return str;
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

    std::string toString() override
    {
        std::string str = "[";
        str += this->innerType->toString();
        if (this->knownCount)
        {
            str += " # ";
            str += std::to_string(this->count);
        }
        str += "]";
        return str;
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
    StructType(std::string name, std::vector<StructTypeField> fields, bool packed) : Type(TypeCode::STRUCT), name(name), fields(fields), packed(packed) {}

    bool operator==(const Type &b) const override
    {
        if (b.getTypeCode() == TypeCode::STRUCT)
        {
            auto other = static_cast<const StructType &>(b);
            if (other.fields.size() != this->fields.size())
            {
                return false;
            }

            if (other.packed != this->packed)
            {
                return false;
            }

            for (int i = 0; i < this->fields.size(); i++)
            {
                if (*this->fields[i].type != *other.fields[i].type)
                {
                    return false;
                }
            }

            return true;
        }
        else
        {
            return false;
        }
    }

    llvm::Type *getLLVMType(llvm::LLVMContext &context) const override
    {
        std::vector<llvm::Type *> fieldTypes;
        for (auto &field : this->fields)
        {
            fieldTypes.push_back(field.type->getLLVMType(context));
        }
        if (this->name != "")
        {
            auto type = llvm::StructType::getTypeByName(context, this->name);
            if (type == NULL)
            {
                type = llvm::StructType::create(fieldTypes, this->name, this->packed);
            }
            return type;
        }
        else
        {
            return llvm::StructType::get(context, fieldTypes, this->packed);
        }
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

    std::string toString() override
    {
        std::string str = "";
        if (this->packed)
        {
            str += "packed ";
        }
        if (this->name != "")
        {
            str += this->name;
            str += " ";
        }
        str += "{";
        bool first = true;
        for (auto &field : this->fields)
        {
            if (first)
            {
                first = false;
            }
            else
            {
                str += ", ";
            }
            str += field.name;
            str += ": ";
            str += field.type->toString();
        }
        str += "}";
        return str;
    }

    std::vector<StructTypeField> getFields()
    {
        return this->fields;
    }

private:
    std::vector<StructTypeField> fields;
    bool packed;
    std::string name;
};

extern IntegerType BYTE_TYPE;
extern IntegerType CHAR_TYPE;
extern IntegerType BOOL_TYPE;
extern IntegerType UINT32_TYPE;

std::string typeCodeToString(TypeCode code);