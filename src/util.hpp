#pragma once

#include <iostream>
#include <list>
#include <map>
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"
#include "llvm/Transforms/Utils.h"

class TypedValue;
class GenerationContext;
class Type;

TypedValue *generateDereferenceToPointer(GenerationContext *context, TypedValue *currentValue);
TypedValue *generateDereferenceToValue(GenerationContext *context, TypedValue *currentValue);
bool generateTypeJugging(GenerationContext *context, TypedValue **leftInOut, TypedValue **rightInOut);
TypedValue *generateTypeConversion(GenerationContext *context, TypedValue *valueToConvert, Type *targetType, bool allowLosePrecision);
bool generateAssignment(GenerationContext *context, TypedValue *valuePointer, TypedValue *newValue, bool isVolatile);
TypedValue *generateReferenceAwareLoad(GenerationContext *context, TypedValue *valuePointer);
TypedValue *generateLoad(GenerationContext *context, TypedValue *valuePointer);
void generateIncrementReference(GenerationContext *context, TypedValue *managedPointer);
void generateDecrementReference(GenerationContext *context, TypedValue *managedPointer, bool checkFree);
void generateIncrementReferenceIfPointer(GenerationContext *context, TypedValue *managedPointer);
void generateDecrementReferenceIfPointer(GenerationContext *context, TypedValue *maybeManagedPointer, bool checkFree);
void generatePanic(GenerationContext *context, std::string reason);
TypedValue *generateUnionIs(GenerationContext *context, TypedValue *unionToCompare, Type *compareType);
TypedValue *generateUnionConversion(GenerationContext *context, TypedValue *unionToConvert, Type *targetType);
TypedValue *generateUnionGetData(GenerationContext *context, TypedValue *unionToConvert, Type *asType);
llvm::BasicBlock *generateUnionIsBranches(GenerationContext *context, TypedValue *unionToCompare, Type *compareType);

llvm::Type *getRefCountType(llvm::LLVMContext &context);
llvm::Type *getUnionIdType(llvm::LLVMContext &context);

llvm::Value *generateSizeOf(GenerationContext *context, llvm::Type *type, std::string twine);
llvm::Value *generateMalloc(GenerationContext *context, llvm::Type *type, std::string twine);
llvm::Value *generateFree(GenerationContext *context, llvm::Value *toFree, std::string twine);
llvm::AllocaInst *generateAllocaInCurrentFunction(GenerationContext *context, llvm::Type *type, llvm::StringRef twine);