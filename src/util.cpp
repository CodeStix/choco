#include "util.hpp"
#include "typedValue.hpp"
#include "context.hpp"

const char *mallocName = "chocoAlloc";
const char *freeName = "chocoFree";
const char *panicName = "chocoPanic";

TypedValue *generateLoad(GenerationContext *context, TypedValue *valuePointer)
{
    if (valuePointer->getTypeCode() != TypeCode::POINTER)
    {
        std::cout << "ERROR: generateLoad(...) only accepts pointers\n";
        return NULL;
    }

    std::string originVariable = valuePointer->getOriginVariable();
    PointerType *pointerType = static_cast<PointerType *>(valuePointer->getType());
    llvm::Value *derefValue = context->irBuilder->CreateLoad(pointerType->getPointedType()->getLLVMType(context), valuePointer->getValue(), originVariable + ".load");

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
    generateDecrementReferenceIfPointer(context, valuePointer, false);

    valuePointer = generateLoad(context, valuePointer);

    // Pointer 'valueToConvert' just got loaded, increase ref count
    generateIncrementReferenceIfPointer(context, valuePointer);

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
                rightValue = context->irBuilder->CreateSExt(rightValue, leftIntType->getLLVMType(context), "jugglesext");
            }
            else
            {
                rightValue = context->irBuilder->CreateZExt(rightValue, leftIntType->getLLVMType(context), "jugglezext");
            }
            *rightInOut = new TypedValue(rightValue, leftIntType);
        }
        else if (leftIntType->getBitSize() < rightIntType->getBitSize())
        {
            // Left must be converted to match right int size
            if (leftIntType->getSigned() && rightIntType->getSigned())
            {
                leftValue = context->irBuilder->CreateSExt(leftValue, rightIntType->getLLVMType(context), "jugglesext");
            }
            else
            {
                leftValue = context->irBuilder->CreateZExt(leftValue, rightIntType->getLLVMType(context), "jugglezext");
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
            leftValue = context->irBuilder->CreateSIToFP(leftValue, rightFloatType->getLLVMType(context), "jugglefp");
        }
        else
        {
            leftValue = context->irBuilder->CreateUIToFP(leftValue, rightFloatType->getLLVMType(context), "jugglefp");
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
            rightValue = context->irBuilder->CreateSIToFP(rightValue, leftFloatType->getLLVMType(context), "jugglefp");
        }
        else
        {
            rightValue = context->irBuilder->CreateUIToFP(rightValue, leftFloatType->getLLVMType(context), "jugglefp");
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
            rightValue = context->irBuilder->CreateFPExt(rightValue, leftFloatType->getLLVMType(context), "jugglefpext");
            *rightInOut = new TypedValue(rightValue, leftFloatType);
        }
        else if (leftFloatType->getBitSize() < rightFloatType->getBitSize())
        {
            leftValue = context->irBuilder->CreateFPExt(leftValue, rightFloatType->getLLVMType(context), "jugglefpext");
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

llvm::Type *getUnionIdType(llvm::LLVMContext &context)
{
    return llvm::IntegerType::getInt64Ty(context);
}

void generateCallFreeFunction(GenerationContext *context, TypedValue *managedPointer)
{
    assert(managedPointer->getTypeCode() == TypeCode::POINTER && "generateCallFreeFunction arg must be pointer");

    PointerType *pointerType = static_cast<PointerType *>(managedPointer->getType());
    assert(pointerType->isManaged() && "generateCallFreeFunction pointer arg must be managed");

    llvm::Function *freeFunction;
    llvm::Type *llvmTypeToFree = pointerType->getLLVMPointedType(context);
    if (context->freeFunctions.count(llvmTypeToFree) > 0)
    {
        // Free function already generated
        freeFunction = context->freeFunctions[llvmTypeToFree];
    }
    else
    {
        // Need to generate function
        std::vector<llvm::Type *> freeParams;
        freeParams.push_back(pointerType->getLLVMType(context));
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
                int fieldIndex = structType->getFieldIndex(field.name);

                std::vector<llvm::Value *> indices;
                indices.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context->context), 0));
                indices.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context->context), 1));
                indices.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context->context), fieldIndex));
                auto fieldPointer = context->irBuilder->CreateGEP(pointerType->getLLVMPointedType(context), pointerArg, indices, "member.free");
                auto fieldValue = context->irBuilder->CreateLoad(field.type->getLLVMType(context), fieldPointer, "member.free.load");

                generateDecrementReferenceIfPointer(context, new TypedValue(fieldValue, field.type, field.name), true);
            }
        }

        generateFree(context, pointerArg, "free");

        context->irBuilder->CreateRetVoid();

        context->irBuilder->SetInsertPoint(savedBlock);

        assert(!llvm::verifyFunction(*freeFunction, &llvm::errs()));
    }

    std::vector<llvm::Value *> params;
    params.push_back(managedPointer->getValue());
    context->irBuilder->CreateCall(freeFunction, params);
}

