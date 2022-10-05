#include "util.hpp"

const char *mallocName = "chocoAlloc";
const char *freeName = "chocoFree";

TypedValue *generateLoad(GenerationContext *context, TypedValue *valuePointer)
{
    if (valuePointer->getTypeCode() != TypeCode::POINTER)
    {
        std::cout << "ERROR: generateLoad(...) only accepts pointers\n";
        return NULL;
    }

    std::string originVariable = valuePointer->getOriginVariable();
    PointerType *pointerType = static_cast<PointerType *>(valuePointer->getType());
    llvm::Value *derefValue = context->irBuilder->CreateLoad(pointerType->getPointedType()->getLLVMType(*context->context), valuePointer->getValue(), originVariable + ".load");

    return new TypedValue(derefValue, pointerType->getPointedType(), originVariable);
}

// Generates a single dereference, decreasing/increasing pointer reference counts if needed
// T*** -> T**
// T* -> T
// T -> (ERROR)
TypedValue *generateReferenceAwareLoad(GenerationContext *context, TypedValue *valuePointer)
{
    if (valuePointer->getTypeCode() != TypeCode::POINTER)
    {
        std::cout << "ERROR: generateReferenceAwareLoad(...) only accepts pointers\n";
        return NULL;
    }

    // Pointer 'valueToConvert' will be dereferenced, decrease ref count
    if (!generateDecrementReferenceIfPointer(context, valuePointer, false))
    {
        std::cout << "ERROR: generateReferenceAwareLoad(...) could not decrement pointer\n";
        return NULL;
    }

    valuePointer = generateLoad(context, valuePointer);

    // Pointer 'valueToConvert' just got loaded, increase ref count
    if (!generateIncrementReferenceIfPointer(context, valuePointer))
    {
        std::cout << "ERROR: generateReferenceAwareLoad(...) could not increment pointer\n";
        return NULL;
    }

    return valuePointer;
}

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

        currentValue = generateReferenceAwareLoad(context, currentValue);
        if (currentValue == NULL)
        {
            std::cout << "ERROR: Could not generateDereferenceToPointer(...), generateReferenceAwareLoad(...) error\n";
            return NULL;
        }
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
        currentValue = generateReferenceAwareLoad(context, currentValue);
        if (currentValue == NULL)
        {
            std::cout << "ERROR: Could not generateDereferenceToValue(...), generateReferenceAwareLoad(...) error\n";
            return NULL;
        }
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

llvm::Type *getRefCountType(llvm::LLVMContext &context)
{
    return llvm::IntegerType::getInt64Ty(context);
}

