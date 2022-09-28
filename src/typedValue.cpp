#include "typedValue.hpp"
#include "ast.hpp"

IntegerType BYTE_TYPE(8, false);
IntegerType CHAR_TYPE(8, false);
IntegerType BOOL_TYPE(1, false);
IntegerType UINT32_TYPE(32, false);

Type *Type::getPointerToType(bool byValue)
{
    return new PointerType(this, byValue);
}

TypedValue *ModuleType::getValue(std::string name, GenerationContext *context, FunctionScope *scope)
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
            auto savedBlock = context->irBuilder->GetInsertBlock();
            FunctionType *savedCurrentFunction = context->currentFunction;
            TypedValue *savedSRet = context->currentFunctionSRet;
            value = lazyValue->generateLLVM(context, scope, NULL);
            context->currentFunctionSRet = savedSRet;
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