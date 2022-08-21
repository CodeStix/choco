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
    const Token *exportToken = NULL;
    if (tok->type == TokenType::EXPORT_KEYWORD)
    {
        exportToken = tok;
        tokens.pop_front();
        tok = tokens.front();
    }
    const Token *externToken = NULL;
    if (tok->type == TokenType::EXTERN_KEYWORD)
    {
        externToken = tok;
        tokens.pop_front();
        tok = tokens.front();
    }
    if (tok->type != TokenType::FUNC_KEYWORD)
    {
        if (externToken != NULL)
        {
            tokens.push_front(externToken);
        }
        if (exportToken != NULL)
        {
            tokens.push_front(exportToken);
        }
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

    if (externToken == NULL)
    {
        if (tok->type == TokenType::CURLY_BRACKET_OPEN)
        {
            ASTBlock *body = parseBlock(tokens);
            return new ASTFunction(nameToken, argumentNames, body, exportToken != NULL);
        }
        else
        {
            std::cout << "ERROR: Function must have body are be marked as extern\n";
            return NULL;
        }
    }
    else
    {
        // This is an external function
        return new ASTFunction(nameToken, argumentNames, NULL, exportToken != NULL);
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
        case TokenType::EXPORT_KEYWORD:
        case TokenType::EXTERN_KEYWORD:
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
            std::cout << "ERROR: Invalid statement, unexpected " << getTokenTypeName(tok->type) << "(" << (int)tok->type << ")"
                      << " at " << tok->position << "\n";
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

TypedValue *ASTReadVariable::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    auto value = scope->getValue(this->nameToken->value);
    if (!value)
    {
        std::cout << "BUILD ERROR: Could not find variable '" << this->nameToken->value << "'\n";
    }
    value->setOriginVariable(this->nameToken->value);
    return value;
}

TypedValue *ASTLiteralNumber::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    double parsed = strtod(this->valueToken->value.c_str(), NULL);
    auto value = llvm::ConstantFP::get(*context->context, llvm::APFloat(parsed));
    return new TypedValue(value, new FloatType(32));
}

