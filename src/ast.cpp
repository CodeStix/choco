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

ASTParameter *parseParameter(std::list<const Token *> &tokens)
{
    const Token *tok = tokens.front();
    if (tok == NULL)
    {
        std::cout << "ERROR: Unexpected end of file\n";
        return NULL;
    }
    if (tok->type != TokenType::SYMBOL)
    {
        std::cout << "ERROR: Type must be a name\n";
        return NULL;
    }
    const Token *nameToken = tok;
    tokens.pop_front();
    tok = tokens.front();
    if (tok == NULL)
    {
        std::cout << "ERROR: Unexpected end of file\n";
        return NULL;
    }

    ASTType *typeSpecifier;
    if (tok->type == TokenType::COLON)
    {
        // Parse type specifier
        tokens.pop_front();
        typeSpecifier = parseType(tokens);
        if (typeSpecifier == NULL)
        {
            std::cout << "ERROR: Could not parse type specifier\n";
            return NULL;
        }
    }
    else
    {
        typeSpecifier = NULL;
    }

    if (typeSpecifier == NULL)
    {
        // TODO this should be allowed when implementing an interface function
        std::cout << "ERROR: function argument must specify a type (it does not implement a known signature)\n";
        return NULL;
    }

    return new ASTParameter(nameToken, typeSpecifier);
}

