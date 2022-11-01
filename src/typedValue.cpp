#include "context.hpp"
#include "typedValue.hpp"
#include "ast.hpp"
#include "util.hpp"

IntegerType BYTE_TYPE(8, false);
IntegerType CHAR_TYPE(8, false);
IntegerType BOOL_TYPE(1, false);
IntegerType UINT32_TYPE(32, false);
IntegerType UINT64_TYPE(64, false);

PointerType *Type::getUnmanagedPointerToType()
{
    return new PointerType(this, false);
}

PointerType *Type::getManagedPointerToType()
{
    return new PointerType(this, true);
}

TypedValue *ModuleType::getValue(std::string name, GenerationContext *context, FunctionScope *scope)
{
    if (this->namedStatics.count(name) > 0)
    {
        return this->namedStatics[name];
    }
    else
    {
        if (this->lazyNamedStatics.count(name) > 0)
        {
            ASTNode *lazyValue = this->lazyNamedStatics[name];
            auto savedBlock = context->irBuilder->GetInsertBlock();
            auto savedCurrentFunction = context->currentFunction;
            auto savedReturnValuePointer = context->currentFunctionReturnValuePointer;
            auto savedReturnBlock = context->currentFunctionReturnBlock;
            auto value = lazyValue->generateLLVM(context, NULL, NULL, true);
            context->currentFunctionReturnBlock = savedReturnBlock;
            context->currentFunctionReturnValuePointer = savedReturnValuePointer;
            context->currentFunction = savedCurrentFunction;
            context->irBuilder->SetInsertPoint(savedBlock);
            return value;
        }
        else
        {
            return NULL;
        }
    }
}

std::string typeCodeToString(TypeCode code)
{
    switch (code)
    {
    case TypeCode::BITS:
        return "BITS";
    case TypeCode::INTEGER:
        return "INTEGER";
    case TypeCode::FLOAT:
        return "FLOAT";
    case TypeCode::ARRAY:
        return "ARRAY";
    case TypeCode::STRUCT:
        return "STRUCT";
    case TypeCode::ENUM:
        return "ENUM";
    case TypeCode::UNION:
        return "UNION";
    case TypeCode::RANGE:
        return "RANGE";
    case TypeCode::STRING:
        return "STRING";
    case TypeCode::FUNCTION:
        return "FUNCTION";
    case TypeCode::POINTER:
        return "POINTER";
    case TypeCode::MODULE:
        return "MODULE";
    default:
        return "Unknown";
    }
}

bool ModuleType::addLazyValue(std::string name, ASTNode *node)
{
    if (this->lazyNamedStatics.count(name) > 0)
    {
        return false;
    }
    else
    {
        this->lazyNamedStatics[name] = node;
        return true;
    }
}

bool ModuleType::addValue(std::string name, TypedValue *value)
{
    if (this->namedStatics.count(name) > 0)
    {
        return false;
    }
    else
    {
        this->namedStatics[name] = value;
        return true;
    }
}

bool ModuleType::hasValue(std::string name)
{
    return this->namedStatics.count(name) > 0 || this->lazyNamedStatics.count(name) > 0;
}

std::string ModuleType::toString()
{
    std::string str = "module ";
    str += this->name;
    str += " { ";
    bool first = true;
    for (auto &p : this->lazyNamedStatics)
    {
        if (first)
        {
            first = false;
        }
        else
        {
            str += ", ";
        }
        str += p.first;
    }
    str += " } ";
    return str;
}

TypedValue *ModuleType::getValueCascade(std::string name, GenerationContext *context, FunctionScope *scope)
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

