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
            value = lazyValue->generateLLVM(context, scope);
            this->namedStatics[name] = value;
            return value;
        }
        else
        {
            return NULL;
        }
    }
}