ASTType *parseType(std::list<const Token *> &tokens)
{
    const Token *tok = tokens.front();
    if (tok == NULL)
    {
        std::cout << "ERROR: Unexpected end of file\n";
        return NULL;
    }
    if (tok->type != TokenType::SYMBOL)
    {
        std::cout << "ERROR: Type must be a name\n";
        return NULL;
    }
    const Token *nameToken = tok;
    tokens.pop_front();
    return new ASTType(nameToken);
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

    std::vector<ASTParameter *> *parameters = new std::vector<ASTParameter *>();
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

        ASTParameter *parameter = parseParameter(tokens);
        if (parameter == NULL)
        {
            std::cout << "ERROR: Could not parse function parameter\n";
            return NULL;
        }
        parameters->push_back(parameter);

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

    ASTType *returnType;
    if (tok->type == TokenType::COLON)
    {
        tokens.pop_front();
        returnType = parseType(tokens);
        if (returnType == NULL)
        {
            std::cout << "ERROR: Could not parse function return type\n";
            return NULL;
        }
    }
    else
    {
        returnType = NULL;
    }

    if (externToken == NULL)
    {
        tok = tokens.front();
        if (tok == NULL)
        {
            std::cout << "ERROR: Unexpected end of file\n";
            return NULL;
        }

        if (tok->type == TokenType::CURLY_BRACKET_OPEN)
        {
            ASTBlock *body = parseBlock(tokens);
            return new ASTFunction(nameToken, parameters, returnType, body, exportToken != NULL);
        }
        else
        {
            std::cout << "ERROR: Function must have body are be marked as extern " << (int)tok->type << "\n";
            return NULL;
        }
    }
    else
    {
        // This is an external function
        return new ASTFunction(nameToken, parameters, returnType, NULL, exportToken != NULL);
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

    case TokenType::LET_KEYWORD:
    {
        value = parseDeclaration(tokens);
        if (value == NULL)
        {
            std::cout << "ERROR: Unexpected let \n";
            return NULL;
        }
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
    ASTType *typeSpecifier;
    if (tok->type == TokenType::COLON)
    {
        tokens.pop_front();
        typeSpecifier = parseType(tokens);
    }
    else
    {
        typeSpecifier = NULL;
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

        return new ASTDeclaration(nameToken, value, typeSpecifier);
    }
    else
    {
        return new ASTDeclaration(nameToken, NULL, typeSpecifier);
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

llvm::AllocaInst *createAllocaInCurrentFunction(GenerationContext *context, llvm::Type *type, llvm::StringRef twine = "")
{
    llvm::Function *function = context->irBuilder->GetInsertBlock()->getParent();
    llvm::IRBuilder<> insertBuilder(&function->getEntryBlock(), function->getEntryBlock().begin());
    return insertBuilder.CreateAlloca(type, NULL, twine);
}

TypedValue *ASTReadVariable::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    auto valuePointer = scope->getValue(this->nameToken->value);
    if (!valuePointer)
    {
        std::cout << "BUILD ERROR: Could not find variable '" << this->nameToken->value << "'\n";
    }
    valuePointer->setOriginVariable(this->nameToken->value);

    auto value = context->irBuilder->CreateLoad(valuePointer->getType()->getLLVMType(*context->context), valuePointer->getValue(), "load");
    return new TypedValue(value, valuePointer->getType());
}

TypedValue *ASTLiteralNumber::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    double parsed = strtod(this->valueToken->value.c_str(), NULL);
    Type *type = new FloatType(32);
    auto value = llvm::ConstantFP::get(type->getLLVMType(*context->context), parsed);
    return new TypedValue(value, type);
}

// Converts the left or right value to match the other one's type without losing precision
bool generateTypeJugging(GenerationContext *context, TypedValue **leftInOut, TypedValue **rightInOut)
{
    auto leftType = (*leftInOut)->getType();
    auto rightType = (*leftInOut)->getType();
    if (*leftType == *rightType)
    {
        return true;
    }

    auto leftValue = (*leftInOut)->getValue();
    auto rightValue = (*leftInOut)->getValue();

    if (leftType->getTypeCode() == TypeCode::INTEGER && rightType->getTypeCode() == TypeCode::INTEGER)
    {
        auto leftIntType = static_cast<IntegerType *>(leftType);
        auto rightIntType = static_cast<IntegerType *>(rightType);

        if (leftIntType->getBitSize() > rightIntType->getBitSize())
        {
            // Right must be converted to match left int size
            if (leftIntType->getSigned() && rightIntType->getSigned())
            {
                rightValue = context->irBuilder->CreateSExt(rightValue, leftIntType->getLLVMType(*context->context), "jugglesext");
            }
            else
            {
                rightValue = context->irBuilder->CreateZExt(rightValue, leftIntType->getLLVMType(*context->context), "jugglezext");
            }
            *rightInOut = new TypedValue(rightValue, leftIntType);
        }
        else if (leftIntType->getBitSize() < rightIntType->getBitSize())
        {
            // Left must be converted to match right int size
            if (leftIntType->getSigned() && rightIntType->getSigned())
            {
                leftValue = context->irBuilder->CreateSExt(leftValue, rightIntType->getLLVMType(*context->context), "jugglesext");
            }
            else
            {
                leftValue = context->irBuilder->CreateZExt(leftValue, rightIntType->getLLVMType(*context->context), "jugglezext");
            }
            *leftInOut = new TypedValue(leftValue, rightIntType);
        }

        return true;
    }
    else if (leftType->getTypeCode() == TypeCode::INTEGER && rightType->getTypeCode() == TypeCode::FLOAT)
    {
        auto leftIntType = static_cast<IntegerType *>(leftType);
        auto rightFloatType = static_cast<FloatType *>(rightType);

        // TODO: check if float can fit integer precision

        if (leftIntType->getSigned())
        {
            leftValue = context->irBuilder->CreateSIToFP(leftValue, rightFloatType->getLLVMType(*context->context), "jugglefp");
        }
        else
        {
            leftValue = context->irBuilder->CreateUIToFP(leftValue, rightFloatType->getLLVMType(*context->context), "jugglefp");
        }

        *leftInOut = new TypedValue(leftValue, rightFloatType);
        return true;
    }
    else if (leftType->getTypeCode() == TypeCode::FLOAT && rightType->getTypeCode() == TypeCode::INTEGER)
    {
        auto leftFloatType = static_cast<FloatType *>(leftType);
        auto rightIntType = static_cast<IntegerType *>(rightType);

        // TODO: check if float can fit integer precision

        if (rightIntType->getSigned())
        {
            rightValue = context->irBuilder->CreateSIToFP(rightValue, leftFloatType->getLLVMType(*context->context), "jugglefp");
        }
        else
        {
            rightValue = context->irBuilder->CreateUIToFP(rightValue, leftFloatType->getLLVMType(*context->context), "jugglefp");
        }

        *rightInOut = new TypedValue(rightValue, leftFloatType);
        return true;
    }
    else if (leftType->getTypeCode() == TypeCode::FLOAT && rightType->getTypeCode() == TypeCode::FLOAT)
    {
        auto leftFloatType = static_cast<FloatType *>(leftType);
        auto rightFloatType = static_cast<FloatType *>(rightType);

        if (leftFloatType->getBitSize() > rightFloatType->getBitSize())
        {
            rightValue = context->irBuilder->CreateFPExt(rightValue, leftFloatType->getLLVMType(*context->context), "jugglefpext");
            *rightInOut = new TypedValue(rightValue, leftFloatType);
        }
        else if (leftFloatType->getBitSize() < rightFloatType->getBitSize())
        {
            leftValue = context->irBuilder->CreateFPExt(leftValue, rightFloatType->getLLVMType(*context->context), "jugglefpext");
            *leftInOut = new TypedValue(leftValue, rightFloatType);
        }

        return true;
    }
    else
    {

        return false;
    }
}

// Try to cast a value to a specific type
TypedValue *generateTypeConversion(GenerationContext *context, TypedValue *valueToConvert, Type *targetType, bool allowLosePrecision)
{
    llvm::Value *currentValue = valueToConvert->getValue();
    Type *currentType = valueToConvert->getType();
    if (targetType->getTypeCode() == TypeCode::INTEGER && currentType->getTypeCode() == TypeCode::INTEGER)
    {
        IntegerType *currentIntType = static_cast<IntegerType *>(currentType);
        IntegerType *targetIntType = static_cast<IntegerType *>(targetType);

        if (targetIntType->getBitSize() > currentIntType->getBitSize())
        {
            // Target type has more bits, this cast may be implicit
            if (targetIntType->getSigned() == currentIntType->getSigned())
            {
                // Target type has the same signedness, this cast may be implicit
                if (targetIntType->getSigned())
                {
                    currentValue = context->irBuilder->CreateZExt(currentValue, targetIntType->getLLVMType(*context->context), "convzextint");
                }
                else
                {
                    currentValue = context->irBuilder->CreateSExt(currentValue, targetIntType->getLLVMType(*context->context), "convzextint");
                }
            }
            else
            {
                if (!allowLosePrecision)
                {
                    return NULL;
                }

                // TODO: is this the right was to convert
                if (targetIntType->getSigned())
                {
                    currentValue = context->irBuilder->CreateZExt(currentValue, targetIntType->getLLVMType(*context->context), "convzextint");
                }
                else
                {
                    currentValue = context->irBuilder->CreateSExt(currentValue, targetIntType->getLLVMType(*context->context), "convzextint");
                }
            }
        }
        else if (targetIntType->getBitSize() < currentIntType->getBitSize())
        {
            if (!allowLosePrecision)
            {
                return NULL;
            }

            currentValue = context->irBuilder->CreateTrunc(currentValue, targetIntType->getLLVMType(*context->context), "convtruncint");
        }
    }
    else if (targetType->getTypeCode() == TypeCode::FLOAT && currentType->getTypeCode() == TypeCode::FLOAT)
    {
        FloatType *currentFloatType = static_cast<FloatType *>(currentType);
        FloatType *targetFloatType = static_cast<FloatType *>(targetType);

        if (targetFloatType->getBitSize() > currentFloatType->getBitSize())
        {
            currentValue = context->irBuilder->CreateFPExt(currentValue, targetFloatType->getLLVMType(*context->context), "convfpext");
        }
        else if (targetFloatType->getBitSize() < currentFloatType->getBitSize())
        {
            if (!allowLosePrecision)
            {
                return NULL;
            }

            currentValue = context->irBuilder->CreateFPTrunc(currentValue, targetFloatType->getLLVMType(*context->context), "convtruncfp");
        }
    }
    else if (targetType->getTypeCode() == TypeCode::FLOAT && currentType->getTypeCode() == TypeCode::INTEGER)
    {
        // TODO: this operation can be done without losing precision in some cases
        if (!allowLosePrecision)
        {
            return NULL;
        }

        IntegerType *currentIntType = static_cast<IntegerType *>(currentType);
        FloatType *targetFloatType = static_cast<FloatType *>(targetType);
        if (currentIntType->getSigned())
        {
            currentValue = context->irBuilder->CreateSIToFP(currentValue, targetFloatType->getLLVMType(*context->context), "convsitofp");
        }
        else
        {
            currentValue = context->irBuilder->CreateUIToFP(currentValue, targetFloatType->getLLVMType(*context->context), "convuitofp");
        }
    }
    else if (targetType->getTypeCode() == TypeCode::INTEGER && currentType->getTypeCode() == TypeCode::FLOAT)
    {
        if (!allowLosePrecision)
        {
            return NULL;
        }

        FloatType *currentFloatType = static_cast<FloatType *>(currentType);
        IntegerType *targetIntType = static_cast<IntegerType *>(targetType);

        if (targetIntType->getSigned())
        {
            currentValue = context->irBuilder->CreateFPToSI(currentValue, targetIntType->getLLVMType(*context->context), "convfptosi");
        }
        else
        {
            currentValue = context->irBuilder->CreateFPToUI(currentValue, targetIntType->getLLVMType(*context->context), "convfptoui");
        }
    }
    else
    {
        // Cannot convert type automatically
        return NULL;
    }

    return new TypedValue(currentValue, targetType);
}

TypedValue *ASTOperator::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    auto *left = this->left->generateLLVM(context, scope);
    auto *right = this->right->generateLLVM(context, scope);
    if (!left || !right)
    {
        return NULL;
    }

    if (!generateTypeJugging(context, &left, &right))
    {
        std::cout << "ERROR: Cannot " << this->operatorToken->value << " values, their types cannot be matched\n";
        return NULL;
    }

    auto sharedType = left->getType(); // or right->getType()
    auto leftValue = left->getValue();
    auto rightValue = right->getValue();
    if (sharedType->getTypeCode() == TypeCode::FLOAT)
    {
        FloatType *sharedFloatType = static_cast<FloatType *>(sharedType);
        Type *resultingType = sharedType;
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
    else if (sharedType->getTypeCode() == TypeCode::INTEGER)
    {
        IntegerType *sharedIntType = static_cast<IntegerType *>(sharedType);
        Type *resultingType = sharedType;
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
            if (sharedIntType->getSigned())
            {
                result = context->irBuilder->CreateSDiv(leftValue, rightValue, "opdivint");
            }
            else
            {
                result = context->irBuilder->CreateUDiv(leftValue, rightValue, "opdivint");
            }
            break;

        case '%':
            if (sharedIntType->getSigned())
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
            if (sharedIntType->getSigned())
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
            if (sharedIntType->getSigned())
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
        std::cout << "ERROR: Cannot " << this->operatorToken->value << " values, the type does not support this operator\n";
        return NULL;
    }

    // auto leftValue = left->getValue();
    // auto rightValue = right->getValue();
    // if (left->getTypeCode() == TypeCode::INTEGER && right->getTypeCode() == TypeCode::INTEGER)
    // {
    //     IntegerType *castedType = NULL;

    //     auto leftIntType = static_cast<IntegerType *>(left->getType());
    //     auto rightIntType = static_cast<IntegerType *>(right->getType());
    //     if (leftIntType == rightIntType)
    //     {
    //         // OK!
    //         castedType = leftIntType; // or rightIntType
    //     }
    //     else if (leftIntType->getSigned())
    //     {
    //         // Right is unsigned, left is signed but size is equal. Uints have higher priority
    //         castedType = rightIntType;
    //     }
    //     else if (leftIntType->getBitSize() > rightIntType->getBitSize())
    //     {
    //         // Right must be expanded
    //         if (rightIntType->getSigned())
    //         {
    //             rightValue = context->irBuilder->CreateSExt(rightValue, leftIntType->getLLVMType(*context->context), "opsextright");
    //         }
    //         else
    //         {
    //             rightValue = context->irBuilder->CreateZExt(rightValue, leftIntType->getLLVMType(*context->context), "opzextright");
    //         }
    //         castedType = leftIntType;
    //     }
    //     else if (leftIntType->getBitSize() < rightIntType->getBitSize())
    //     {
    //         // Left must be expanded
    //         if (leftIntType->getSigned())
    //         {
    //             leftValue = context->irBuilder->CreateSExt(leftValue, rightIntType->getLLVMType(*context->context), "opsextleft");
    //         }
    //         else
    //         {
    //             leftValue = context->irBuilder->CreateZExt(leftValue, rightIntType->getLLVMType(*context->context), "opzextleft");
    //         }
    //         castedType = rightIntType;
    //     }
    //     else
    //     {
    //         std::cout << "ERROR: Could not create an add instruction for the given types\n";
    //         return NULL;
    //     }

    //     Type *resultingType = castedType;
    //     llvm::Value *result;
    //     switch (this->operatorToken->value[0])
    //     {
    //     case '+':
    //         result = context->irBuilder->CreateAdd(leftValue, rightValue, "addint");
    //         break;

    //     case '-':
    //         result = context->irBuilder->CreateSub(leftValue, rightValue, "opsubint");
    //         break;

    //     case '*':
    //         result = context->irBuilder->CreateMul(leftValue, rightValue, "opmulint");
    //         break;

    //     case '/':
    //         if (castedType->getSigned())
    //         {
    //             result = context->irBuilder->CreateSDiv(leftValue, rightValue, "opdivint");
    //         }
    //         else
    //         {
    //             result = context->irBuilder->CreateUDiv(leftValue, rightValue, "opdivint");
    //         }
    //         break;

    //     case '%':
    //         if (castedType->getSigned())
    //         {
    //             result = context->irBuilder->CreateSRem(leftValue, rightValue, "opmodint");
    //         }
    //         else
    //         {
    //             result = context->irBuilder->CreateURem(leftValue, rightValue, "opmodint");
    //         }
    //         break;

    //     case '<':
    //     {
    //         if (castedType->getSigned())
    //         {
    //             result = context->irBuilder->CreateICmpSLT(leftValue, rightValue, "opcmpltint");
    //         }
    //         else
    //         {
    //             result = context->irBuilder->CreateICmpULT(leftValue, rightValue, "opcmpltint");
    //         }
    //         resultingType = &BOOL_TYPE;
    //         break;
    //     }

    //     case '>':
    //     {
    //         if (castedType->getSigned())
    //         {
    //             result = context->irBuilder->CreateICmpSGT(leftValue, rightValue, "opcmpgtint");
    //         }
    //         else
    //         {
    //             result = context->irBuilder->CreateICmpUGT(leftValue, rightValue, "opcmpgtint");
    //         }
    //         resultingType = &BOOL_TYPE;
    //         break;
    //     }

    //     default:
    //         std::cout << "ERROR: Invalid operator on integers\n";
    //         return NULL;
    //     }

    //     return new TypedValue(result, resultingType);
    // }
    // else
    // {
    //     FloatType *castedType = NULL;

    //     // Left or/and right is a float, ints will be converted to float
    //     if (left->getTypeCode() == TypeCode::INTEGER)
    //     {
    //         // Convert left to float
    //         FloatType *rightFloatType = static_cast<FloatType *>(left->getType());
    //         leftValue = context->irBuilder->CreateFPCast(leftValue, rightFloatType->getLLVMType(*context->context), "opfpcast");
    //         castedType = rightFloatType;
    //     }
    //     else if (right->getTypeCode() == TypeCode::INTEGER)
    //     {
    //         // Convert right to float
    //         FloatType *leftFloatType = static_cast<FloatType *>(left->getType());
    //         rightValue = context->irBuilder->CreateFPCast(rightValue, leftFloatType->getLLVMType(*context->context), "opfpcast");
    //         castedType = leftFloatType;
    //     }
    //     else
    //     {
    //         FloatType *leftFloatType = static_cast<FloatType *>(left->getType());
    //         FloatType *rightFloatType = static_cast<FloatType *>(left->getType());
    //         if (leftFloatType->getBitSize() > rightFloatType->getBitSize())
    //         {
    //             rightValue = context->irBuilder->CreateFPExt(rightValue, leftFloatType->getLLVMType(*context->context), "opfpext");
    //             castedType = leftFloatType;
    //         }
    //         else if (leftFloatType->getBitSize() < rightFloatType->getBitSize())
    //         {
    //             leftValue = context->irBuilder->CreateFPExt(leftValue, rightFloatType->getLLVMType(*context->context), "opfpext");
    //             castedType = rightFloatType;
    //         }
    //         else
    //         {
    //             castedType = leftFloatType; // or rightFloatType
    //         }
    //     }

    //     Type *resultingType = castedType;
    //     llvm::Value *result;
    //     switch (this->operatorToken->value[0])
    //     {
    //     case '+':
    //         result = context->irBuilder->CreateFAdd(leftValue, rightValue, "opaddfp");
    //         break;

    //     case '-':
    //         result = context->irBuilder->CreateFSub(leftValue, rightValue, "opsubfp");
    //         break;

    //     case '*':
    //         result = context->irBuilder->CreateFMul(leftValue, rightValue, "opmulfp");
    //         break;

    //     case '/':
    //         result = context->irBuilder->CreateFDiv(leftValue, rightValue, "opdivfp");
    //         break;

    //     case '%':
    //         result = context->irBuilder->CreateFRem(leftValue, rightValue, "opmodfp");
    //         break;

    //     case '<':
    //     {
    //         result = context->irBuilder->CreateFCmpULT(leftValue, rightValue, "opcmpltfp");
    //         resultingType = &BOOL_TYPE;
    //         break;
    //     }

    //     case '>':
    //     {
    //         result = context->irBuilder->CreateFCmpUGT(leftValue, rightValue, "opcmpgtfp");
    //         resultingType = &BOOL_TYPE;
    //         break;
    //     }

    //     default:
    //         std::cout << "ERROR: Invalid operator on floats\n";
    //         return NULL;
    //     }

    //     return new TypedValue(result, resultingType);
    // }
}

TypedValue *ASTLiteralString::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    auto value = context->irBuilder->CreateGlobalString(this->valueToken->value, "str");
    Type *type = new ArrayType(&CHAR_TYPE, this->valueToken->value.length() + 1);
    return new TypedValue(value, type);
}

TypedValue *ASTDeclaration::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    if (scope->hasValue(this->nameToken->value))
    {
        std::cout << "BUILD ERROR: Cannot redeclare '" << this->nameToken->value << "', it has already been declared\n";
        return NULL;
    }

    TypedValue *initialValue;
    if (this->value != NULL)
    {
        initialValue = this->value->generateLLVM(context, scope);
    }
    else
    {
        initialValue = NULL;
    }

    Type *specifiedType;
    if (this->typeSpecifier != NULL)
    {
        specifiedType = this->typeSpecifier->getSpecifiedType();
    }
    else
    {
        specifiedType = NULL;
    }

    Type *pointerType;
    if (specifiedType != NULL)
    {
        pointerType = specifiedType;
    }
    else
    {
        if (initialValue != NULL)
        {
            pointerType = initialValue->getType();
        }
        else
        {
            std::cout << "ERROR: declaration type must be specifier when value is missing\n";
            return NULL;
        }
    }

    llvm::Value *pointerValue = createAllocaInCurrentFunction(context, pointerType->getLLVMType(*context->context), "alloca");
    TypedValue *pointer = new TypedValue(pointerValue, pointerType);
    scope->addValue(this->nameToken->value, pointer);

    if (initialValue != NULL)
    {
        TypedValue *convertedValue;
        if (specifiedType != NULL)
        {
            convertedValue = generateTypeConversion(context, initialValue, specifiedType, true);
            if (convertedValue == NULL)
            {
                std::cout << "ERROR: Cannot declare variable '" << this->nameToken->value << "', right-hand side has invalid assignment type\n";
                return NULL;
            }
        }
        else
        {
            convertedValue = initialValue;
        }

        bool isVolatile = false;
        context->irBuilder->CreateStore(convertedValue->getValue(), pointer->getValue(), isVolatile);
    }

    return initialValue;
}

