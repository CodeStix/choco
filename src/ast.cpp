#include "ast.hpp"

ASTFunction *parseFunction(std::list<const Token *> &tokens)
{
    const Token *tok = tokens.front();
    if (tok == NULL)
    {
        std::cout << "ERROR: Unexpected end of file\n";
        return NULL;
    }
    if (tok->type != TokenType::FUNC_KEYWORD)
    {
        std::cout << "ERROR: Function must start with 'func'\n";
        return NULL;
    }
    tokens.pop_front();

    tok = tokens.front();
    if (tok == NULL)
    {
        std::cout << "ERROR: Unexpected end of file\n";
        return NULL;
    }
    const Token *nameToken = tok;
    if (nameToken->type != TokenType::SYMBOL)
    {
        std::cout << "ERROR: Function name must be symbol, not " << getTokenTypeName(nameToken->type) << "\n";
    }
    else
    {
        tokens.pop_front();
    }

    tok = tokens.front();
    if (tok == NULL)
    {
        std::cout << "ERROR: Unexpected end of file\n";
        return NULL;
    }
    if (tok->type != TokenType::BRACKET_OPEN)
    {
        std::cout << "ERROR: Function must have parameter list\n";
    }
    else
    {
        tokens.pop_front();
    }

    std::vector<const Token *> *argumentNames = new std::vector<const Token *>();
    while (true)
    {
        tok = tokens.front();
        if (tok == NULL)
        {
            std::cout << "ERROR: Unexpected end of file\n";
            return NULL;
        }
        if (tok->type == TokenType::BRACKET_CLOSE)
        {
            tokens.pop_front();
            break;
        }

        if (tok->type != TokenType::SYMBOL)
        {
            std::cout << "ERROR: Invocation parameters must be a symbol\n";
        }
        else
        {
            argumentNames->push_back(tok);
            tokens.pop_front();
        }

        tok = tokens.front();
        if (tok == NULL)
        {
            std::cout << "ERROR: Unexpected end of file\n";
            return NULL;
        }
        if (tok->type != TokenType::COMMA)
        {
            if (tok->type != TokenType::BRACKET_CLOSE)
            {
                std::cout << "ERROR: Invocation parameters should be split using commas (function definition)\n";
            }
        }
        else
        {
            tokens.pop_front();
        }
    }

    tok = tokens.front();
    if (tok == NULL)
    {
        std::cout << "ERROR: Unexpected end of file\n";
        return NULL;
    }

    if (tok->type == TokenType::EXTERN_KEYWORD)
    {
        tokens.pop_front();

        return new ASTFunction(nameToken, argumentNames, NULL);
    }
    else if (tok->type == TokenType::CURLY_BRACKET_OPEN)
    {
        tokens.pop_front();

        std::list<ASTNode *> *statements = new std::list<ASTNode *>();

        while (1)
        {
            tok = tokens.front();
            if (tok == NULL)
            {
                std::cout << "ERROR: Unexpected end of file\n";
                return NULL;
            }
            if (tok->type == TokenType::CURLY_BRACKET_CLOSE)
            {
                tokens.pop_front();
                break;
            }

            ASTNode *statement = NULL;
            switch (tok->type)
            {
            case TokenType::FUNC_KEYWORD:
                statement = parseFunction(tokens);
                break;
            case TokenType::CONST_KEYWORD:
            case TokenType::LET_KEYWORD:
                statement = parseDeclaration(tokens);
                break;
            case TokenType::SYMBOL:
                statement = parseSymbolOperation(tokens);
                break;
            }

            if (statement == NULL)
            {
                std::cout << "ERROR: Invalid function statement\n";
            }
            else
            {
                statements->push_back(statement);
            }
        }

        return new ASTFunction(nameToken, argumentNames, statements);
    }
    else
    {
        std::cout << "ERROR: Function must have body are be marked as extern\n";
        return NULL;
    }
}