void generateDecrementReference(GenerationContext *context, TypedValue *managedPointer, bool checkFree)
{
    assert(managedPointer->getTypeCode() == TypeCode::POINTER && "generateDecrementReference arg must be pointer");

    PointerType *pointerType = static_cast<PointerType *>(managedPointer->getType());
    assert(pointerType->isManaged() && "generateDecrementReference pointer arg must be managed");

    std::string twine = managedPointer->getOriginVariable();

    std::vector<llvm::Value *> indices;
    indices.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context->context), 0, false));
    indices.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context->context), 0, false));
    llvm::Value *refCountPointer = context->irBuilder->CreateGEP(pointerType->getLLVMPointedType(context), managedPointer->getValue(), indices, twine + ".refcount.ptr");

    llvm::Value *refCount = context->irBuilder->CreateLoad(getRefCountType(*context->context), refCountPointer, twine + ".refcount");
    // Decrease ref count by 1
    refCount = context->irBuilder->CreateSub(refCount, llvm::ConstantInt::get(getRefCountType(*context->context), 1, false), twine + ".refcount.dec", true, true);
    context->irBuilder->CreateStore(refCount, refCountPointer, false);

    // Free the block if refCount is zero
    if (checkFree)
    {
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
}

void generateIncrementReference(GenerationContext *context, TypedValue *managedPointer)
{
    assert(managedPointer->getTypeCode() == TypeCode::POINTER && "generateIncrementReference arg must be pointer");

    PointerType *pointerType = static_cast<PointerType *>(managedPointer->getType());
    assert(pointerType->isManaged() && "generateIncrementReference pointer arg must be managed");

    std::string twine = managedPointer->getOriginVariable();

    std::vector<llvm::Value *> indices;
    indices.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context->context), 0, false));
    indices.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context->context), 0, false));
    llvm::Value *refCountPointer = context->irBuilder->CreateGEP(pointerType->getLLVMPointedType(context), managedPointer->getValue(), indices, twine + ".refcount.ptr");

    llvm::Value *refCount = context->irBuilder->CreateLoad(getRefCountType(*context->context), refCountPointer, twine + ".refcount");
    // Increase refCount by 1
    refCount = context->irBuilder->CreateAdd(refCount, llvm::ConstantInt::get(getRefCountType(*context->context), 1, false), twine + ".refcount.inc", true, true);
    context->irBuilder->CreateStore(refCount, refCountPointer, false);
}