TypedValue *ASTOperator::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    auto *left = this->left->generateLLVM(context, scope);
    auto *right = this->right->generateLLVM(context, scope);
    if (!left || !right)
    {
        return NULL;
    }

    auto leftValue = left->getValue();
    auto rightValue = right->getValue();
    if (left->getTypeCode() == TypeCode::INTEGER && right->getTypeCode() == TypeCode::INTEGER)
    {
        IntegerType *castedType = NULL;

        auto leftIntType = static_cast<IntegerType *>(left->getType());
        auto rightIntType = static_cast<IntegerType *>(right->getType());
        if (leftIntType == rightIntType)
        {
            // OK!
            castedType = leftIntType; // or rightIntType
        }
        else if (leftIntType->getSigned())
        {
            // Right is unsigned, left is signed but size is equal. Uints have higher priority
            castedType = rightIntType;
        }
        else if (leftIntType->getBitSize() > rightIntType->getBitSize())
        {
            // Right must be expanded
            if (rightIntType->getSigned())
            {
                rightValue = context->irBuilder->CreateSExt(rightValue, leftIntType->getLLVMType(*context->context), "opsextright");
            }
            else
            {
                rightValue = context->irBuilder->CreateZExt(rightValue, leftIntType->getLLVMType(*context->context), "opzextright");
            }
            castedType = leftIntType;
        }
        else if (leftIntType->getBitSize() < rightIntType->getBitSize())
        {
            // Left must be expanded
            if (leftIntType->getSigned())
            {
                leftValue = context->irBuilder->CreateSExt(leftValue, rightIntType->getLLVMType(*context->context), "opsextleft");
            }
            else
            {
                leftValue = context->irBuilder->CreateZExt(leftValue, rightIntType->getLLVMType(*context->context), "opzextleft");
            }
            castedType = rightIntType;
        }
        else
        {
            std::cout << "ERROR: Could not create an add instruction for the given types\n";
            return NULL;
        }

        Type *resultingType = castedType;
        llvm::Value *result;
        switch (this->operatorToken->value[0])
        {
        case '+':
            result = context->irBuilder->CreateAdd(leftValue, rightValue, "addint");
            break;

        case '-':
            result = context->irBuilder->CreateSub(leftValue, rightValue, "opsubint");
            break;

        case '*':
            result = context->irBuilder->CreateMul(leftValue, rightValue, "opmulint");
            break;

        case '/':
            if (castedType->getSigned())
            {
                result = context->irBuilder->CreateSDiv(leftValue, rightValue, "opdivint");
            }
            else
            {
                result = context->irBuilder->CreateUDiv(leftValue, rightValue, "opdivint");
            }
            break;

        case '%':
            if (castedType->getSigned())
            {
                result = context->irBuilder->CreateSRem(leftValue, rightValue, "opmodint");
            }
            else
            {
                result = context->irBuilder->CreateURem(leftValue, rightValue, "opmodint");
            }
            break;

        case '<':
        {
            if (castedType->getSigned())
            {
                result = context->irBuilder->CreateICmpSLT(leftValue, rightValue, "opcmpltint");
            }
            else
            {
                result = context->irBuilder->CreateICmpULT(leftValue, rightValue, "opcmpltint");
            }
            resultingType = &BOOL_TYPE;
            break;
        }

        case '>':
        {
            if (castedType->getSigned())
            {
                result = context->irBuilder->CreateICmpSGT(leftValue, rightValue, "opcmpgtint");
            }
            else
            {
                result = context->irBuilder->CreateICmpUGT(leftValue, rightValue, "opcmpgtint");
            }
            resultingType = &BOOL_TYPE;
            break;
        }

        default:
            std::cout << "ERROR: Invalid operator on integers\n";
            return NULL;
        }

        return new TypedValue(result, resultingType);
    }
    else
    {
        FloatType *castedType = NULL;

        // Left or/and right is a float, ints will be converted to float
        if (left->getTypeCode() == TypeCode::INTEGER)
        {
            // Convert left to float
            FloatType *rightFloatType = static_cast<FloatType *>(left->getType());
            leftValue = context->irBuilder->CreateFPCast(leftValue, rightFloatType->getLLVMType(*context->context), "opfpcast");
            castedType = rightFloatType;
        }
        else if (right->getTypeCode() == TypeCode::INTEGER)
        {
            // Convert right to float
            FloatType *leftFloatType = static_cast<FloatType *>(left->getType());
            rightValue = context->irBuilder->CreateFPCast(rightValue, leftFloatType->getLLVMType(*context->context), "opfpcast");
            castedType = leftFloatType;
        }
        else
        {
            FloatType *leftFloatType = static_cast<FloatType *>(left->getType());
            FloatType *rightFloatType = static_cast<FloatType *>(left->getType());
            if (leftFloatType->getBitSize() > rightFloatType->getBitSize())
            {
                rightValue = context->irBuilder->CreateFPExt(rightValue, leftFloatType->getLLVMType(*context->context), "opfpext");
                castedType = leftFloatType;
            }
            else if (leftFloatType->getBitSize() < rightFloatType->getBitSize())
            {
                leftValue = context->irBuilder->CreateFPExt(leftValue, rightFloatType->getLLVMType(*context->context), "opfpext");
                castedType = rightFloatType;
            }
            else
            {
                castedType = leftFloatType; // or rightFloatType
            }
        }

        Type *resultingType = castedType;
        llvm::Value *result;
        switch (this->operatorToken->value[0])
        {
        case '+':
            result = context->irBuilder->CreateFAdd(leftValue, rightValue, "opaddfp");
            break;

        case '-':
            result = context->irBuilder->CreateFSub(leftValue, rightValue, "opsubfp");
            break;

        case '*':
            result = context->irBuilder->CreateFMul(leftValue, rightValue, "opmulfp");
            break;

        case '/':
            result = context->irBuilder->CreateFDiv(leftValue, rightValue, "opdivfp");
            break;

        case '%':
            result = context->irBuilder->CreateFRem(leftValue, rightValue, "opmodfp");
            break;

        case '<':
        {
            result = context->irBuilder->CreateFCmpULT(leftValue, rightValue, "opcmpltfp");
            resultingType = &BOOL_TYPE;
            break;
        }

        case '>':
        {
            result = context->irBuilder->CreateFCmpUGT(leftValue, rightValue, "opcmpgtfp");
            resultingType = &BOOL_TYPE;
            break;
        }

        default:
            std::cout << "ERROR: Invalid operator on floats\n";
            return NULL;
        }

        return new TypedValue(result, resultingType);
    }
}

TypedValue *ASTLiteralString::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    auto value = context->irBuilder->CreateGlobalString(this->valueToken->value, "str");
    auto global = new llvm::GlobalVariable(llvm::PointerType::get(*context->context, 0), true, llvm::GlobalValue::LinkageTypes::PrivateLinkage, value);
    return new TypedValue(global, new ArrayType(&CHAR_TYPE));
}

