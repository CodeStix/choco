#include <iostream>
#include <string>
#include <fstream>
#include <list>
#include "token.hpp"
#include "ast.hpp"

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
    file->generateLLVM(context);
    std::cout << "Generation done\n";
    context->llvmCurrentModule.print(llvm::errs(), NULL);

    std::cout << "Everything is done\n";
    return 0;
}
