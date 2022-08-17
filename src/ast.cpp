#include "ast.hpp"

ASTBlock *parseBlock(std::list<const Token *> &tokens)
{
    const Token *tok = tokens.front();
    if (tok == NULL)
    {
        std::cout << "ERROR: Unexpected end of file\n";
        return NULL;
    }
    if (tok->type != TokenType::CURLY_BRACKET_OPEN)
    {
        std::cout << "ERROR: Expected { to open code block\n";
        return NULL;
    }
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
            return new ASTBlock(statements);
        }

        ASTNode *statement = NULL;
        switch (tok->type)
        {
        case TokenType::FUNC_KEYWORD:
            statement = parseFunction(tokens);
            break;
        case TokenType::IF_KEYWORD:
            statement = parseIfStatement(tokens);
            break;
        case TokenType::WHILE_KEYWORD:
            statement = parseWhileStatement(tokens);
            break;
        case TokenType::CONST_KEYWORD:
        case TokenType::LET_KEYWORD:
            statement = parseDeclaration(tokens);
            break;
        case TokenType::RETURN_KEYWORD:
            statement = parseReturn(tokens);
            break;
        case TokenType::SYMBOL:
            statement = parseSymbolOperation(tokens);
            break;
        }

        if (statement == NULL)
        {
            std::cout << "ERROR: Invalid statement\n";
        }
        else
        {
            statements->push_back(statement);
        }
    }
}

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
        ASTBlock *body = parseBlock(tokens);
        return new ASTFunction(nameToken, argumentNames, body);
    }
    else
    {
        std::cout << "ERROR: Function must have body are be marked as extern\n";
        return NULL;
    }
}

ASTNode *parseIfStatement(std::list<const Token *> &tokens)
{
    const Token *tok = tokens.front();
    if (tok == NULL)
    {
        std::cout << "ERROR: Unexpected end of file\n";
        return NULL;
    }
    if (tok->type != TokenType::IF_KEYWORD)
    {
        return NULL;
    }

    tokens.pop_front();

    ASTNode *condition = parseValueOrOperator(tokens);
    if (condition == NULL)
    {
        return NULL;
    }

    ASTBlock *thenBody = parseBlock(tokens);
    if (thenBody == NULL)
    {
        return NULL;
    }

    ASTNode *elseBody = NULL;
    tok = tokens.front();
    if (tok->type == TokenType::ELSE_KEYWORD)
    {
        tokens.pop_front();

        elseBody = parseBlock(tokens);
        if (elseBody == NULL)
        {
            return NULL;
        }
    }

    return new ASTIfStatement(condition, thenBody, elseBody);
}

ASTNode *parseWhileStatement(std::list<const Token *> &tokens)
{
    const Token *tok = tokens.front();
    if (tok == NULL)
    {
        std::cout << "ERROR: Unexpected end of file\n";
        return NULL;
    }
    if (tok->type != TokenType::WHILE_KEYWORD)
    {
        return NULL;
    }

    tokens.pop_front();

    ASTNode *condition = parseValueOrOperator(tokens);
    if (condition == NULL)
    {
        return NULL;
    }

    ASTBlock *loopBody = parseBlock(tokens);
    if (loopBody == NULL)
    {
        return NULL;
    }

    ASTNode *elseBody = NULL;
    tok = tokens.front();
    if (tok->type == TokenType::ELSE_KEYWORD)
    {
        tokens.pop_front();

        elseBody = parseBlock(tokens);
        if (elseBody == NULL)
        {
            return NULL;
        }
    }

    return new ASTWhileStatement(condition, loopBody, elseBody);
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

ASTReturn *parseReturn(std::list<const Token *> &tokens)
{
    const Token *tok = tokens.front();
    if (tok == NULL)
    {
        std::cout << "ERROR: Unexpected end of file\n";
        return NULL;
    }
    if (tok->type != TokenType::RETURN_KEYWORD)
    {
        std::cout << "ERROR: Expected 'return'\n";
        return NULL;
    }
    tokens.pop_front();

    ASTNode *value = parseValueOrOperator(tokens);
    if (value == NULL)
    {
        std::cout << "ERROR: Invalid return value\n";
        return NULL;
    }

    return new ASTReturn(value);
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

    return new ASTFile(rootNodes);
}

llvm::Value *ASTReadVariable::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    auto value = scope->getValue(this->nameToken->value);
    if (!value)
    {
        std::cout << "BUILD ERROR: Could not find variable '" << this->nameToken->value << "'\n";
    }
    return value;
}

