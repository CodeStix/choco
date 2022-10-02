#include "util.hpp"

// Generates code that converts a deeply nested pointer to a single-deep pointer
// T*** -> T*
// T* -> T*
// T -> (ERROR)
// T***** -> T*
TypedValue *generateDereferenceToPointer(GenerationContext *context, TypedValue *currentValue)
{
    if (currentValue->getType()->getTypeCode() != TypeCode::POINTER)
    {
        std::cout << "ERROR: cannot generateDereferenceToPointer(...) to pointer when its not a pointer\n";
        return NULL;
    }

    while (1)
    {
        PointerType *currentPointerType = static_cast<PointerType *>(currentValue->getType());
        if (currentPointerType->getPointedType()->getTypeCode() != TypeCode::POINTER)
        {
            // Stop when a value pointer has been reached
            break;
        }

        llvm::Value *derefValue = context->irBuilder->CreateLoad(currentPointerType->getPointedType()->getLLVMType(*context->context), currentValue->getValue(), "deref");
        std::string originVariable = currentValue->getOriginVariable();
        currentValue = new TypedValue(derefValue, currentPointerType->getPointedType());
        currentValue->setOriginVariable(originVariable);
    }

    return currentValue;
}

// Generates code that converts a deeply nested pointer to a value
// T*** -> T
// T* -> T
// T -> T
// T***** -> T
TypedValue *generateDereferenceToValue(GenerationContext *context, TypedValue *currentValue)
{
    while (currentValue->getTypeCode() == TypeCode::POINTER)
    {
        PointerType *currentPointerType = static_cast<PointerType *>(currentValue->getType());
        llvm::Value *newValue = context->irBuilder->CreateLoad(currentPointerType->getPointedType()->getLLVMType(*context->context), currentValue->getValue(), "deref");
        std::string originVariable = currentValue->getOriginVariable();
        currentValue = new TypedValue(newValue, currentPointerType->getPointedType());
        currentValue->setOriginVariable(originVariable);
    }

    return currentValue;
}

// Converts the left or right value to match the other one's type without losing precision
bool generateTypeJugging(GenerationContext *context, TypedValue **leftInOut, TypedValue **rightInOut)
{
    if (*(*leftInOut)->getType() == *(*rightInOut)->getType())
    {
        return true;
    }

    *leftInOut = generateDereferenceToValue(context, *leftInOut);
    *rightInOut = generateDereferenceToValue(context, *rightInOut);

    auto leftType = (*leftInOut)->getType();
    auto leftValue = (*leftInOut)->getValue();
    auto rightType = (*rightInOut)->getType();
    auto rightValue = (*rightInOut)->getValue();

    if (leftType->getTypeCode() == TypeCode::INTEGER && rightType->getTypeCode() == TypeCode::INTEGER)
    {
        auto leftIntType = static_cast<IntegerType *>(leftType);
        auto rightIntType = static_cast<IntegerType *>(rightType);

        if (leftIntType->getBitSize() > rightIntType->getBitSize())
        {
            // Right must be converted to match left int size
            if (leftIntType->getSigned() && rightIntType->getSigned())
            {
                rightValue = context->irBuilder->CreateSExt(rightValue, leftIntType->getLLVMType(*context->context), "jugglesext");
            }
            else
            {
                rightValue = context->irBuilder->CreateZExt(rightValue, leftIntType->getLLVMType(*context->context), "jugglezext");
            }
            *rightInOut = new TypedValue(rightValue, leftIntType);
        }
        else if (leftIntType->getBitSize() < rightIntType->getBitSize())
        {
            // Left must be converted to match right int size
            if (leftIntType->getSigned() && rightIntType->getSigned())
            {
                leftValue = context->irBuilder->CreateSExt(leftValue, rightIntType->getLLVMType(*context->context), "jugglesext");
            }
            else
            {
                leftValue = context->irBuilder->CreateZExt(leftValue, rightIntType->getLLVMType(*context->context), "jugglezext");
            }
            *leftInOut = new TypedValue(leftValue, rightIntType);
        }

        return true;
    }
    else if (leftType->getTypeCode() == TypeCode::INTEGER && rightType->getTypeCode() == TypeCode::FLOAT)
    {
        auto leftIntType = static_cast<IntegerType *>(leftType);
        auto rightFloatType = static_cast<FloatType *>(rightType);

        // TODO: check if float can fit integer precision

        if (leftIntType->getSigned())
        {
            leftValue = context->irBuilder->CreateSIToFP(leftValue, rightFloatType->getLLVMType(*context->context), "jugglefp");
        }
        else
        {
            leftValue = context->irBuilder->CreateUIToFP(leftValue, rightFloatType->getLLVMType(*context->context), "jugglefp");
        }

        *leftInOut = new TypedValue(leftValue, rightFloatType);
        return true;
    }
    else if (leftType->getTypeCode() == TypeCode::FLOAT && rightType->getTypeCode() == TypeCode::INTEGER)
    {
        auto leftFloatType = static_cast<FloatType *>(leftType);
        auto rightIntType = static_cast<IntegerType *>(rightType);

        // TODO: check if float can fit integer precision

        if (rightIntType->getSigned())
        {
            rightValue = context->irBuilder->CreateSIToFP(rightValue, leftFloatType->getLLVMType(*context->context), "jugglefp");
        }
        else
        {
            rightValue = context->irBuilder->CreateUIToFP(rightValue, leftFloatType->getLLVMType(*context->context), "jugglefp");
        }

        *rightInOut = new TypedValue(rightValue, leftFloatType);
        return true;
    }
    else if (leftType->getTypeCode() == TypeCode::FLOAT && rightType->getTypeCode() == TypeCode::FLOAT)
    {
        auto leftFloatType = static_cast<FloatType *>(leftType);
        auto rightFloatType = static_cast<FloatType *>(rightType);

        if (leftFloatType->getBitSize() > rightFloatType->getBitSize())
        {
            rightValue = context->irBuilder->CreateFPExt(rightValue, leftFloatType->getLLVMType(*context->context), "jugglefpext");
            *rightInOut = new TypedValue(rightValue, leftFloatType);
        }
        else if (leftFloatType->getBitSize() < rightFloatType->getBitSize())
        {
            leftValue = context->irBuilder->CreateFPExt(leftValue, rightFloatType->getLLVMType(*context->context), "jugglefpext");
            *leftInOut = new TypedValue(leftValue, rightFloatType);
        }

        return true;
    }
    else
    {
        return false;
    }
}

