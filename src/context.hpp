#pragma once
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
class ModuleType;
class FunctionType;
class Type;

class FunctionScope
{
public:
    FunctionScope() {}
    FunctionScope(const FunctionScope &copyFrom);

    // Returns false if the name already exists locally
    bool addValue(const std::string &name, TypedValue *value);

    bool hasValue(const std::string &name);

    TypedValue *getValue(const std::string &name);

    // private:
    std::map<std::string, TypedValue *> namedValues;
};

class GenerationContext
{
public:
    GenerationContext();

    uint64_t getTypeId(Type *type);

    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::IRBuilder<>> irBuilder;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::legacy::FunctionPassManager> passManager;
    FunctionType *currentFunction;
    llvm::BasicBlock *currentFunctionReturnBlock;
    llvm::Value *currentFunctionReturnValuePointer;
    std::map<llvm::Type *, llvm::Function *> freeFunctions;
    std::map<llvm::Type *, llvm::Function *> mallocFunctions;
    std::vector<Type *> unionTypeIds;
    ModuleType *globalModule;
};