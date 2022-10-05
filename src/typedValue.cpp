#include "typedValue.hpp"
#include "ast.hpp"

IntegerType BYTE_TYPE(8, false);
IntegerType CHAR_TYPE(8, false);
IntegerType BOOL_TYPE(1, false);
IntegerType UINT32_TYPE(32, false);

PointerType *Type::getUnmanagedPointerToType(bool byValue)
{
    return new PointerType(this, byValue, false);
}

PointerType *Type::getManagedPointerToType()
{
    return new PointerType(this, false, true);
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
            auto value = lazyValue->generateLLVM(context, NULL, NULL);
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