// Try to cast a value to a specific type
TypedValue *generateTypeConversion(GenerationContext *context, TypedValue *valueToConvert, Type *targetType, bool allowLosePrecision)
{
    if (*valueToConvert->getType() == *targetType)
    {
        return valueToConvert;
    }

    if (targetType->getTypeCode() == TypeCode::POINTER)
    {
        // Try converting to a single-deep pointer and try again
        TypedValue *currentValue = generateDereferenceToPointer(context, valueToConvert);
        if (currentValue != NULL && *currentValue->getType() == *targetType)
        {
            return currentValue;
        }
        else
        {
            std::cout << "ERROR: Cannot convert " << valueToConvert->getType()->toString() << " to " << targetType->toString() << "\n";
            return NULL;
        }
    }

    valueToConvert = generateDereferenceToValue(context, valueToConvert);
    llvm::Value *currentValue = valueToConvert->getValue();
    Type *currentType = valueToConvert->getType();

    if (targetType->getTypeCode() == TypeCode::INTEGER && currentType->getTypeCode() == TypeCode::INTEGER)
    {
        IntegerType *currentIntType = static_cast<IntegerType *>(currentType);
        IntegerType *targetIntType = static_cast<IntegerType *>(targetType);

        if (targetIntType->getBitSize() > currentIntType->getBitSize())
        {
            // Target type has more bits, this cast may be implicit
            if (targetIntType->getSigned() == currentIntType->getSigned())
            {
                // Target type has the same signedness, this cast may be implicit
                if (targetIntType->getSigned())
                {
                    currentValue = context->irBuilder->CreateZExt(currentValue, targetIntType->getLLVMType(*context->context), "convzextint");
                }
                else
                {
                    currentValue = context->irBuilder->CreateSExt(currentValue, targetIntType->getLLVMType(*context->context), "convzextint");
                }
            }
            else
            {
                if (!allowLosePrecision)
                {
                    std::cout << "ERROR: Cannot implicitly convert integers of size " << currentIntType->getBitSize() << " and " << targetIntType->getBitSize() << "\n";
                    return NULL;
                }

                // TODO: is this the right was to convert
                if (targetIntType->getSigned())
                {
                    currentValue = context->irBuilder->CreateZExt(currentValue, targetIntType->getLLVMType(*context->context), "convzextint");
                }
                else
                {
                    currentValue = context->irBuilder->CreateSExt(currentValue, targetIntType->getLLVMType(*context->context), "convzextint");
                }
            }
        }
        else if (targetIntType->getBitSize() < currentIntType->getBitSize())
        {
            if (!allowLosePrecision)
            {
                std::cout << "ERROR: Cannot implicitly convert integers of size " << currentIntType->getBitSize() << " and " << targetIntType->getBitSize() << "\n";
                return NULL;
            }

            currentValue = context->irBuilder->CreateTrunc(currentValue, targetIntType->getLLVMType(*context->context), "convtruncint");
        }
    }
    else if (targetType->getTypeCode() == TypeCode::FLOAT && currentType->getTypeCode() == TypeCode::FLOAT)
    {
        FloatType *currentFloatType = static_cast<FloatType *>(currentType);
        FloatType *targetFloatType = static_cast<FloatType *>(targetType);

        if (targetFloatType->getBitSize() > currentFloatType->getBitSize())
        {
            currentValue = context->irBuilder->CreateFPExt(currentValue, targetFloatType->getLLVMType(*context->context), "convfpext");
        }
        else if (targetFloatType->getBitSize() < currentFloatType->getBitSize())
        {
            if (!allowLosePrecision)
            {
                std::cout << "ERROR: Cannot implicitly convert floats of size " << currentFloatType->getBitSize() << " and " << targetFloatType->getBitSize() << "\n";
                return NULL;
            }

            currentValue = context->irBuilder->CreateFPTrunc(currentValue, targetFloatType->getLLVMType(*context->context), "convtruncfp");
        }
    }
    else if (targetType->getTypeCode() == TypeCode::FLOAT && currentType->getTypeCode() == TypeCode::INTEGER)
    {
        // TODO: this operation can be done without losing precision in some cases
        if (!allowLosePrecision)
        {
            std::cout << "ERROR: Cannot implicitly convert integers to floats\n";
            return NULL;
        }

        IntegerType *currentIntType = static_cast<IntegerType *>(currentType);
        FloatType *targetFloatType = static_cast<FloatType *>(targetType);
        if (currentIntType->getSigned())
        {
            currentValue = context->irBuilder->CreateSIToFP(currentValue, targetFloatType->getLLVMType(*context->context), "convsitofp");
        }
        else
        {
            currentValue = context->irBuilder->CreateUIToFP(currentValue, targetFloatType->getLLVMType(*context->context), "convuitofp");
        }
    }
    else if (targetType->getTypeCode() == TypeCode::INTEGER && currentType->getTypeCode() == TypeCode::FLOAT)
    {
        if (!allowLosePrecision)
        {
            std::cout << "ERROR: Cannot implicitly convert floats to integers\n";
            return NULL;
        }

        FloatType *currentFloatType = static_cast<FloatType *>(currentType);
        IntegerType *targetIntType = static_cast<IntegerType *>(targetType);

        if (targetIntType->getSigned())
        {
            currentValue = context->irBuilder->CreateFPToSI(currentValue, targetIntType->getLLVMType(*context->context), "convfptosi");
        }
        else
        {
            currentValue = context->irBuilder->CreateFPToUI(currentValue, targetIntType->getLLVMType(*context->context), "convfptoui");
        }
    }
    else
    {
        // Cannot convert type automatically
        std::cout << "ERROR: Cannot convert " << currentType->toString() << " to " << targetType->toString() << "\n";
        return NULL;
    }

    return new TypedValue(currentValue, targetType);
}