// Parses invocation, assingment or a read of a symbol
ASTNode *parseSymbolOperation(std::list<const Token *> &tokens)
{
    const Token *tok = tokens.front();
    if (tok == NULL)
    {
        std::cout << "ERROR: Unexpected end of file\n";
        return NULL;
    }
    if (tok->type != TokenType::SYMBOL)
    {
        return NULL;
    }

    const Token *nameToken = tok;

    tokens.pop_front();
    tok = tokens.front();
    switch (tok->type)
    {
    case TokenType::BRACKET_OPEN:
    {
        // Parse invocation
        tokens.pop_front();

        std::list<ASTNode *> *parameterValues = new std::list<ASTNode *>();
        while (true)
        {
            tok = tokens.front();
            if (tok->type == TokenType::BRACKET_CLOSE)
            {
                tokens.pop_front();
                break;
            }

            ASTNode *value = parseValueOrOperator(tokens);
            parameterValues->push_back(value);

            tok = tokens.front();
            if (tok->type != TokenType::COMMA)
            {
                if (tok->type != TokenType::BRACKET_CLOSE)
                {
                    std::cout << "ERROR: Invocation parameters should be split using commas\n";
                }
            }
            else
            {
                tokens.pop_front();
            }
        }
        return new ASTInvocation(nameToken, parameterValues);
    }

    case TokenType::ASSIGNMENT_OPERATOR:
    {
        // Parse assigment
        tokens.pop_front();

        ASTNode *value = parseValueOrOperator(tokens);
        if (!value)
        {
            std::cout << "ERROR: Assigment value is invalid\n";
            return NULL;
        }

        return new ASTAssignment(nameToken, value);
    }

    default:
        // Parse read
        return new ASTReadVariable(nameToken);
    }
}

ASTNode *parseValue(std::list<const Token *> &tokens)
{
    const Token *tok = tokens.front();
    if (tok == NULL)
    {
        std::cout << "ERROR: Unexpected end of file\n";
        return NULL;
    }

    ASTNode *value;
    switch (tok->type)
    {
    case TokenType::LITERAL_STRING:
        value = new ASTLiteralString(tok);
        tokens.pop_front();
        break;

    case TokenType::LITERAL_NUMBER:
        value = new ASTLiteralNumber(tok);
        tokens.pop_front();
        break;

    case TokenType::BRACKET_OPEN:
    {
        tokens.pop_front();

        ASTNode *innerValue = parseValueOrOperator(tokens);
        if (innerValue == NULL)
        {
            std::cout << "ERROR: Invalid brackets content \n";
            return NULL;
        }

        tok = tokens.front();
        if (tok->type != TokenType::BRACKET_CLOSE)
        {
            std::cout << "ERROR: Brackets must be closed\n";
        }
        else
        {
            tokens.pop_front();
        }

        value = new ASTBrackets(innerValue);
        break;
    }

    case TokenType::SYMBOL:
    {
        value = parseSymbolOperation(tokens);
        if (value == NULL)
        {
            std::cout << "ERROR: Unexpected symbol \n";
            return NULL;
        }
        break;
    }

    default:
        std::cout << "ERROR: Not a value: " << getTokenTypeName(tok->type) << "\n";
        return NULL;
    }

    return value;
}

ASTNode *parseValueOrOperator(std::list<const Token *> &tokens)
{
    ASTNode *top = parseValue(tokens);
    if (top == NULL)
    {
        return NULL;
    }

    while (true)
    {
        const Token *tok = tokens.front();
        if (tok == NULL)
        {
            return top;
        }

        int operatorImportance = getTokenOperatorImportance(tok->type);
        if (operatorImportance < 0)
        {
            // Isn't an operator, return
            return top;
        }

        tokens.pop_front();
        ASTNode *right = parseValue(tokens);
        if (right == NULL)
        {
            std::cout << "ERROR: Right side of operator must be specified\n";
            return NULL;
        }

        // Operators with higher importance should be calculated first, meaning it should be lower in the AST
        if (top->type == ASTNodeType::OPERATOR)
        {
            ASTOperator *topOperator = (ASTOperator *)top;
            int topImportance = getTokenOperatorImportance(topOperator->operatorToken->type);
            if (operatorImportance > topImportance)
            {
                // Current operator should move down in the tree
                topOperator->right = new ASTOperator(tok, topOperator->right, right);
            }
            else
            {
                // Current operator should move to top
                top = new ASTOperator(tok, topOperator, right);
            }
        }
        else
        {
            top = new ASTOperator(tok, top, right);
        }
    }
}

