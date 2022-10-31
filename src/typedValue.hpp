#pragma once
#include <iostream>
#include <map>
#include <list>
#include "llvm/IR/Value.h"
#include "llvm/IR/Constants.h"

class FunctionScope;
class GenerationContext;
class ASTNode;
class PointerType;

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
    NULLT,
};

class Type
{
public:
    Type(TypeCode typeCode) : typeCode(typeCode) {}

    TypeCode getTypeCode() const
    {
        return this->typeCode;
    }

    virtual llvm::Type *getLLVMType(GenerationContext *context) const = 0;
    virtual bool operator==(const Type &b) const = 0;

    bool operator!=(const Type &b)
    {
        return !(*this == b);
    }

    virtual std::string toString()
    {
        return "<Unknown>";
    }

    PointerType *getUnmanagedPointerToType();
    PointerType *getManagedPointerToType();

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
        return this->originVariableName == "" ? "unknown" : this->originVariableName;
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

    bool addLazyValue(std::string name, ASTNode *node);
    bool addValue(std::string name, TypedValue *value);
    bool hasValue(std::string name);
    TypedValue *getValue(std::string name, GenerationContext *context, FunctionScope *scope);
    TypedValue *getValueCascade(std::string name, GenerationContext *context, FunctionScope *scope);

    llvm::Type *getLLVMType(GenerationContext *context) const override
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

    std::string toString() override;

private:
    std::string name;
    ModuleType *parent;
    std::map<std::string, ASTNode *> lazyNamedStatics;
    std::map<std::string, TypedValue *> namedStatics;
};

class NullType : public Type
{
public:
    NullType() : Type(TypeCode::NULLT) {}

    bool operator==(const Type &b) const override
    {
        return b.getTypeCode() == TypeCode::NULLT;
    }

    llvm::Type *getLLVMType(GenerationContext *context) const override
    {
        assert(false && "NullType has no getLLVMType");
    }

    std::string toString() override
    {
        return "null";
    }
};

class UnionType : public Type
{
public:
    UnionType() : Type(TypeCode::UNION), managed(true) {}
    UnionType(std::vector<Type *> types) : Type(TypeCode::UNION), types(types), managed(true) {}

    bool getIsManaged()
    {
        return this->managed;
    }

    bool operator==(const Type &b) const override;

    llvm::Type *getLLVMType(GenerationContext *context) const override;

    llvm::Type *getLLVMDataType(GenerationContext *context) const;

    llvm::Value *createValue(GenerationContext *context, TypedValue *value) const;

    void addTypes(std::vector<Type *> types)
    {
        for (auto t : types)
        {
            this->addType(t);
        }
    }

    void addType(Type *type)
    {
        for (auto t : this->types)
        {
            if (*t == *type)
            {
                return;
            }
        }

        this->types.push_back(type);
    }

    std::vector<Type *> getTypes()
    {
        return this->types;
    }

    std::string toString() override
    {
        std::string str = "";
        if (!this->managed)
        {
            str += "unmanaged ";
        }

        bool isFirst = true;
        for (auto t : this->types)
        {
            if (isFirst)
            {
                isFirst = false;
            }
            else
            {
                str += "|";
            }
            str += t->toString();
        }
        return str;
    }

    bool containsType(Type *type) const
    {
        for (auto t : this->types)
        {
            if (*t == *type)
            {
                return true;
            }
        }
        return false;
    }

    bool containsNullType() const
    {
        for (auto t : this->types)
        {
            if (t->getTypeCode() == TypeCode::NULLT)
            {
                return true;
            }
        }
        return false;
    }

private:
    std::vector<Type *> types;
    bool managed;
};

class FloatType : public Type
{
public:
    FloatType(int bitSize) : Type(TypeCode::FLOAT), bitSize(bitSize) {}

    bool operator==(const Type &b) const override;

    int getBitSize() const
    {
        return this->bitSize;
    }

    llvm::Type *getLLVMType(GenerationContext *context) const override;

    std::string toString() override;

private:
    int bitSize;
};

class IntegerType : public Type
{
public:
    IntegerType(int bitSize, bool isSigned) : Type(TypeCode::INTEGER), bitSize(bitSize), isSigned(isSigned) {}

    bool operator==(const Type &b) const override;

    int getBitSize() const
    {
        return this->bitSize;
    }

    bool getSigned() const
    {
        return this->isSigned;
    }

    llvm::Type *getLLVMType(GenerationContext *context) const override;

    std::string toString() override;

private:
    bool isSigned;
    int bitSize;
};

class RangeType : public Type
{
public:
    RangeType(int startInclusive, int endExclusive) : Type(TypeCode::RANGE), startInclusive(startInclusive), endExclusive(endExclusive) {}

    bool operator==(const Type &b) const override;

    llvm::Type *getLLVMType(GenerationContext *context) const override;

    std::string toString() override;

private:
    int startInclusive;
    int endExclusive;
};

class PointerType : public Type
{
public:
    // byValue contains whether the pointed value should be passed by value
    PointerType(Type *pointedType, bool managed) : Type(TypeCode::POINTER), pointedType(pointedType), managed(managed)
    {
    }

    Type *getPointedType()
    {
        return this->pointedType;
    }

    bool operator==(const Type &b) const override;

    llvm::Type *getLLVMPointedType(GenerationContext *context) const;

    llvm::Type *getLLVMType(GenerationContext *context) const override;

    std::string toString() override;

    bool isManaged()
    {
        return this->managed;
    }

private:
    // True if a ref count field should be emitted
    bool managed;
    // True if a length field should be emitted (does point to multiple objects of the same type)
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

    llvm::Type *getLLVMType(GenerationContext *context) const override;

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

    std::string toString() override;

private:
    bool isVarArg;
    std::vector<FunctionParameter> parameters;
    Type *returnType;
};

class ArrayType : public Type
{
public:
    ArrayType(Type *innerType, bool value, bool managed) : Type(TypeCode::ARRAY), innerType(innerType), count(-1), value(value), managed(managed) {}
    ArrayType(Type *innerType, int64_t count, bool value, bool managed) : Type(TypeCode::ARRAY), innerType(innerType), count(count), value(value), managed(managed) {}

    bool operator==(const Type &b) const override
    {
        if (b.getTypeCode() == TypeCode::ARRAY)
        {
            auto other = static_cast<const ArrayType &>(b);
            return other.count == this->count && *other.innerType == *this->innerType;
        }
        else
        {
            return false;
        }
    }

    llvm::Type *getLLVMType(GenerationContext *context) const override;

    std::string toString() override;

    bool hasKnownCount()
    {
        return this->count >= 0;
    }

    Type *getItemType()
    {
        return this->innerType;
    }

    int64_t getCount()
    {
        return this->count;
    }

    static llvm::Type *getLLVMLengthFieldType(GenerationContext *context);

private:
    int64_t count;
    Type *innerType;
    bool value;
    bool managed;
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

    bool operator==(const Type &b) const override;

    llvm::Type *getLLVMType(GenerationContext *context) const override;

    StructTypeField *getField(std::string name);

    int getFieldIndex(std::string name);

    int getMaxIndex();

    std::string toString() override;

    std::vector<StructTypeField> getFields()
    {
        return this->fields;
    }

    std::string getName()
    {
        return this->name;
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
extern IntegerType UINT64_TYPE;

std::string typeCodeToString(TypeCode code);