void generateDecrementReferenceIfPointer(GenerationContext *context, TypedValue *maybeManagedPointer, bool checkFree)
{
    // Union type could include pointer
    if (maybeManagedPointer->getTypeCode() == TypeCode::UNION)
    {
        UnionType *unionType = static_cast<UnionType *>(maybeManagedPointer->getType());

        // Check if union contains pointer types
        for (Type *containedUnionType : unionType->getTypes())
        {
            // If the union can be a pointer
            if (containedUnionType->getTypeCode() == TypeCode::POINTER && static_cast<PointerType *>(containedUnionType)->isManaged())
            {
                auto okBlock = generateUnionIsBranches(context, maybeManagedPointer, containedUnionType);
                llvm::BasicBlock *continueBlock = llvm::BasicBlock::Create(*context->context, "union.ref.dec.continue", context->irBuilder->GetInsertBlock()->getParent());
                context->irBuilder->CreateBr(continueBlock);

                context->irBuilder->SetInsertPoint(okBlock);
                // At this point, we are sure maybeManagedPointer is containedUnionType
                auto llvmUnionData = generateUnionGetData(context, maybeManagedPointer, containedUnionType);
                generateDecrementReference(context, llvmUnionData, checkFree);
                context->irBuilder->CreateBr(continueBlock);

                context->irBuilder->SetInsertPoint(continueBlock);
            }
        }
    }
    else if (maybeManagedPointer->getTypeCode() == TypeCode::POINTER)
    {
        // Increase reference count for loaded pointer
        PointerType *loadedPointerType = static_cast<PointerType *>(maybeManagedPointer->getType());
        if (loadedPointerType->isManaged())
        {
            generateDecrementReference(context, maybeManagedPointer, checkFree);
        }
    }
}

void generateIncrementReferenceIfPointer(GenerationContext *context, TypedValue *maybeManagedPointer)
{
    // Union type could include pointer
    if (maybeManagedPointer->getTypeCode() == TypeCode::UNION)
    {
        UnionType *unionType = static_cast<UnionType *>(maybeManagedPointer->getType());

        // Check if union contains pointer types
        for (Type *containedUnionType : unionType->getTypes())
        {
            // If the union can be a pointer
            if (containedUnionType->getTypeCode() == TypeCode::POINTER && static_cast<PointerType *>(containedUnionType)->isManaged())
            {
                auto okBlock = generateUnionIsBranches(context, maybeManagedPointer, containedUnionType);
                llvm::BasicBlock *continueBlock = llvm::BasicBlock::Create(*context->context, "union.ref.inc.continue", context->irBuilder->GetInsertBlock()->getParent());
                context->irBuilder->CreateBr(continueBlock);

                context->irBuilder->SetInsertPoint(okBlock);
                // At this point, we are sure maybeManagedPointer is containedUnionType
                auto llvmUnionData = generateUnionGetData(context, maybeManagedPointer, containedUnionType);
                generateIncrementReference(context, llvmUnionData);
                context->irBuilder->CreateBr(continueBlock);

                context->irBuilder->SetInsertPoint(continueBlock);
            }
        }
    }
    else if (maybeManagedPointer->getTypeCode() == TypeCode::POINTER)
    {
        // Increase reference count for loaded pointer
        PointerType *loadedPointerType = static_cast<PointerType *>(maybeManagedPointer->getType());
        if (loadedPointerType->isManaged())
        {
            generateIncrementReference(context, maybeManagedPointer);
        }
    }
}

llvm::Value *generateUnionGetTypeId(GenerationContext *context, TypedValue *unionToExtract)
{
    assert(unionToExtract->getTypeCode() == TypeCode::UNION);

    std::vector<unsigned int> indices;
    indices.push_back(0);
    return context->irBuilder->CreateExtractValue(unionToExtract->getValue(), indices, "union.typeid");
}

llvm::BasicBlock *generateUnionIsBranches(GenerationContext *context, TypedValue *unionToCompare, Type *compareType)
{
    assert(unionToCompare->getTypeCode() == TypeCode::UNION);

    llvm::Function *currentFunction = context->irBuilder->GetInsertBlock()->getParent();

    llvm::Value *llvmTypeIdValue = generateUnionGetTypeId(context, unionToCompare);

    std::vector<uint64_t> allowedTypeIds;
    if (compareType->getTypeCode() == TypeCode::UNION)
    {
        UnionType *targetUnionType = static_cast<UnionType *>(compareType);
        for (auto type : targetUnionType->getTypes())
        {
            allowedTypeIds.push_back(context->getTypeId(type));
        }
    }
    else
    {
        allowedTypeIds.push_back(context->getTypeId(compareType));
    }
    assert(allowedTypeIds.size() > 0);

    llvm::BasicBlock *okBlock = llvm::BasicBlock::Create(*context->context, "union.match", currentFunction);

    llvm::BasicBlock *nextBlock;
    for (int i = 0; i < allowedTypeIds.size(); i++)
    {
        uint64_t typeId = allowedTypeIds[i];

        auto llvmMatchesValue = context->irBuilder->CreateICmpEQ(llvm::ConstantInt::get(getUnionIdType(*context->context), typeId, false), llvmTypeIdValue, "union.cmp." + std::to_string(i));

        nextBlock = llvm::BasicBlock::Create(*context->context, "union.check", currentFunction);
        context->irBuilder->CreateCondBr(llvmMatchesValue, okBlock, nextBlock);
        context->irBuilder->SetInsertPoint(nextBlock);
    }

    return okBlock;
}

