#include <iostream>
#include <string>
#include <fstream>
#include <list>
#include "token.hpp"
#include "ast.hpp"
#include "jit.hpp"

llvm::ExitOnError exitOnError;

extern "C" double printDouble(double X)
{
    fprintf(stderr, "%f\n", X);
    return 0;
}

int main()
{
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    std::string fileContent;
    std::getline(std::ifstream("test.ch"), fileContent, '\0');

    std::cout << "Tokenizing...\n";

    std::list<const Token *> tokens;
    parseString(fileContent, tokens);

    std::cout << "Tokenizing done\n";
    // for (const auto &token : tokens)
    // {
    //     std::cout << getTokenTypeName(token->type) << " token at " << token->position << ", value = " << token->value << "\n";
    // }

    std::cout << "Parsing...\n";
    ASTFile *file = parseFile(tokens);
    std::cout << "Parsing done\n";
    std::cout << file->toString() << "\n";

    std::cout << "Generating code...\n";
    auto context = new GenerationContext();
    auto scope = new FunctionScope();
    file->generateLLVM(context, scope);
    std::cout << "Generation done\n";
    context->module->print(llvm::errs(), NULL);

    std::cout << "Executing...\n";

    auto jit = exitOnError(llvm::orc::KaleidoscopeJIT::Create());
    context->module->setDataLayout(jit->getDataLayout());

    auto resourceTracker = jit->getMainJITDylib().createResourceTracker();
    exitOnError(jit->addModule(llvm::orc::ThreadSafeModule(std::move(context->module), std::move(context->context)), resourceTracker));

    auto entryPoint = exitOnError(jit->lookup("testmain"));
    double (*entryPointFunction)() = (double (*)())(void *)entryPoint.getAddress();
    printf("entryPointFunction = %p\n", entryPointFunction);
    double result = entryPointFunction();
    printf("result = %f\n", result);

    std::cout << "Everything is done\n";
    return 0;
}