llvm::Value *ASTLiteralNumber::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    double parsed = strtod(this->valueToken->value.c_str(), NULL);
    return llvm::ConstantFP::get(*context->context, llvm::APFloat(parsed));
}

llvm::Value *ASTOperator::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    llvm::Value *left = this->left->generateLLVM(context, scope);
    llvm::Value *right = this->right->generateLLVM(context, scope);
    if (!left || !right)
    {
        return NULL;
    }

    switch (this->operatorToken->value[0])
    {
    case '+':
        return context->irBuilder->CreateFAdd(left, right, "addtemp");

    case '-':
        return context->irBuilder->CreateFSub(left, right, "subtemp");

    case '*':
        return context->irBuilder->CreateFMul(left, right, "multemp");

    case '/':
        return context->irBuilder->CreateFDiv(left, right, "divtemp");

    case '%':
        return context->irBuilder->CreateSRem(left, right, "modtemp");

    case '<':
    {
        auto tempValue = context->irBuilder->CreateFCmpULT(left, right, "ltcmptemp");
        // Convert tempValue (which is an unsigned integer to a double)
        return context->irBuilder->CreateUIToFP(tempValue, llvm::Type::getDoubleTy(*context->context), "ltcmpbooltemp");
    }

    case '>':
    {
        auto tempValue = context->irBuilder->CreateFCmpUGT(left, right, "gtcmptemp");
        // Convert tempValue (which is an unsigned integer to a double)
        return context->irBuilder->CreateUIToFP(tempValue, llvm::Type::getDoubleTy(*context->context), "gtcmpbooltemp");
    }

    default:
        std::cout
            << "BUILD ERROR: Invalid operator '" << this->operatorToken->value << "'\n";
        return NULL;
    }
}

llvm::Value *ASTLiteralString::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    return NULL;
}

llvm::Value *ASTDeclaration::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    llvm::Value *value;
    if (this->value != NULL)
    {
        // There is an assignment after the declaration
        value = this->value->generateLLVM(context, scope);
    }
    else
    {
        // Uninitialized value
        value = llvm::ConstantTokenNone::get(*context->context);
    }

    if (scope->hasValue(this->nameToken->value))
    {
        std::cout << "BUILD ERROR: Cannot redeclare '" << this->nameToken->value << "', it has already been declared\n";
        return NULL;
    }
    scope->setValue(this->nameToken->value, value);
    return value;
}

llvm::Value *ASTAssignment::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    if (!scope->hasValue(this->nameToken->value))
    {
        std::cout << "BUILD ERROR: Cannot set variable '" << this->nameToken->value << "', it is not found\n";
        return NULL;
    }
    else
    {
        std::cout << "DEBUG: Assign" << this->nameToken->value << "\n";
        llvm::Value *value = this->value->generateLLVM(context, scope);
        if (!scope->hasValue(this->nameToken->value))
        {
            std::cout << "BUILD ERROR: Cannot set variable '" << this->nameToken->value << "', it is not found\n";
            return NULL;
        }
        scope->setValue(this->nameToken->value, value);
        return value;
    }
}

llvm::Value *ASTReturn::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    if (this->value != NULL)
    {
        return context->irBuilder->CreateRet(this->value->generateLLVM(context, scope));
    }
    else
    {
        return context->irBuilder->CreateRetVoid();
    }
}

llvm::Value *ASTBlock::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    for (ASTNode *statement : *this->statements)
    {
        statement->generateLLVM(context, scope);
    }

    return context->irBuilder->GetInsertBlock();
}