ASTDeclaration *parseDeclaration(std::list<const Token *> &tokens)
{
    const Token *tok = tokens.front();
    if (tok == NULL)
    {
        std::cout << "ERROR: Unexpected end of file\n";
        return NULL;
    }
    if (tok->type != TokenType::CONST_KEYWORD && tok->type != TokenType::LET_KEYWORD)
    {
        std::cout << "ERROR: Declaration must start with 'const' or 'let'\n";
        return NULL;
    }
    tokens.pop_front();

    tok = tokens.front();
    if (tok == NULL)
    {
        std::cout << "ERROR: Unexpected end of file\n";
        return NULL;
    }
    const Token *nameToken = tok;
    if (nameToken->type != TokenType::SYMBOL)
    {
        std::cout << "ERROR: Constant name must be symbol, not " << getTokenTypeName(nameToken->type) << "\n";
    }
    else
    {
        tokens.pop_front();
    }

    tok = tokens.front();
    if (tok == NULL)
    {
        std::cout << "ERROR: Unexpected end of file\n";
        return NULL;
    }
    if (tok->type == TokenType::ASSIGNMENT_OPERATOR)
    {
        // Parse assignment
        tokens.pop_front();
        ASTNode *value = parseValueOrOperator(tokens);
        if (value == NULL)
        {
            std::cout << "ERROR: Invalid assignment value\n";
            return NULL;
        }

        return new ASTDeclaration(nameToken, value);
    }
    else
    {
        return new ASTDeclaration(nameToken, NULL);
    }
}

ASTFile *parseFile(std::list<const Token *> &tokens)
{
    std::list<ASTNode *> *rootNodes = new std::list<ASTNode *>();

    while (!tokens.empty())
    {
        const Token *tok = tokens.front();

        ASTNode *statement = NULL;
        switch (tok->type)
        {
        case TokenType::FUNC_KEYWORD:
            statement = parseFunction(tokens);
            break;
        case TokenType::CONST_KEYWORD:
        case TokenType::LET_KEYWORD:
            statement = parseDeclaration(tokens);
            break;
        case TokenType::SYMBOL:
            statement = parseSymbolOperation(tokens);
            break;
        }

        if (statement == NULL)
        {
            tokens.pop_front();
            std::cout << "ERROR: Invalid statement, unexpected " << getTokenTypeName(tok->type) << " at " << tok->position << "\n";
        }
        else if (statement->type == ASTNodeType::READ_VARIABLE)
        {
            std::cout << "ERROR: Variable read is not a valid statement\n";
        }
        else
        {
            rootNodes->push_back(statement);
        }
    }

    for (ASTNode *node : *rootNodes)
    {
        std::cout << node->toString() << "\n";
    }

    return new ASTFile(rootNodes);
}

llvm::Value *ASTReadVariable::generateLLVM(GenerationContext *context)
{
    auto value = context->staticNamedValues[this->nameToken->value];
    if (!value)
    {
        std::cout << "BUILD ERROR: Could not find variable '" << this->nameToken->value << "'\n";
    }
    return value;
}

llvm::Value *ASTLiteralNumber::generateLLVM(GenerationContext *context)
{
    double parsed = strtod(this->valueToken->value.c_str(), NULL);
    return llvm::ConstantFP::get(context->llvmContext, llvm::APFloat(parsed));
}

llvm::Value *ASTOperator::generateLLVM(GenerationContext *context)
{
    llvm::Value *left = this->left->generateLLVM(context);
    llvm::Value *right = this->right->generateLLVM(context);
    if (!left || !right)
    {
        return NULL;
    }

    switch (this->operatorToken->value[0])
    {
    case '+':
        return context->llvmIRBuilder.CreateFAdd(left, right, "operator-add");

    case '-':
        return context->llvmIRBuilder.CreateFSub(left, right, "operator-sub");

    case '*':
        return context->llvmIRBuilder.CreateFMul(left, right, "operator-mul");

    case '/':
        return context->llvmIRBuilder.CreateFDiv(left, right, "operator-div");

    case '%':
        return context->llvmIRBuilder.CreateSRem(left, right, "operator-mod");

    case '<':
    {
        auto tempValue = context->llvmIRBuilder.CreateFCmpULT(left, right, "operator-lt");
        // Convert tempValue (which is an unsigned integer to a double)
        return context->llvmIRBuilder.CreateUIToFP(tempValue, llvm::Type::getDoubleTy(context->llvmContext), "operator-lt-fp");
    }

    case '>':
    {
        auto tempValue = context->llvmIRBuilder.CreateFCmpUGT(left, right, "operator-gt");
        // Convert tempValue (which is an unsigned integer to a double)
        return context->llvmIRBuilder.CreateUIToFP(tempValue, llvm::Type::getDoubleTy(context->llvmContext), "operator-gt-fp");
    }

    default:
        std::cout
            << "BUILD ERROR: Invalid operator '" << this->operatorToken->value << "'\n";
        return NULL;
    }
}