bool generateCallFreeFunction(GenerationContext *context, TypedValue *managedPointer)
{
    if (managedPointer->getTypeCode() != TypeCode::POINTER)
    {
        std::cout << "WARNING: Cannot generateFreeFunction(...) a non pointer (" << managedPointer->getType()->toString() << ")\n";
        return false;
    }
    PointerType *pointerType = static_cast<PointerType *>(managedPointer->getType());
    if (!pointerType->isManaged())
    {
        std::cout << "WARNING: Cannot generateFreeFunction(...) an unmanaged pointer (" << managedPointer->getType()->toString() << ")\n";
        return false;
    }

    std::cout << "INFO: Generate free\n";

    llvm::Function *freeFunction;
    llvm::Type *llvmTypeToFree = pointerType->getLLVMPointedType(*context->context);
    if (context->freeFunctions.count(llvmTypeToFree) > 0)
    {
        // Free function already generated
        freeFunction = context->freeFunctions[llvmTypeToFree];
    }
    else
    {
        // Need to generate function
        std::vector<llvm::Type *> freeParams;
        freeParams.push_back(pointerType->getLLVMType(*context->context));
        llvm::FunctionType *functionType = llvm::FunctionType::get(llvm::Type::getVoidTy(*context->context), freeParams, false);
        freeFunction = llvm::Function::Create(functionType, llvm::Function::InternalLinkage, std::to_string(context->freeFunctions.size()) + ".free", *context->module);

        context->freeFunctions[llvmTypeToFree] = freeFunction;

        auto savedBlock = context->irBuilder->GetInsertBlock();

        llvm::BasicBlock *freeFunctionBlock = llvm::BasicBlock::Create(*context->context, "free.entry", freeFunction);
        context->irBuilder->SetInsertPoint(freeFunctionBlock);
        auto pointerArg = freeFunction->getArg(0);

        if (pointerType->getPointedType()->getTypeCode() == TypeCode::STRUCT)
        {
            StructType *structType = static_cast<StructType *>(pointerType->getPointedType());
            for (auto &field : structType->getFields())
            {
                if (field.type->getTypeCode() == TypeCode::POINTER)
                {
                    PointerType *structFieldType = static_cast<PointerType *>(field.type);
                    if (structFieldType->isManaged())
                    {
                        // Call freer function
                        int fieldIndex = structType->getFieldIndex(field.name);

                        std::vector<llvm::Value *> indices;
                        indices.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context->context), 0));
                        indices.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context->context), 1));
                        indices.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context->context), fieldIndex));
                        auto fieldPointer = context->irBuilder->CreateGEP(pointerType->getLLVMPointedType(*context->context), pointerArg, indices, "member.free");
                        auto fieldValue = context->irBuilder->CreateLoad(structFieldType->getLLVMType(*context->context), fieldPointer, "member.free.load");

                        if (!generateDecrementReference(context, new TypedValue(fieldValue, structFieldType, field.name), true))
                        {
                            return false;
                        }
                    }
                }
            }
        }

        generateFree(context, pointerArg, "free");

        context->irBuilder->CreateRetVoid();

        context->irBuilder->SetInsertPoint(savedBlock);

        if (llvm::verifyFunction(*freeFunction, &llvm::errs()))
        {
            std::cout << "ERROR: LLVM reported invalid free function\n";
            return false;
        }
    }

    std::vector<llvm::Value *> params;
    params.push_back(managedPointer->getValue());
    context->irBuilder->CreateCall(freeFunction, params);
    return true;
}

bool generateDecrementReference(GenerationContext *context, TypedValue *managedPointer, bool checkFree)
{
    std::cout << "DEBUG: generateDecrementReference\n";

    if (managedPointer->getTypeCode() != TypeCode::POINTER)
    {
        std::cout << "WARNING: Cannot generateDecrementReference(...) a non pointer (" << managedPointer->getType()->toString() << ")\n";
        return false;
    }
    PointerType *pointerType = static_cast<PointerType *>(managedPointer->getType());
    if (!pointerType->isManaged())
    {
        std::cout << "WARNING: Cannot generateDecrementReference(...) an unmanaged pointer (" << managedPointer->getType()->toString() << ")\n";
        return false;
    }

    std::string twine = managedPointer->getOriginVariable();

    std::vector<llvm::Value *> indices;
    indices.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context->context), 0, false));
    indices.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context->context), 0, false));
    llvm::Value *refCountPointer = context->irBuilder->CreateGEP(pointerType->getLLVMPointedType(*context->context), managedPointer->getValue(), indices, twine + ".refcount.ptr");

    llvm::Value *refCount = context->irBuilder->CreateLoad(getRefCountType(*context->context), refCountPointer, twine + ".refcount");
    // Decrease ref count by 1
    refCount = context->irBuilder->CreateSub(refCount, llvm::ConstantInt::get(getRefCountType(*context->context), 1, false), twine + ".refcount.dec");
    context->irBuilder->CreateStore(refCount, refCountPointer, false);

    // Free the block if refCount is zero
    if (checkFree)
    {
        std::cout << "DEBUG: generate free\n";
        llvm::Value *isRefZero = context->irBuilder->CreateICmpEQ(refCount, llvm::ConstantInt::get(getRefCountType(*context->context), 0, false), twine + ".refcount.dec.cmp");

        llvm::Function *currentFunction = context->irBuilder->GetInsertBlock()->getParent();
        llvm::BasicBlock *freeBlock = llvm::BasicBlock::Create(*context->context, twine + ".free", currentFunction);
        llvm::BasicBlock *continueBlock = llvm::BasicBlock::Create(*context->context, twine + ".nofree", currentFunction);

        context->irBuilder->CreateCondBr(isRefZero, freeBlock, continueBlock);

        context->irBuilder->SetInsertPoint(freeBlock);

        // Also decrement pointers for nested pointers
        generateCallFreeFunction(context, managedPointer);

        context->irBuilder->CreateBr(continueBlock);

        context->irBuilder->SetInsertPoint(continueBlock);
    }

    std::cout << "DEBUG: generateDecrementReference end\n";
    return true;
}