llvm::Value *ASTFunction::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    llvm::Function *function = context->module->getFunction(this->nameToken->value);

    if (function == NULL)
    {
        // Define it
        std::vector<llvm::Type *> argumentTypes(this->arguments->size(), llvm::Type::getDoubleTy(*context->context));
        llvm::FunctionType *functionType = llvm::FunctionType::get(llvm::Type::getDoubleTy(*context->context), argumentTypes, false);

        function = llvm::Function::Create(functionType, llvm::Function::ExternalLinkage, this->nameToken->value, *context->module);
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

    if (this->body != NULL)
    {
        if (!function->empty())
        {
            std::cout << "BUILD ERROR: Cannot implement the '" << this->nameToken->value << "' function a second time\n";
            return NULL;
        }

        FunctionScope *functionScope = new FunctionScope(*scope);

        for (auto &arg : function->args())
        {
            functionScope->setValue(arg.getName().str(), &arg);
        }

        llvm::BasicBlock *functionBlock = llvm::BasicBlock::Create(*context->context, "block", function);
        context->irBuilder->SetInsertPoint(functionBlock);
        this->body->generateLLVM(context, functionScope);

        // TODO check if return was specified, else emit context->irBuilder.CreateRetVoid();

        // Check generated IR for issues
        if (llvm::verifyFunction(*function, &llvm::errs()))
        {
            std::cout << "BUILD ERROR: LLVM reported invalid function\n";
            return NULL;
        }

        // Optimize the function code
        context->passManager->run(*function);
    }

    // if (scope->hasValue(this->nameToken->value))
    // {
    //     std::cout << "BUILD ERROR: Cannot redeclare function '" << this->nameToken->value << "'\n";
    //     return NULL;
    // }
    // scope->setValue(this->nameToken->value, function);

    return function;
}

llvm::Value *ASTWhileStatement::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    llvm::BasicBlock *originalBlock = context->irBuilder->GetInsertBlock();
    llvm::Function *parentFunction = originalBlock->getParent();
    llvm::BasicBlock *elseBlock = llvm::BasicBlock::Create(*context->context, "whileelse");
    llvm::BasicBlock *loopBlock = llvm::BasicBlock::Create(*context->context, "whilebody");
    llvm::BasicBlock *continueBlock = llvm::BasicBlock::Create(*context->context, "whilecont");

    llvm::Value *preConditionValue = this->condition->generateLLVM(context, scope);
    llvm::Value *preConditionResult = context->irBuilder->CreateFCmpONE(preConditionValue, llvm::ConstantFP::get(*context->context, llvm::APFloat(0.0)), "whileprecond");
    context->irBuilder->CreateCondBr(preConditionResult, loopBlock, elseBlock);

    context->irBuilder->SetInsertPoint(loopBlock);
    loopBlock->insertInto(parentFunction);
    auto loopScope = new FunctionScope(*scope);
    std::map<std::string, llvm::PHINode *> namedPhis;
    for (auto const &loopPair : loopScope->namedValues)
    {
        auto phi = context->irBuilder->CreatePHI(llvm::Type::getDoubleTy(*context->context), 2, "whiletmp");
        phi->addIncoming(loopPair.second, originalBlock);
        namedPhis[loopPair.first] = phi;
        loopScope->setValue(loopPair.first, phi);
    }

    this->loopBody->generateLLVM(context, loopScope);

    for (auto const &loopPair : loopScope->namedValues)
    {
        llvm::PHINode *phi = namedPhis[loopPair.first];
        if (phi != NULL)
        {
            phi->addIncoming(loopPair.second, loopBlock);
        }
    }

    llvm::Value *conditionValue = this->condition->generateLLVM(context, loopScope);
    llvm::Value *conditionResult = context->irBuilder->CreateFCmpONE(conditionValue, llvm::ConstantFP::get(*context->context, llvm::APFloat(0.0)), "whilecond");
    context->irBuilder->CreateCondBr(conditionResult, loopBlock, continueBlock);
    loopBlock = context->irBuilder->GetInsertBlock(); // Current block could have changed in generateLLVM calls above, update it here

    context->irBuilder->SetInsertPoint(elseBlock);
    elseBlock->insertInto(parentFunction);
    auto elseScope = new FunctionScope(*scope);
    if (this->elseBody != NULL)
    {
        this->elseBody->generateLLVM(context, elseScope);
    }
    context->irBuilder->CreateBr(continueBlock);
    elseBlock = context->irBuilder->GetInsertBlock(); // Current block could have changed in generateLLVM calls above, update it here

    context->irBuilder->SetInsertPoint(continueBlock);
    continueBlock->insertInto(parentFunction);

    for (auto const &loopPair : loopScope->namedValues)
    {
        for (auto const &elsePair : elseScope->namedValues)
        {
            if (loopPair.first == elsePair.first && loopPair.second != elsePair.second)
            {
                // Combine the values from the else and then blocks are store in current scope
                auto phi = context->irBuilder->CreatePHI(llvm::Type::getDoubleTy(*context->context), 2, "whileconttmp");
                phi->addIncoming(loopPair.second, loopBlock);
                phi->addIncoming(elsePair.second, elseBlock);
                scope->setValue(loopPair.first, phi);
            }
        }
    }

    return continueBlock;
}