bool generateAssignment(GenerationContext *context, TypedValue *valuePointer, TypedValue *newValue, bool isVolatile)
{
    std::cout << "debug: Assign " << newValue->getType()->toString() << " to " << valuePointer->getType()->toString() << " ('" << valuePointer->getOriginVariable() << "')\n";

    if (valuePointer->getTypeCode() != TypeCode::POINTER)
    {
        std::cout << "ERROR: Can only assign to a pointer\n";
        return false;
    }
    PointerType *valuePointerType = static_cast<PointerType *>(valuePointer->getType());

    if (newValue->getTypeCode() != TypeCode::POINTER)
    {
        // T* = T
        auto convertedValue = generateTypeConversion(context, newValue, valuePointerType->getPointedType(), false);
        if (convertedValue == NULL)
        {
            std::cout << "ERROR: Cannot assign " << newValue->getType()->toString() << " to " << valuePointer->getType()->toString() << " ('" << valuePointer->getOriginVariable() << "')\n";
            return false;
        }
        context->irBuilder->CreateStore(convertedValue->getValue(), valuePointer->getValue(), isVolatile);
    }
    else
    {
        PointerType *newValuePointerType = static_cast<PointerType *>(newValue->getType());
        if (newValuePointerType->getPointedType()->getTypeCode() != TypeCode::POINTER && valuePointerType->getPointedType()->getTypeCode() != TypeCode::POINTER)
        {
            // T* = T*
            auto convertedValue = generateTypeConversion(context, newValue, valuePointerType, false);
            if (convertedValue == NULL)
            {
                std::cout << "ERROR: Cannot assign (copy) " << newValue->getType()->toString() << " to " << valuePointer->getType()->toString() << " ('" << valuePointer->getOriginVariable() << "')\n";
                return false;
            }

            llvm::Type *typeToCopy = valuePointerType->getPointedType()->getLLVMType(*context->context);
            auto typeSize = generateSizeOf(context, typeToCopy);
            // context->irBuilder->CreateMemCpyInline(valuePointer->getValue(), llvm::MaybeAlign(8), convertedValue->getValue(), llvm::MaybeAlign(8), typeSize, isVolatile);
            context->irBuilder->CreateMemCpy(valuePointer->getValue(), llvm::MaybeAlign(8), convertedValue->getValue(), llvm::MaybeAlign(8), typeSize, isVolatile);
        }
        else
        {
            // T** = T*
            auto convertedValue = generateTypeConversion(context, newValue, valuePointerType->getPointedType(), false);
            if (convertedValue == NULL)
            {
                std::cout << "ERROR: Cannot assign " << newValue->getType()->toString() << " to " << valuePointer->getType()->toString() << " ('" << valuePointer->getOriginVariable() << "')\n";
                return false;
            }
            context->irBuilder->CreateStore(convertedValue->getValue(), valuePointer->getValue(), isVolatile);
        }
    }

    return true;
}