bool generateIncrementReference(GenerationContext *context, TypedValue *managedPointer)
{
    std::cout << "DEBUG: generateIncrementReference\n";
    if (managedPointer->getTypeCode() != TypeCode::POINTER)
    {
        std::cout << "WARNING: Cannot generateIncrementReference(...) a non pointer (" << managedPointer->getType()->toString() << ")\n";
        return false;
    }
    PointerType *pointerType = static_cast<PointerType *>(managedPointer->getType());
    if (!pointerType->isManaged())
    {
        std::cout << "WARNING: Cannot generateIncrementReference(...) an unmanaged pointer (" << managedPointer->getType()->toString() << ")\n";
        return false;
    }

    std::string twine = managedPointer->getOriginVariable();

    std::vector<llvm::Value *> indices;
    indices.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context->context), 0, false));
    indices.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context->context), 0, false));
    llvm::Value *refCountPointer = context->irBuilder->CreateGEP(pointerType->getLLVMPointedType(*context->context), managedPointer->getValue(), indices, twine + ".refcount.ptr");

    llvm::Value *refCount = context->irBuilder->CreateLoad(getRefCountType(*context->context), refCountPointer, twine + ".refcount");
    // Increase refCount by 1
    refCount = context->irBuilder->CreateAdd(refCount, llvm::ConstantInt::get(getRefCountType(*context->context), 1, false), twine + ".refcount.inc");
    context->irBuilder->CreateStore(refCount, refCountPointer, false);
    std::cout << "DEBUG: generateIncrementReference end\n";
    return true;
}

bool generateDecrementReferenceIfPointer(GenerationContext *context, TypedValue *maybeManagedPointer, bool checkFree)
{
    if (maybeManagedPointer->getTypeCode() == TypeCode::POINTER)
    {
        // Increase reference count for loaded pointer
        PointerType *loadedPointerType = static_cast<PointerType *>(maybeManagedPointer->getType());
        if (loadedPointerType->isManaged())
        {
            if (!generateDecrementReference(context, maybeManagedPointer, checkFree))
            {
                std::cout << "ERROR: Could not generate reference counting decrease code\n";
                return false;
            }
        }
    }
    return true;
}

bool generateIncrementReferenceIfPointer(GenerationContext *context, TypedValue *maybeManagedPointer)
{
    if (maybeManagedPointer->getTypeCode() == TypeCode::POINTER)
    {
        // Increase reference count for loaded pointer
        PointerType *loadedPointerType = static_cast<PointerType *>(maybeManagedPointer->getType());
        if (loadedPointerType->isManaged())
        {
            if (!generateIncrementReference(context, maybeManagedPointer))
            {
                std::cout << "ERROR: Could not generate reference counting increase code\n";
                return false;
            }
        }
    }
    return true;
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
        while (1)
        {
            if (valueToConvert->getTypeCode() != TypeCode::POINTER)
            {
                std::cout << "ERROR: Cannot convert " << valueToConvert->getType()->toString() << " to a pointer (" << targetType->toString() << ")\n";
                return NULL;
            }

            PointerType *pointerToConvert = static_cast<PointerType *>(valueToConvert->getType());

            valueToConvert = generateReferenceAwareLoad(context, valueToConvert);
            if (valueToConvert == NULL)
            {
                std::cout << "ERROR: Could not generateTypeConversion(...), generateReferenceAwareLoad(...) error\n";
                return NULL;
            }

            if (*valueToConvert->getType() == *targetType)
            {
                return valueToConvert;
            }
        }
    }

    valueToConvert = generateDereferenceToValue(context, valueToConvert);
    if (*valueToConvert->getType() == *targetType)
    {
        return valueToConvert;
    }

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

    auto convertedValue = generateTypeConversion(context, newValue, valuePointerType->getPointedType(), false);
    if (convertedValue == NULL)
    {
        std::cout << "ERROR: Cannot assign " << newValue->getType()->toString() << " to " << valuePointer->getType()->toString() << " ('" << valuePointer->getOriginVariable() << "')\n";
        return false;
    }
    context->irBuilder->CreateStore(convertedValue->getValue(), valuePointer->getValue(), isVolatile);
    return true;
}

