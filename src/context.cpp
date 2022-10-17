#include "context.hpp"
#include "typedValue.hpp"

GenerationContext::GenerationContext() : context(std::make_unique<llvm::LLVMContext>()),
                                         irBuilder(std::make_unique<llvm::IRBuilder<>>(*context)),
                                         module(std::make_unique<llvm::Module>("default-choco-module", *context)),
                                         passManager(std::make_unique<llvm::legacy::FunctionPassManager>(module.get())),
                                         globalModule(new ModuleType("Global"))
{
#ifndef DEBUG
    passManager->add(llvm::createMemCpyOptPass());
    passManager->add(llvm::createPromoteMemoryToRegisterPass());
    passManager->add(llvm::createInstructionCombiningPass());
    passManager->add(llvm::createReassociatePass());
    passManager->add(llvm::createGVNPass());
    passManager->add(llvm::createCFGSimplificationPass());
    passManager->add(llvm::createDeadCodeEliminationPass());
#endif
    passManager->doInitialization();
}

FunctionScope::FunctionScope(const FunctionScope &copyFrom)
{
    for (auto const &p : copyFrom.namedValues)
    {
        this->namedValues[p.first] = p.second;
    }
}

bool FunctionScope::addValue(const std::string &name, TypedValue *value)
{
    if (this->hasValue(name))
    {
        return false;
    }
    else
    {
        this->namedValues[name] = value;
        return true;
    }
}

bool FunctionScope::hasValue(const std::string &name)
{
    return this->namedValues.count(name) > 0;
}

TypedValue *FunctionScope::getValue(const std::string &name)
{
    if (this->namedValues.count(name) > 0)
    {
        return this->namedValues[name];
    }
    else
    {
        return NULL;
    }
}