TypedValue *ASTAssignment::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    if (!scope->hasValue(this->nameToken->value))
    {
        std::cout << "ERROR: Cannot set variable '" << this->nameToken->value << "', it is not found\n";
        return NULL;
    }
    else
    {
        TypedValue *newValue = this->value->generateLLVM(context, scope);
        TypedValue *pointedValue = scope->getValue(this->nameToken->value);
        if (pointedValue == NULL)
        {
            std::cout << "ERROR: Cannot set variable '" << this->nameToken->value << "', it is not found\n";
            return NULL;
        }

        TypedValue *convertedValue;
        if (*pointedValue->getType() != *newValue->getType())
        {
            convertedValue = generateTypeConversion(context, newValue, pointedValue->getType(), true); // TODO: remove allowLosePrecision when casts are supported
            if (convertedValue == NULL)
            {
                std::cout << "ERROR: Cannot set variable '" << this->nameToken->value << "', right-hand side has invalid type\n";
                return NULL;
            }
        }
        else
        {
            convertedValue = newValue;
        }

        bool isVolatile = false;
        context->irBuilder->CreateStore(convertedValue->getValue(), pointedValue->getValue(), isVolatile);
        return newValue;
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
        std::vector<llvm::Type *> parameterTypes; // TODO get from type specifier
        for (ASTParameter *parameter : *this->parameters)
        {
            parameterTypes.push_back(parameter->getSpecifiedType()->getLLVMType(*context->context));
        }

        llvm::Type *returnType;
        if (this->returnType != NULL)
        {
            returnType = this->returnType->getSpecifiedType()->getLLVMType(*context->context);
        }
        else
        {
            returnType = llvm::Type::getVoidTy(*context->context);
        }

        bool isVarArg = false;
        llvm::FunctionType *functionType = llvm::FunctionType::get(returnType, parameterTypes, isVarArg);
        function = llvm::Function::Create(functionType, this->exported ? llvm::Function::ExternalLinkage : llvm::Function::PrivateLinkage, this->nameToken->value, *context->module);
        if (function == NULL)
        {
            std::cout << "BUILD ERROR: Function::Create returned null\n";
            return NULL;
        }

        int i = 0;
        for (auto &arg : function->args())
        {
            ASTParameter *parameter = (*this->parameters)[i++];
            arg.setName(parameter->getParameterName());
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
        llvm::BasicBlock *functionBlock = llvm::BasicBlock::Create(*context->context, "block", function);
        context->irBuilder->SetInsertPoint(functionBlock);

        int i = 0;
        for (auto &parameterValue : function->args())
        {
            ASTParameter *parameter = (*this->parameters)[i++];
            Type *parameterType = parameter->getSpecifiedType();
            auto parameterPointer = context->irBuilder->CreateAlloca(parameterType->getLLVMType(*context->context), NULL, "loadarg");
            context->irBuilder->CreateStore(&parameterValue, parameterPointer, false);
            functionScope->addValue(parameter->getParameterName(), new TypedValue(parameterPointer, parameterType));
        }

        // for (auto &arg : function->args())
        // {
        //     auto argType = new FloatType(32); // TODO get from type specifier
        //     auto argPointer = context->irBuilder->CreateAlloca(argType->getLLVMType(*context->context), NULL, "loadarg");
        //     context->irBuilder->CreateStore(&arg, argPointer, false);
        //     functionScope->addValue(arg.getName().str(), new TypedValue(argPointer, argType));
        // }

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
    this->loopBody->generateLLVM(context, loopScope);

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