llvm::Value *ASTLiteralString::generateLLVM(GenerationContext *context)
{
    return NULL;
}

llvm::Value *ASTDeclaration::generateLLVM(GenerationContext *context)
{
    if (context->staticNamedValues[this->nameToken->value])
    {
        // This name is already declared
        std::cout << "BUILD ERROR: Variable '" << this->nameToken->value << "' has already been declared\n";
        return NULL;
    }
    else
    {
        llvm::Value *value;
        if (this->value != NULL)
        {
            // There is an assignment after the declaration
            value = this->value->generateLLVM(context);
        }
        else
        {
            // Uninitialized value
            value = llvm::ConstantTokenNone::get(context->llvmContext);
        }

        context->staticNamedValues[this->nameToken->value] = value;
        return value;
    }
}

llvm::Value *ASTAssignment::generateLLVM(GenerationContext *context)
{
    if (!context->staticNamedValues[this->nameToken->value])
    {
        // This name is already declared
        std::cout << "BUILD ERROR: Variable '" << this->nameToken->value << "' not found\n";
        return NULL;
    }
    else
    {
        llvm::Value *value = this->value->generateLLVM(context);
        context->staticNamedValues[this->nameToken->value] = value;
        return value;
    }
}

llvm::Value *ASTFunction::generateLLVM(GenerationContext *context)
{
    llvm::Function *function = context->llvmCurrentModule.getFunction(this->nameToken->value);

    if (function == NULL)
    {
        // Define it
        std::vector<llvm::Type *> argumentTypes(this->arguments->size(), llvm::Type::getDoubleTy(context->llvmContext));
        llvm::FunctionType *functionType = llvm::FunctionType::get(llvm::Type::getDoubleTy(context->llvmContext), argumentTypes, false);

        function = llvm::Function::Create(functionType, llvm::Function::ExternalLinkage, this->nameToken->value, context->llvmCurrentModule);
        if (function == NULL)
        {
            std::cout << "BUILD ERROR: Function::Create returned null\n";
            return NULL;
        }

        int i = 0;
        for (auto &arg : function->args())
        {
            arg.setName((*this->arguments)[i++]->value);
        }
    }

    if (this->statements != NULL)
    {
        if (!function->empty())
        {
            std::cout << "BUILD ERROR: Cannot implement the '" << this->nameToken->value << "' function a second time\n";
            return NULL;
        }

        llvm::BasicBlock *block = llvm::BasicBlock::Create(context->llvmContext, "function-block", function);
        context->llvmIRBuilder.SetInsertPoint(block);
        context->staticNamedValues.clear();

        for (auto &arg : function->args())
        {
            context->staticNamedValues[arg.getName()] = &arg;
        }

        llvm::Value *returnValue = NULL;
        for (ASTNode *statement : *this->statements)
        {
            returnValue = statement->generateLLVM(context);
        }

        if (returnValue != NULL)
        {
            context->llvmIRBuilder.CreateRet(returnValue);
        }
        else
        {
            context->llvmIRBuilder.CreateRetVoid();
        }

        llvm::verifyFunction(*function);
    }

    return function;
}

llvm::Value *ASTInvocation::generateLLVM(GenerationContext *context)
{
    llvm::Function *functionToCall = context->llvmCurrentModule.getFunction(this->functionNameToken->value);
    if (functionToCall == NULL)
    {
        std::cout << "BUILD ERROR: Function '" << this->functionNameToken->value << "' not found\n";
        return NULL;
    }

    if (functionToCall->arg_size() != this->parameterValues->size())
    {
        std::cout << "BUILD ERROR: Invalid amount of parameters for function '" << this->functionNameToken->value << "', expected " << functionToCall->arg_size() << ", got " << this->parameterValues->size() << "\n";
        return NULL;
    }

    std::vector<llvm::Value *> functionArgs;
    for (ASTNode *node : *this->parameterValues)
    {
        llvm::Value *value = node->generateLLVM(context);
        if (!value)
        {
            return NULL;
        }
        functionArgs.push_back(value);
    }

    return context->llvmIRBuilder.CreateCall(functionToCall, functionArgs, "invocation");
}

llvm::Value *ASTBrackets::generateLLVM(GenerationContext *context)
{
    return this->inner->generateLLVM(context);
}

llvm::Value *ASTFile::generateLLVM(GenerationContext *context)
{
    for (ASTNode *statement : *this->statements)
    {
        statement->generateLLVM(context);
    }
    return NULL;
}
