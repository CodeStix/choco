#include <iostream>
#include <string>
#include <fstream>
#include <list>
#include "token.hpp"
#include "ast.hpp"
// #include "jit.hpp"

int main()
{
    std::string fileContent;
    std::getline(std::ifstream("test.ch"), fileContent, '\0');

    std::cout << "Tokenizing...\n";

    std::list<const Token *> tokens;
    parseString(fileContent, tokens);

    std::cout << "Tokenizing done\n";
    for (const auto &token : tokens)
    {
        std::cout << getTokenTypeName(token->type) << " token at " << token->position << ", value = " << token->value << "\n";
    }

    std::cout << "Parsing...\n";
    ASTFile *file = parseFile(tokens);
    std::cout << "Parsing done\n";
    std::cout << file->toString() << "\n";

    std::cout << "Generating code...\n";
    auto context = new GenerationContext();
    auto scope = new Scope();
    file->generateLLVM(context, scope);
    std::cout << "Generation done\n";
    context->module->print(llvm::errs(), NULL);

    // std::cout << "Executing...\n";

    // auto jitError = llvm::orc::KaleidoscopeJIT::Create();
    // if (!jitError)
    // {
    //     std::cout << "Coult not create JIT\n";
    //     return -1;
    // }
    // auto jit = jitError->get();
    // context->module.setDataLayout(jit->getDataLayout());

    // jit->addModule(llvm::orc::ThreadSafeModule(std::make_unique(context->module), std::make_unique(context->context)));

    std::cout << "Everything is done\n";
    return 0;
}