llvm::Value *generateSizeOf(GenerationContext *context, llvm::Type *type, std::string twine)
{
    auto fakePointer = context->irBuilder->CreateGEP(type, llvm::ConstantPointerNull::get(type->getPointerTo()), llvm::ConstantInt::get(llvm::IntegerType::getInt32Ty(*context->context), llvm::APInt(32, 1)), twine + ".sizeof");
    return context->irBuilder->CreatePtrToInt(fakePointer, llvm::Type::getInt64Ty(*context->context), twine + ".sizeof.int");
}

llvm::Value *generateMalloc(GenerationContext *context, llvm::Type *type, std::string twine)
{
    llvm::Function *mallocFunction = context->module->getFunction(mallocName);
    if (mallocFunction == NULL)
    {
        std::cout << "INFO: Declare malloc\n";
        std::vector<llvm::Type *> mallocParams;
        mallocParams.push_back(llvm::Type::getInt64Ty(*context->context));
        llvm::FunctionType *functionType = llvm::FunctionType::get(llvm::PointerType::get(*context->context, 0), mallocParams, false);
        mallocFunction = llvm::Function::Create(functionType, llvm::Function::ExternalLinkage, mallocName, *context->module);
    }

    std::vector<llvm::Value *> parameters;
    llvm::Value *sizeOf = generateSizeOf(context, type, twine);
    parameters.push_back(sizeOf);
    auto opaquePointer = context->irBuilder->CreateCall(mallocFunction, parameters, twine + ".malloc.ptr.opaque");
    return context->irBuilder->CreateBitCast(opaquePointer, llvm::PointerType::get(type, 0), twine + ".malloc.ptr");
}

llvm::Value *generateFree(GenerationContext *context, llvm::Value *toFree, std::string twine)
{
    llvm::Function *freeFunction = context->module->getFunction(freeName);
    if (freeFunction == NULL)
    {
        std::vector<llvm::Type *> freeParams;
        freeParams.push_back(llvm::PointerType::get(*context->context, 0));
        llvm::FunctionType *functionType = llvm::FunctionType::get(llvm::Type::getVoidTy(*context->context), freeParams, false);
        freeFunction = llvm::Function::Create(functionType, llvm::Function::ExternalLinkage, freeName, *context->module);
    }

    auto opaquePointer = context->irBuilder->CreateBitCast(toFree, llvm::PointerType::get(*context->context, 0), twine + ".opaque");
    std::vector<llvm::Value *> parameters;
    parameters.push_back(opaquePointer);
    return context->irBuilder->CreateCall(freeFunction, parameters);
}

llvm::AllocaInst *generateAllocaInCurrentFunction(GenerationContext *context, llvm::Type *type, llvm::StringRef twine)
{
    llvm::Function *function = context->irBuilder->GetInsertBlock()->getParent();
    llvm::IRBuilder<> insertBuilder(&function->getEntryBlock(), function->getEntryBlock().begin());
    return insertBuilder.CreateAlloca(type, NULL, twine + ".alloca.ptr");
}