TypedValue *ASTDeclaration::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    TypedValue *value;
    if (this->value != NULL)
    {
        // There is an assignment after the declaration
        value = this->value->generateLLVM(context, scope); // TODO: get from type specifier
    }
    else
    {
        // Uninitialized value
        value = new TypedValue(NULL, new FloatType(32)); // TODO: get from type specifier
    }

    if (scope->hasValue(this->nameToken->value))
    {
        std::cout << "BUILD ERROR: Cannot redeclare '" << this->nameToken->value << "', it has already been declared\n";
        return NULL;
    }
    scope->setValue(this->nameToken->value, value);
    return value;
}

TypedValue *ASTAssignment::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    if (!scope->hasValue(this->nameToken->value))
    {
        std::cout << "BUILD ERROR: Cannot set variable '" << this->nameToken->value << "', it is not found\n";
        return NULL;
    }
    else
    {
        TypedValue *newValue = this->value->generateLLVM(context, scope);
        TypedValue *scopeValue = scope->getValue(this->nameToken->value);
        if (scopeValue == NULL)
        {
            std::cout << "BUILD ERROR: Cannot set variable '" << this->nameToken->value << "', it is not found\n";
            return NULL;
        }
        if (*newValue->getType() != *scopeValue->getType())
        {
            std::cout << "BUILD ERROR: Cannot set variable '" << this->nameToken->value << "', assignment has different type\n";
            return NULL;
        }
        scopeValue->setValue(newValue->getValue());
        return scopeValue;
    }
}

TypedValue *ASTReturn::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    if (this->value != NULL)
    {
        auto value = this->value->generateLLVM(context, scope);
        context->irBuilder->CreateRet(value->getValue());
        return value;
    }
    else
    {
        context->irBuilder->CreateRetVoid();
        return NULL;
    }
}

TypedValue *ASTBlock::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    TypedValue *lastValue = NULL;
    for (ASTNode *statement : *this->statements)
    {
        lastValue = statement->generateLLVM(context, scope);
    }
    return lastValue;
}

TypedValue *ASTFunction::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    llvm::Function *function = context->module->getFunction(this->nameToken->value);

    if (function == NULL)
    {
        // Define it
        std::vector<llvm::Type *> argumentTypes(this->arguments->size(), llvm::Type::getDoubleTy(*context->context));
        llvm::FunctionType *functionType = llvm::FunctionType::get(llvm::Type::getDoubleTy(*context->context), argumentTypes, false);

        function = llvm::Function::Create(functionType, this->exported ? llvm::Function::ExternalLinkage : llvm::Function::PrivateLinkage, this->nameToken->value, *context->module);
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
            functionScope->setValue(arg.getName().str(), new TypedValue(&arg, new FloatType(32))); // TODO get from type specfier
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

    return new TypedValue(function, new FunctionType());
}

TypedValue *ASTWhileStatement::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    TypedValue *preConditionValue = this->condition->generateLLVM(context, scope);
    if (*preConditionValue->getType() != BOOL_TYPE)
    {
        std::cout << "While condition must be a bool (UInt1) type!\n";
        return NULL;
    }

    llvm::BasicBlock *originalBlock = context->irBuilder->GetInsertBlock();
    llvm::Function *parentFunction = originalBlock->getParent();
    llvm::BasicBlock *elseBlock = llvm::BasicBlock::Create(*context->context, "whileelse");
    llvm::BasicBlock *loopBlock = llvm::BasicBlock::Create(*context->context, "whilebody");
    llvm::BasicBlock *continueBlock = llvm::BasicBlock::Create(*context->context, "whilecont");

    // llvm::Value *preConditionResult = context->irBuilder->CreateFCmpONE(preConditionValue, llvm::ConstantFP::get(*context->context, llvm::APFloat(0.0)), "whileprecond");
    context->irBuilder->CreateCondBr(preConditionValue->getValue(), loopBlock, elseBlock);

    context->irBuilder->SetInsertPoint(loopBlock);
    loopBlock->insertInto(parentFunction);
    auto loopScope = new FunctionScope(*scope);
    std::map<std::string, llvm::PHINode *> namedPhis;
    for (auto const &loopPair : loopScope->namedValues)
    {
        auto phi = context->irBuilder->CreatePHI(llvm::Type::getDoubleTy(*context->context), 2, "whiletmp");
        phi->addIncoming(loopPair.second->getValue(), originalBlock);
        namedPhis[loopPair.first] = phi;
        loopPair.second->setValue(phi);
    }

    this->loopBody->generateLLVM(context, loopScope);

    for (auto const &loopPair : loopScope->namedValues)
    {
        llvm::PHINode *phi = namedPhis[loopPair.first];
        if (phi != NULL)
        {
            phi->addIncoming(loopPair.second->getValue(), loopBlock);
        }
    }

    TypedValue *conditionValue = this->condition->generateLLVM(context, loopScope);
    if (*conditionValue->getType() != BOOL_TYPE)
    {
        std::cout << "While condition must be a bool (UInt1) type!\n";
        return NULL;
    }

    // llvm::Value *conditionResult = context->irBuilder->CreateFCmpONE(conditionValue->getValue(), llvm::ConstantFP::get(*context->context, llvm::APFloat(0.0)), "whilecond");
    context->irBuilder->CreateCondBr(conditionValue->getValue(), loopBlock, continueBlock);
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
            if (loopPair.first == elsePair.first && scope->hasValue(loopPair.first) && loopPair.second->getValue() != elsePair.second->getValue())
            {
                TypedValue *scopedValue = scope->getValue(loopPair.first);
                if (*loopPair.second->getType() != *elsePair.second->getType() || *loopPair.second->getType() != *scopedValue->getType())
                {
                    std::cout << "ERROR: While statement else and loop variables do not match in types\n";
                    return NULL;
                }
                // Combine the values from the else and then blocks are store in current scope
                auto phi = context->irBuilder->CreatePHI(llvm::Type::getDoubleTy(*context->context), 2, "whileconttmp");
                phi->addIncoming(loopPair.second->getValue(), loopBlock);
                phi->addIncoming(elsePair.second->getValue(), elseBlock);
                scopedValue->setValue(phi);
            }
        }
    }

    return NULL;
}