llvm::Value *ASTIfStatement::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    llvm::Value *conditionFloat = this->condition->generateLLVM(context, scope);
    llvm::Value *condition = context->irBuilder->CreateFCmpONE(conditionFloat, llvm::ConstantFP::get(*context->context, llvm::APFloat(0.0)), "ifcond");

    llvm::Function *parentFunction = context->irBuilder->GetInsertBlock()->getParent();
    llvm::BasicBlock *thenBlock = llvm::BasicBlock::Create(*context->context, "ifthen");
    llvm::BasicBlock *elseBlock = llvm::BasicBlock::Create(*context->context, "ifelse");
    llvm::BasicBlock *continueBlock = llvm::BasicBlock::Create(*context->context, "ifcont");

    context->irBuilder->CreateCondBr(condition, thenBlock, elseBlock);

    context->irBuilder->SetInsertPoint(thenBlock);
    thenBlock->insertInto(parentFunction);
    auto thenScope = new FunctionScope(*scope);
    this->thenBody->generateLLVM(context, thenScope);
    context->irBuilder->CreateBr(continueBlock);
    thenBlock = context->irBuilder->GetInsertBlock(); // Current block could have changed in generateLLVM calls above, update it here

    context->irBuilder->SetInsertPoint(elseBlock);
    elseBlock->insertInto(parentFunction);
    auto elseScope = new FunctionScope(*scope);
    if (this->elseBody != NULL)
    {
        this->elseBody->generateLLVM(context, elseScope);
    }
    context->irBuilder->CreateBr(continueBlock);
    elseBlock = context->irBuilder->GetInsertBlock(); // Current block could have changed in generateLLVM calls above, update it here

    context->irBuilder->SetInsertPoint(continueBlock);
    continueBlock->insertInto(parentFunction);

    for (auto const &thenPair : thenScope->namedValues)
    {
        for (auto const &elsePair : elseScope->namedValues)
        {
            if (thenPair.first == elsePair.first && thenPair.second != elsePair.second)
            {
                // Combine the values from the else and then blocks are store in current scope
                auto phi = context->irBuilder->CreatePHI(llvm::Type::getDoubleTy(*context->context), 2, "iftmp");
                std::cout << "Combine " << thenPair.first << " (" << thenPair.second << ")"
                          << " and " << elsePair.first << " (" << elsePair.second << ")\n";
                phi->addIncoming(thenPair.second, thenBlock);
                phi->addIncoming(elsePair.second, elseBlock);
                scope->setValue(thenPair.first, phi);
            }
        }
    }

    return continueBlock;
}

llvm::Value *ASTInvocation::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    llvm::Function *functionToCall = context->module->getFunction(this->functionNameToken->value);
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
        llvm::Value *value = node->generateLLVM(context, scope);
        if (!value)
        {
            return NULL;
        }
        functionArgs.push_back(value);
    }

    return context->irBuilder->CreateCall(functionToCall, functionArgs, "calltemp");
}

llvm::Value *ASTBrackets::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    return this->inner->generateLLVM(context, scope);
}

llvm::Value *ASTFile::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    FunctionScope *fileScope = new FunctionScope(*scope);
    for (ASTNode *statement : *this->statements)
    {
        statement->generateLLVM(context, fileScope);
    }
    return NULL;
}