TypedValue *generateUnionGetData(GenerationContext *context, TypedValue *unionToConvert, Type *asType)
{
    assert(unionToConvert->getTypeCode() == TypeCode::UNION);

    std::vector<unsigned int> indices;
    indices.push_back(1);
    llvm::Value *llvmUnionData = context->irBuilder->CreateExtractValue(unionToConvert->getValue(), indices, "union.data.asint");
    auto llvmTargetType = asType->getLLVMType(context);

    int asTypeBitSize = context->module->getDataLayout().getTypeStoreSizeInBits(llvmTargetType);
    auto llvmCastedAsIntValue = context->irBuilder->CreateTruncOrBitCast(llvmUnionData, llvm::Type::getIntNTy(*context->context, asTypeBitSize), "union.data.trunc");
    auto llvmCastedValue = context->irBuilder->CreateBitOrPointerCast(llvmCastedAsIntValue, llvmTargetType, "union.data");
    return new TypedValue(llvmCastedValue, asType);
}

TypedValue *generateUnionConversion(GenerationContext *context, TypedValue *unionToConvert, Type *targetType)
{
    llvm::BasicBlock *okBlock = generateUnionIsBranches(context, unionToConvert, targetType);

    // After the last block is reached, the value does not match the union, panic
    generatePanic(context, "Cannot cast " + unionToConvert->getType()->toString() + " to " + targetType->toString());

    context->irBuilder->SetInsertPoint(okBlock);

    if (targetType->getTypeCode() == TypeCode::UNION)
    {
        auto llvmBitCastedValue = context->irBuilder->CreateBitCast(unionToConvert->getValue(), targetType->getLLVMType(context));
        return new TypedValue(llvmBitCastedValue, targetType);
    }
    else
    {
        return generateUnionGetData(context, unionToConvert, targetType);
    }
}

TypedValue *generateUnionIs(GenerationContext *context, TypedValue *unionToCompare, Type *compareType)
{
    llvm::BasicBlock *okBlock = generateUnionIsBranches(context, unionToCompare, compareType);
    llvm::BasicBlock *notOkBlock = context->irBuilder->GetInsertBlock();

    llvm::BasicBlock *continueBlock = llvm::BasicBlock::Create(*context->context, "union.is.continue", notOkBlock->getParent());
    context->irBuilder->CreateBr(continueBlock);

    context->irBuilder->SetInsertPoint(okBlock);
    context->irBuilder->CreateBr(continueBlock);

    context->irBuilder->SetInsertPoint(continueBlock);

    // After the last block is reached, the value does not match the union, return false
    auto llvmBoolType = BOOL_TYPE.getLLVMType(context);
    auto phi = context->irBuilder->CreatePHI(llvmBoolType, 2, "union.is");
    phi->addIncoming(llvm::ConstantInt::get(llvmBoolType, 0, false), notOkBlock);
    phi->addIncoming(llvm::ConstantInt::get(llvmBoolType, 1, false), okBlock);
    return new TypedValue(phi, &BOOL_TYPE);
}