TypedValue *ASTIfStatement::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    TypedValue *condition = this->condition->generateLLVM(context, scope);
    // llvm::Value *condition = context->irBuilder->CreateFCmpONE(conditionFloat, llvm::ConstantFP::get(*context->context, llvm::APFloat(0.0)), "ifcond");
    if (*condition->getType() != BOOL_TYPE)
    {
        std::cout << "While condition must be a bool (UInt1) type!\n";
        return NULL;
    }

    llvm::Function *parentFunction = context->irBuilder->GetInsertBlock()->getParent();
    llvm::BasicBlock *thenBlock = llvm::BasicBlock::Create(*context->context, "ifthen");
    llvm::BasicBlock *elseBlock = llvm::BasicBlock::Create(*context->context, "ifelse");
    llvm::BasicBlock *continueBlock = llvm::BasicBlock::Create(*context->context, "ifcont");

    context->irBuilder->CreateCondBr(condition->getValue(), thenBlock, elseBlock);

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
            if (thenPair.first == elsePair.first && scope->hasValue(thenPair.first) && thenPair.second->getValue() != elsePair.second->getValue())
            {
                TypedValue *scopedValue = scope->getValue(thenPair.first);
                if (*thenPair.second->getType() != *elsePair.second->getType() || *thenPair.second->getType() != *scopedValue->getType())
                {
                    std::cout << "ERROR: If statement then and else variables do not match in types\n";
                    return NULL;
                }

                // Combine the values from the else and then blocks are store in current scope
                auto phi = context->irBuilder->CreatePHI(llvm::Type::getDoubleTy(*context->context), 2, "iftmp");
                // std::cout << "Combine " << thenPair.first << " (" << thenPair.second << ")"
                //           << " and " << elsePair.first << " (" << elsePair.second << ")\n";
                phi->addIncoming(thenPair.second->getValue(), thenBlock);
                phi->addIncoming(elsePair.second->getValue(), elseBlock);
                scopedValue->setValue(phi);
            }
        }
    }

    return NULL;
}

TypedValue *ASTInvocation::generateLLVM(GenerationContext *context, FunctionScope *scope)
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
        TypedValue *value = node->generateLLVM(context, scope);
        if (!value || !value->getValue())
        {
            return NULL;
        }
        functionArgs.push_back(value->getValue());
    }

    llvm::Value *callResult = context->irBuilder->CreateCall(functionToCall, functionArgs, "call");
    return new TypedValue(callResult, new FloatType(32)); // TODO get from type specifier
}

TypedValue *ASTBrackets::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    return this->inner->generateLLVM(context, scope);
}

TypedValue *ASTFile::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    FunctionScope *fileScope = new FunctionScope(*scope);
    for (ASTNode *statement : *this->statements)
    {
        statement->generateLLVM(context, fileScope);
    }
    return NULL;
}
