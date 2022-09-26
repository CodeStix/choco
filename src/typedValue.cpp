#include "typedValue.hpp"
#include "ast.hpp"

IntegerType BYTE_TYPE(8, false);
IntegerType CHAR_TYPE(8, false);
IntegerType BOOL_TYPE(1, false);

Type *Type::getPointerToType()
{
    return new PointerType(this);
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
            value = lazyValue->generateLLVM(context, scope);
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