bool UnionType::operator==(const Type &b) const
{
    if (b.getTypeCode() == TypeCode::UNION)
    {
        auto f = static_cast<const UnionType &>(b);
        if (f.managed != this->managed || f.types.size() != this->types.size())
        {
            return false;
        }

        for (int i = 0; i < this->types.size(); i++)
        {
            if (*this->types[i] != *f.types[i])
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

llvm::Type *UnionType::getLLVMType(GenerationContext *context) const
{
    std::vector<llvm::Type *> fields;
    fields.push_back(getUnionIdType(*context->context));
    fields.push_back(this->getLLVMDataType(context));
    return llvm::StructType::get(*context->context, fields, false);
}

llvm::Type *UnionType::getLLVMDataType(GenerationContext *context) const
{
    assert(this->types.size() >= 2);

    // Find max size of all type
    int largestSizeBits = 0;
    Type *largestType = NULL;
    for (auto type : this->types)
    {
        if (type->getTypeCode() == TypeCode::NULLT)
            continue;
        auto llvmType = type->getLLVMType(context);
        int size = context->module->getDataLayout().getTypeStoreSizeInBits(llvmType);
        if (size > largestSizeBits)
        {
            largestSizeBits = size;
            largestType = type;
        }
    }

    assert(largestType != NULL);
    assert(largestSizeBits > 0);

    return llvm::Type::getIntNTy(*context->context, largestSizeBits);
}

llvm::Value *UnionType::createValue(GenerationContext *context, TypedValue *value) const
{
    assert(this->managed && "Unmanaged not supported");
    assert(this->containsType(value->getType()));

    uint64_t typeId = context->getTypeId(value->getType());

    auto llvmType = this->getLLVMType(context);
    llvm::Value *llvmUnionValue = llvm::UndefValue::get(llvmType);

    std::vector<unsigned int> indices;
    indices.push_back(0);
    llvmUnionValue = context->irBuilder->CreateInsertValue(llvmUnionValue, llvm::ConstantInt::get(getUnionIdType(*context->context), typeId, false), indices, "union.novalue");

    int typeBits = context->module->getDataLayout().getTypeStoreSizeInBits(value->getValue()->getType());
    auto llvmValueAsInteger = context->irBuilder->CreateBitOrPointerCast(value->getValue(), llvm::Type::getIntNTy(*context->context, typeBits), "union.value.asint");

    auto llvmDataType = this->getLLVMDataType(context);
    llvm::Value *llvmBitCastedValue = context->irBuilder->CreateZExtOrBitCast(llvmValueAsInteger, llvmDataType, "union.value.zext");

    indices[0] = 1;
    llvmUnionValue = context->irBuilder->CreateInsertValue(llvmUnionValue, llvmBitCastedValue, indices, "union");

    return llvmUnionValue;
}

bool FloatType::operator==(const Type &b) const
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

std::string FloatType::toString()
{
    std::string str = "Float";
    str += std::to_string(this->bitSize);
    return str;
}

llvm::Type *FloatType::getLLVMType(GenerationContext *context) const
{
    switch (this->bitSize)
    {
    case 32:
        return llvm::Type::getFloatTy(*context->context);
    case 64:
        return llvm::Type::getDoubleTy(*context->context);
    case 128:
        return llvm::Type::getFP128Ty(*context->context);

    default:
        std::cout << "ERROR: FloatType.getLLVMType() cannot create LLVM type with given bit size " << this->bitSize << "\n";
        return NULL;
    }
}

bool IntegerType::operator==(const Type &b) const
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

llvm::Type *IntegerType::getLLVMType(GenerationContext *context) const
{
    return llvm::Type::getIntNTy(*context->context, this->bitSize);
}

std::string IntegerType::toString()
{
    std::string str = this->isSigned ? "Int" : "UInt";
    str += std::to_string(this->bitSize);
    return str;
}

bool RangeType::operator==(const Type &b) const
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

llvm::Type *RangeType::getLLVMType(GenerationContext *context) const
{
    return llvm::Type::getIntNTy(*context->context, 32);
}

std::string RangeType::toString()
{
    std::string str = "";
    str += this->startInclusive;
    str += "..";
    str += this->endExclusive;
    return str;
}

bool PointerType::operator==(const Type &b) const
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

llvm::Type *PointerType::getLLVMPointedType(GenerationContext *context) const
{
    if (this->managed)
    {
        std::vector<llvm::Type *> fields;
        fields.push_back(getRefCountType(*context->context));
        if (this->pointedType != NULL)
        {
            fields.push_back(this->pointedType->getLLVMType(context));
        }
        return llvm::StructType::get(*context->context, fields, false);
    }
    else
    {
        assert(this->pointedType != NULL);
        return this->pointedType->getLLVMType(context);
    }
}

llvm::Type *PointerType::getLLVMType(GenerationContext *context) const
{
    return llvm::PointerType::get(getLLVMPointedType(context), 0);
}

std::string PointerType::toString()
{
    std::string str = "";
    if (this->managed)
        str += "m";
    str += "*";
    if (this->pointedType != NULL)
    {
        str += this->pointedType->toString();
    }
    else
    {
        str += "Any";
    }
    return str;
}

llvm::Type *FunctionType::getLLVMType(GenerationContext *context) const
{
    std::vector<llvm::Type *> parameters;

    llvm::Type *returnType;
    if (this->returnType == NULL)
    {
        returnType = llvm::Type::getVoidTy(*context->context);
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

std::string FunctionType::toString()
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

PointerType *ArrayType::getArrayPointerType() const
{
    assert(!this->value);
    return new PointerType(new ArrayType(this->innerType, this->count, true, false), this->managed);
}

llvm::Type *ArrayType::getLLVMArrayPointerType(GenerationContext *context) const
{
    return this->getArrayPointerType()->getLLVMType(context);
}

llvm::StructType *ArrayType::getLLVMLengthStructType(GenerationContext *context) const
{
    assert(this->managed);

    std::vector<llvm::Type *> llvmLengthStructFields;
    llvmLengthStructFields.push_back(getLLVMLengthFieldType(context));
    llvmLengthStructFields.push_back(this->getLLVMArrayPointerType(context));
    return llvm::StructType::get(*context->context, llvmLengthStructFields, false);
}

llvm::Type *ArrayType::getLLVMType(GenerationContext *context) const
{
    if (this->value)
    {
        return llvm::ArrayType::get(this->innerType->getLLVMType(context), this->count < 0 ? 0 : this->count);
    }
    else
    {
        // Use a 0 sized array (can use getelemptr trick)
        auto llvmArrayType = llvm::ArrayType::get(this->innerType->getLLVMType(context), this->count < 0 ? 0 : this->count);
        if (this->managed)
        {
            return this->getLLVMLengthStructType(context);
        }
        else
        {
            return llvmArrayType->getPointerTo();
        }
    }
}

llvm::Type *ArrayType::getLLVMLengthFieldType(GenerationContext *context)
{
    return llvm::Type::getInt64Ty(*context->context);
}

std::string ArrayType::toString()
{
    std::string str = "[";
    str += this->innerType->toString();
    if (this->count >= 0)
    {
        str += " # ";
        str += std::to_string(this->count);
    }
    str += "]";
    return str;
}

bool StructType::operator==(const Type &b) const
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

llvm::Type *StructType::getLLVMType(GenerationContext *context) const
{
    std::vector<llvm::Type *> fieldTypes;
    for (auto &field : this->fields)
    {
        fieldTypes.push_back(field.type->getLLVMType(context));
    }
    if (this->name != "")
    {
        auto type = llvm::StructType::getTypeByName(*context->context, this->name);
        if (type == NULL)
        {
            type = llvm::StructType::create(fieldTypes, this->name, this->packed);
        }
        return type;
    }
    else
    {
        return llvm::StructType::get(*context->context, fieldTypes, this->packed);
    }
}

StructTypeField *StructType::getField(std::string name)
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

int StructType::getFieldIndex(std::string name)
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

int StructType::getMaxIndex()
{
    return this->fields.size();
}

std::string StructType::toString()
{
    if (this->name != "")
    {
        return this->name;
    }

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