llvm::Value *generateSizeOf(GenerationContext *context, llvm::Type *type)
{
    auto fakePointer = context->irBuilder->CreateGEP(type, llvm::ConstantPointerNull::get(type->getPointerTo()), llvm::ConstantInt::get(llvm::IntegerType::getInt32Ty(*context->context), llvm::APInt(32, 1)), "sizeof");
    return context->irBuilder->CreatePtrToInt(fakePointer, llvm::Type::getInt32Ty(*context->context), "sizeoftoint");
}

llvm::Value *generateMalloc(GenerationContext *context, llvm::Type *type)
{
    llvm::Function *mallocFunction = context->module->getFunction("malloc");
    if (mallocFunction == NULL)
    {
        std::cout << "ERROR: Cannot generate 'malloc' because the allocator function wasn't found\n";
        return NULL;
    }

    std::vector<llvm::Value *> parameters;
    llvm::Value *sizeOf = generateSizeOf(context, type);
    parameters.push_back(sizeOf);
    return context->irBuilder->CreateCall(mallocFunction, parameters, "malloc");
}

llvm::Value *generateFree(GenerationContext *context, llvm::Value *toFree)
{
    llvm::Function *freeFunction = context->module->getFunction("free");
    if (freeFunction == NULL)
    {
        std::cout << "ERROR: Cannot generate 'free' because the allocator function wasn't found\n";
        return NULL;
    }

    std::vector<llvm::Value *> parameters;
    parameters.push_back(toFree);
    return context->irBuilder->CreateCall(freeFunction, parameters, "malloc");
}

llvm::AllocaInst *generateAllocaInCurrentFunction(GenerationContext *context, llvm::Type *type, llvm::StringRef twine)
{
    llvm::Function *function = context->irBuilder->GetInsertBlock()->getParent();
    llvm::IRBuilder<> insertBuilder(&function->getEntryBlock(), function->getEntryBlock().begin());
    return insertBuilder.CreateAlloca(type, NULL, twine);
}