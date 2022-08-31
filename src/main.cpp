#include <iostream>
#include <string>
#include <fstream>
#include <list>
#include "token.hpp"
#include "ast.hpp"
#include "jit.hpp"
#include "llvm/Support/TargetSelect.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Linker/Linker.h"

llvm::ExitOnError exitOnError;

extern "C" double printDouble(double X)
{
    fprintf(stderr, "%f\n", X);
    return 0;
}

int main()
{
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

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

    return 0;

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

    std::cout << "Creating executable...\n";

    auto targetCpu = "x86-64";
    auto targetFeatures = ""; // "+avx,+avx2,+aes,+sse,+sse2,+sse3";
    auto targetTriple = llvm::sys::getDefaultTargetTriple();
    std::string targetTripleError;
    auto target = llvm::TargetRegistry::lookupTarget(targetTriple, targetTripleError);
    if (!target)
    {
        std::cout << "Could not lookup target: " << targetTripleError;
        return 1;
    }

    llvm::TargetOptions targetOptions;
    auto targetMachine = target->createTargetMachine(targetTriple, targetCpu, targetFeatures, targetOptions, llvm::Reloc::DynamicNoPIC);

    context->module->setDataLayout(targetMachine->createDataLayout());
    context->module->setTargetTriple(targetTriple);

    llvm::legacy::PassManager passManager;
    std::error_code outputFileErrorCode;
    std::string outputFilePath = "output.o";
    llvm::raw_fd_ostream outputFile(outputFilePath, outputFileErrorCode, llvm::sys::fs::OF_None);
    if (outputFileErrorCode)
    {
        std::cout << "Could not open output file: " << outputFileErrorCode;
        return 1;
    }
    targetMachine->addPassesToEmitFile(passManager, outputFile, NULL, llvm::CodeGenFileType::CGFT_ObjectFile);

    passManager.run(*context->module);
    outputFile.close();

    std::cout << "Wrote to " << outputFilePath << "\n";
    std::cout << "Everything is done\n";
    return 0;
}
