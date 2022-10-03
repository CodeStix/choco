#define DEBUG
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

    std::cout << "[1/4] Tokenizing...\n";

    std::vector<const Token *> tokens;
    parseString(fileContent, tokens);

    TokenStream *tokenStream = new TokenStream(tokens);

#ifdef DEBUG
    std::cout << "[1/4] " << tokenStream->size() << " tokens parsed\n";
    for (const auto &token : tokens)
    {
        std::cout << getTokenTypeName(token->type) << " token at " << token->position << ", value = " << token->value << "\n";
    }
#endif

    std::cout << "[2/4] Parsing...\n";
    ASTFile *file = parseFile(tokenStream);
    if (file == NULL)
    {
        std::cout << "ERROR: Could not parse file, check console for programs\n";
        return 1;
    }

    // #ifdef DEBUG
    std::cout << file->toString() << "\n";
    // #endif

    std::cout << "[3/4] Generating code...\n";

    auto context = new GenerationContext();
    file->declareStaticNames(&context->globalModule);
    context->globalModule.addValue("Float32", new TypedValue(NULL, new FloatType(32)));
    context->globalModule.addValue("Float64", new TypedValue(NULL, new FloatType(64)));
    context->globalModule.addValue("Int64", new TypedValue(NULL, new IntegerType(64, true)));
    context->globalModule.addValue("UInt64", new TypedValue(NULL, new IntegerType(64, false)));
    context->globalModule.addValue("Int32", new TypedValue(NULL, new IntegerType(32, true)));
    context->globalModule.addValue("UInt32", new TypedValue(NULL, new IntegerType(32, false)));
    context->globalModule.addValue("Bool", new TypedValue(NULL, new IntegerType(1, false)));

    auto scope = new FunctionScope();

    // Trigger compilation by getting the main function value lazily
    TypedValue *mainFunction = context->globalModule.getValue("main", context, scope);
    if (mainFunction == NULL)
    {
        std::cout << "ERROR: Could not find main function (probably wasn't generated because of other problems)\n";
        return 1;
    }

#ifdef DEBUG
    context->module->print(llvm::errs(), NULL);
#endif

    std::cout << "[4/4] Creating executable...\n";

    auto targetCpu = "x86-64";                               // x86-64
    auto targetFeatures = "";                                // "+avx,+avx2,+aes,+sse,+sse2,+sse3";
    auto targetTriple = llvm::sys::getDefaultTargetTriple(); // "wasm32"
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

    std::cout << "[4/4] Wrote to " << outputFilePath << "\n";
    return 0;
}