// Try to cast a value to a specific type
TypedValue *generateTypeConversion(GenerationContext *context, TypedValue *valueToConvert, Type *targetType, bool allowLosePrecision)
{
    if (*valueToConvert->getType() == *targetType)
    {
        return valueToConvert;
    }

    if (valueToConvert->getTypeCode() == TypeCode::UNION)
    {
        UnionType *unionType = static_cast<UnionType *>(valueToConvert->getType());
        if (!unionType->containsType(targetType))
        {
            std::cout << "ERROR: Cannot convert union " << valueToConvert->getType()->toString() << " to " << targetType->toString() << " (not included in union)\n";
            return NULL;
        }

        return generateUnionConversion(context, valueToConvert, targetType);
    }
    else if (targetType->getTypeCode() == TypeCode::UNION)
    {
        UnionType *unionType = static_cast<UnionType *>(targetType);
        if (!unionType->containsType(valueToConvert->getType()))
        {
            std::cout << "ERROR: Cannot convert " << valueToConvert->getType()->toString() << " to union " << targetType->toString() << " (not included in union)\n";
            return NULL;
        }
        return new TypedValue(unionType->createValue(context, valueToConvert), targetType);
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
                    currentValue = context->irBuilder->CreateZExt(currentValue, targetIntType->getLLVMType(context), "convzextint");
                }
                else
                {
                    currentValue = context->irBuilder->CreateSExt(currentValue, targetIntType->getLLVMType(context), "convzextint");
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
                    currentValue = context->irBuilder->CreateZExt(currentValue, targetIntType->getLLVMType(context), "convzextint");
                }
                else
                {
                    currentValue = context->irBuilder->CreateSExt(currentValue, targetIntType->getLLVMType(context), "convzextint");
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

            currentValue = context->irBuilder->CreateTrunc(currentValue, targetIntType->getLLVMType(context), "convtruncint");
        }
    }
    else if (targetType->getTypeCode() == TypeCode::FLOAT && currentType->getTypeCode() == TypeCode::FLOAT)
    {
        FloatType *currentFloatType = static_cast<FloatType *>(currentType);
        FloatType *targetFloatType = static_cast<FloatType *>(targetType);

        if (targetFloatType->getBitSize() > currentFloatType->getBitSize())
        {
            currentValue = context->irBuilder->CreateFPExt(currentValue, targetFloatType->getLLVMType(context), "convfpext");
        }
        else if (targetFloatType->getBitSize() < currentFloatType->getBitSize())
        {
            if (!allowLosePrecision)
            {
                std::cout << "ERROR: Cannot implicitly convert floats of size " << currentFloatType->getBitSize() << " and " << targetFloatType->getBitSize() << "\n";
                return NULL;
            }

            currentValue = context->irBuilder->CreateFPTrunc(currentValue, targetFloatType->getLLVMType(context), "convtruncfp");
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
            currentValue = context->irBuilder->CreateSIToFP(currentValue, targetFloatType->getLLVMType(context), "convsitofp");
        }
        else
        {
            currentValue = context->irBuilder->CreateUIToFP(currentValue, targetFloatType->getLLVMType(context), "convuitofp");
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
            currentValue = context->irBuilder->CreateFPToSI(currentValue, targetIntType->getLLVMType(context), "convfptosi");
        }
        else
        {
            currentValue = context->irBuilder->CreateFPToUI(currentValue, targetIntType->getLLVMType(context), "convfptoui");
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

void generatePanic(GenerationContext *context, std::string reason)
{
    llvm::Function *panicFunction = context->module->getFunction(panicName);
    if (panicFunction == NULL)
    {
        std::vector<llvm::Type *> panicParams;
        panicParams.push_back(llvm::Type::getInt8PtrTy(*context->context));
        llvm::FunctionType *functionType = llvm::FunctionType::get(llvm::Type::getVoidTy(*context->context), panicParams, false);
        panicFunction = llvm::Function::Create(functionType, llvm::Function::ExternalLinkage, panicName, *context->module);
    }

    std::vector<llvm::Value *> parameters;
    parameters.push_back(context->irBuilder->CreateGlobalStringPtr(reason));
    context->irBuilder->CreateCall(panicFunction, parameters);
    context->irBuilder->CreateUnreachable();
}

llvm::Value *generateMalloc(GenerationContext *context, llvm::Type *type, std::string twine)
{
    llvm::Function *mallocFunction = context->module->getFunction(mallocName);
    if (mallocFunction == NULL)
    {
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