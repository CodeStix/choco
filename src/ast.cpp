#include "ast.hpp"

ASTBlock *parseBlock(TokenStream *tokens)
{
    int saved = tokens->getPosition();
    const Token *tok = tokens->peek();

    if (tok->type != TokenType::CURLY_BRACKET_OPEN)
    {
        std::cout << "ERROR: Expected { to open code block\n";
        tokens->setPosition(saved);
        return NULL;
    }

    tokens->next();

    std::list<ASTNode *> *statements = new std::list<ASTNode *>();
    while (!tokens->isEndOfFile())
    {
        tokens->consume(TokenType::WHITESPACE);
        tokens->consume(TokenType::NEWLINE);
        tok = tokens->peek();

        if (tok->type == TokenType::CURLY_BRACKET_CLOSE)
        {
            tokens->next();
            break;
        }

        ASTNode *statement = NULL;
        switch (tok->type)
        {
        case TokenType::STRUCT_KEYWORD:
            statement = parseStructDeclaration(tokens);
            break;
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
            statement = parseValueOrOperator(tokens);
            break;
        }

        if (statement == NULL)
        {
            std::cout << "ERROR: Invalid statement at '" << getTokenTypeName(tok->type) << "'\n";
            tokens->next();
        }
        else
        {
            statements->push_back(statement);
        }
    }

    return new ASTBlock(statements);
}

ASTParameter *parseParameter(TokenStream *tokens)
{
    int saved = tokens->getPosition();
    const Token *tok = tokens->peek();

    if (tok->type != TokenType::SYMBOL)
    {
        std::cout << "ERROR: Type must be a name\n";
        tokens->setPosition(saved);
        return NULL;
    }
    const Token *nameToken = tok;
    tokens->next();
    tokens->consume(TokenType::WHITESPACE);
    tok = tokens->peek();

    ASTNode *typeSpecifier;
    if (tok->type == TokenType::COLON)
    {
        // Parse type specifier
        tokens->next();
        tokens->consume(TokenType::WHITESPACE);
        typeSpecifier = parseInlineType(tokens);
        if (typeSpecifier == NULL)
        {
            std::cout << "ERROR: Could not parse type specifier\n";
            tokens->setPosition(saved);
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

ASTFunction *parseFunction(TokenStream *tokens)
{
    int saved = tokens->getPosition();

    const Token *tok = tokens->peek();
    const Token *exportToken = NULL;
    if (tok->type == TokenType::EXPORT_KEYWORD)
    {
        exportToken = tok;
        tokens->next();
        tokens->consume(TokenType::WHITESPACE);
        tok = tokens->peek();
    }
    const Token *externToken = NULL;
    if (tok->type == TokenType::EXTERN_KEYWORD)
    {
        externToken = tok;
        tokens->next();
        tokens->consume(TokenType::WHITESPACE);
        tok = tokens->peek();
    }
    if (tok->type != TokenType::FUNC_KEYWORD)
    {
        tokens->setPosition(saved);
        std::cout << "ERROR: Function must start with 'func'\n";
        return NULL;
    }
    tokens->next();
    tokens->consume(TokenType::WHITESPACE);

    tok = tokens->peek();
    const Token *nameToken = tok;
    if (nameToken->type != TokenType::SYMBOL)
    {
        std::cout << "ERROR: Function name must be symbol, not " << getTokenTypeName(nameToken->type) << "\n";
        return NULL;
    }

    tokens->next();
    tokens->consume(TokenType::WHITESPACE);

    tok = tokens->peek();
    if (tok->type != TokenType::BRACKET_OPEN)
    {
        std::cout << "ERROR: Function must have parameter list\n";
        return NULL;
    }

    tokens->next();
    tokens->consume(TokenType::WHITESPACE);

    std::vector<ASTParameter *> *parameters = new std::vector<ASTParameter *>();
    while (true)
    {
        tok = tokens->peek();
        if (tok->type == TokenType::BRACKET_CLOSE)
        {
            tokens->next();
            tokens->consume(TokenType::WHITESPACE);
            break;
        }

        ASTParameter *parameter = parseParameter(tokens);
        if (parameter == NULL)
        {
            std::cout << "ERROR: Could not parse function parameter\n";
            return NULL;
        }
        parameters->push_back(parameter);

        tok = tokens->peek();
        if (tok->type != TokenType::COMMA)
        {
            if (tok->type != TokenType::BRACKET_CLOSE)
            {
                std::cout << "ERROR: Invocation parameters should be split using commas (function definition)\n";
            }
        }
        else
        {
            tokens->next();
            tokens->consume(TokenType::WHITESPACE);
        }
    }

    tok = tokens->peek();

    ASTNode *returnType;
    if (tok->type == TokenType::COLON)
    {
        tokens->next();
        tokens->consume(TokenType::WHITESPACE);
        returnType = parseInlineType(tokens);
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
        tokens->consume(TokenType::WHITESPACE);
        tok = tokens->peek();

        if (tok->type == TokenType::CURLY_BRACKET_OPEN)
        {
            ASTBlock *body = parseBlock(tokens);
            return new ASTFunction(nameToken, parameters, returnType, body, exportToken != NULL);
        }
        else
        {
            std::cout << "ERROR: Function without body must be extern at " << (int)tok->position << "\n";
            return NULL;
        }
    }
    else
    {
        // This is an external function
        return new ASTFunction(nameToken, parameters, returnType, NULL, exportToken != NULL);
    }
}

ASTNode *parseIfStatement(TokenStream *tokens)
{
    int saved = tokens->getPosition();
    const Token *tok = tokens->peek();

    if (tok->type != TokenType::IF_KEYWORD)
    {
        tokens->setPosition(saved);
        return NULL;
    }

    tokens->next();
    tokens->consume(TokenType::WHITESPACE);
    tok = tokens->peek();

    if (tok->type != TokenType::BRACKET_OPEN)
    {
        tokens->setPosition(saved);
        std::cout << "ERROR: If statement must have an opening (\n";
        return NULL;
    }

    tokens->next();
    tokens->consume(TokenType::WHITESPACE);

    ASTNode *condition = parseValueOrOperator(tokens);
    if (condition == NULL)
    {
        return NULL;
    }

    tokens->consume(TokenType::WHITESPACE);
    tok = tokens->peek();

    if (tok->type != TokenType::BRACKET_CLOSE)
    {
        tokens->setPosition(saved);
        std::cout << "ERROR: If statement must have an closing (\n";
        return NULL;
    }

    tokens->next();
    tokens->consume(TokenType::WHITESPACE);
    tokens->consume(TokenType::NEWLINE);

    ASTBlock *thenBody = parseBlock(tokens);
    if (thenBody == NULL)
    {
        return NULL;
    }

    tokens->consume(TokenType::WHITESPACE);
    tokens->consume(TokenType::NEWLINE);

    ASTNode *elseBody = NULL;
    tok = tokens->peek();
    if (tok->type == TokenType::ELSE_KEYWORD)
    {
        tokens->next();
        tokens->consume(TokenType::WHITESPACE);
        tokens->consume(TokenType::NEWLINE);

        elseBody = parseBlock(tokens);
        if (elseBody == NULL)
        {
            return NULL;
        }
    }

    return new ASTIfStatement(condition, thenBody, elseBody);
}

ASTNode *parseWhileStatement(TokenStream *tokens)
{
    int saved = tokens->getPosition();
    const Token *tok = tokens->peek();

    if (tok->type != TokenType::WHILE_KEYWORD)
    {
        tokens->setPosition(saved);
        return NULL;
    }

    tokens->next();
    tokens->consume(TokenType::WHITESPACE);
    tok = tokens->peek();

    if (tok->type != TokenType::BRACKET_OPEN)
    {
        tokens->setPosition(saved);
        std::cout << "ERROR: While statement must have an opening (\n";
        return NULL;
    }

    tokens->next();
    tokens->consume(TokenType::WHITESPACE);

    ASTNode *condition = parseValueOrOperator(tokens);
    if (condition == NULL)
    {
        return NULL;
    }

    tokens->consume(TokenType::WHITESPACE);
    tok = tokens->peek();

    if (tok->type != TokenType::BRACKET_CLOSE)
    {
        tokens->setPosition(saved);
        std::cout << "ERROR: While statement must have an closing )\n";
        return NULL;
    }

    tokens->next();
    tokens->consume(TokenType::WHITESPACE);
    tokens->consume(TokenType::NEWLINE);

    ASTBlock *loopBody = parseBlock(tokens);
    if (loopBody == NULL)
    {
        return NULL;
    }

    tokens->consume(TokenType::WHITESPACE);
    tokens->consume(TokenType::NEWLINE);

    ASTNode *elseBody = NULL;
    tok = tokens->peek();
    if (tok->type == TokenType::ELSE_KEYWORD)
    {
        tokens->next();
        tokens->consume(TokenType::WHITESPACE);
        tokens->consume(TokenType::NEWLINE);

        elseBody = parseBlock(tokens);
        if (elseBody == NULL)
        {
            return NULL;
        }
    }

    return new ASTWhileStatement(condition, loopBody, elseBody);
}

ASTNode *parseUnaryOperator(TokenStream *tokens)
{
    int saved = tokens->getPosition();
    const Token *tok = tokens->peek();

    const Token *operandToken = tok;
    switch (operandToken->type)
    {
    case TokenType::OPERATOR_ADDITION:
    case TokenType::OPERATOR_SUBSTRACTION:
    case TokenType::OPERATOR_EXCLAMATION:
        break;
    default:
        tokens->setPosition(saved);
        return NULL;
    }

    tokens->next();

    ASTNode *operand = parseValueAndSuffix(tokens);
    if (operand == NULL)
    {
        // std::cout << "ERROR: Invalid unary operand at " << tok->position << " type " << getTokenTypeName(tok->type) << "\n";
        tokens->setPosition(saved);
        return NULL;
    }

    return new ASTUnaryOperator(operandToken, operand);
}

ASTNode *parseStructDeclaration(TokenStream *tokens)
{
    int saved = tokens->getPosition();

    const Token *tok = tokens->peek();
    if (tok->type != TokenType::STRUCT_KEYWORD)
    {
        tokens->setPosition(saved);
        return NULL;
    }
    tokens->next();

    tokens->consume(TokenType::WHITESPACE);

    tok = tokens->peek();
    if (tok->type != TokenType::SYMBOL)
    {
        std::cout << "ERROR: A struct declaration must have a name\n";
        return NULL;
    }
    const Token *nameToken = tok;
    tokens->next();

    tokens->consume(TokenType::WHITESPACE);
    tokens->consume(TokenType::NEWLINE);

    return parseStruct(tokens, nameToken);
}

ASTStruct *parseStruct(TokenStream *tokens, const Token *structNameToken)
{
    int saved = tokens->getPosition();
    const Token *tok = tokens->peek();

    bool managed = true, packed = false, value = false;
    bool readingModifiers = true;
    while (readingModifiers)
    {
        tok = tokens->peek();
        switch (tok->type)
        {
        case TokenType::VALUE_KEYWORD:
            value = true;
            tokens->next();
            tokens->consume(TokenType::WHITESPACE);
            break;
        case TokenType::PACKED_KEYWORD:
            packed = true;
            tokens->next();
            tokens->consume(TokenType::WHITESPACE);
            break;
        case TokenType::UNMANAGED_KEYWORD:
            managed = false;
            tokens->next();
            tokens->consume(TokenType::WHITESPACE);
            break;
        default:
            readingModifiers = false;
            break;
        }
    }

    if (tok->type != TokenType::CURLY_BRACKET_OPEN)
    {
        std::cout << "ERROR: Struct value must start with {\n";
        tokens->setPosition(saved);
        return NULL;
    }
    tokens->next();
    tokens->consume(TokenType::WHITESPACE);
    tokens->consume(TokenType::NEWLINE);

    std::vector<ASTStructField *> fields;
    while (1)
    {
        tok = tokens->peek();
        if (tok->type == TokenType::CURLY_BRACKET_CLOSE)
        {
            tokens->next();
            break;
        }

        if (tok->type != TokenType::SYMBOL)
        {
            tokens->next();
            std::cout << "ERROR: Struct value must contain fields at " << tok->position << " but got '" << tok->value << "'\n";
            return NULL;
        }
        const Token *fieldNameToken = tok;

        tokens->next();
        tokens->consume(TokenType::WHITESPACE);
        tok = tokens->peek();
        if (tok->type != TokenType::COLON)
        {
            tokens->next();
            std::cout << "ERROR: Struct value must have colon\n";
            return NULL;
        }
        tokens->next();
        tokens->consume(TokenType::WHITESPACE);

        ASTNode *fieldValue = parseValueOrOperator(tokens);
        fields.push_back(new ASTStructField(fieldNameToken, fieldValue));

        tok = tokens->peek();
        if (tok->type == TokenType::COMMA)
        {
            tokens->next();
        }
        tokens->consume(TokenType::WHITESPACE);
        tokens->consume(TokenType::NEWLINE);
    }

    return new ASTStruct(structNameToken, fields, managed, packed, value);
}

ASTNode *parseInlineType(TokenStream *tokens)
{
    int saved = tokens->getPosition();
    const Token *tok = tokens->peek();

    ASTNode *value;
    switch (tok->type)
    {
    case TokenType::PACKED_KEYWORD:
    case TokenType::UNMANAGED_KEYWORD:
    case TokenType::VALUE_KEYWORD:
    case TokenType::CURLY_BRACKET_OPEN:
    {
        value = parseStruct(tokens, NULL);
        break;
    }

    case TokenType::BRACKET_OPEN:
    {
        tokens->next();
        tokens->consume(TokenType::WHITESPACE);

        ASTNode *innerValue = parseInlineType(tokens);
        if (innerValue == NULL)
        {
            std::cout << "ERROR: Invalid type brackets content \n";
            return NULL;
        }

        tok = tokens->peek();
        if (tok->type != TokenType::BRACKET_CLOSE)
        {
            std::cout << "ERROR: Brackets must be closed\n";
        }
        else
        {
            tokens->next();
        }

        value = new ASTBrackets(innerValue);
        break;
    }

    case TokenType::SYMBOL:
    {
        tokens->next();
        value = new ASTSymbol(tok);
        break;
    }

    default:
        return NULL;
    }

    return value;
}

ASTNode *parseValueOrType(TokenStream *tokens)
{
    int saved = tokens->getPosition();
    const Token *tok = tokens->peek();

    ASTNode *value;
    switch (tok->type)
    {
    case TokenType::PACKED_KEYWORD:
    case TokenType::UNMANAGED_KEYWORD:
    case TokenType::VALUE_KEYWORD:
    case TokenType::CURLY_BRACKET_OPEN:
    {
        value = parseStruct(tokens, NULL);
        break;
    }

    case TokenType::LITERAL_STRING:
        value = new ASTLiteralString(tok);
        tokens->next();
        break;

    case TokenType::LITERAL_NUMBER:
        value = new ASTLiteralNumber(tok);
        tokens->next();
        break;

    case TokenType::OPERATOR_ADDITION:
    case TokenType::OPERATOR_SUBSTRACTION:
    case TokenType::OPERATOR_EXCLAMATION:
        value = parseUnaryOperator(tokens);
        break;

    case TokenType::BRACKET_OPEN:
    {
        tokens->next();
        tokens->consume(TokenType::WHITESPACE);

        ASTNode *innerValue = parseValueOrOperator(tokens);
        if (innerValue == NULL)
        {
            std::cout << "ERROR: Invalid brackets content at " << tok->position << " \n";
            return NULL;
        }

        tok = tokens->peek();
        if (tok->type != TokenType::BRACKET_CLOSE)
        {
            std::cout << "ERROR: Brackets must be closed\n";
        }
        else
        {
            tokens->next();
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
        tokens->next();
        value = new ASTSymbol(tok);
        break;
    }

    default:
        return NULL;
    }

    return value;
}

ASTNode *parseValueAndSuffix(TokenStream *tokens)
{
    int saved = tokens->getPosition();
    ASTNode *value = parseValueOrType(tokens);
    if (value == NULL)
    {
        return value;
    }

    while (1)
    {
        const Token *tok = tokens->peek();
        if (tok->type == TokenType::BRACKET_OPEN)
        {
            // Invocation
            tokens->next();
            tokens->consume(TokenType::WHITESPACE);
            tokens->consume(TokenType::NEWLINE);

            std::vector<ASTNode *> *parameterValues = new std::vector<ASTNode *>();
            while (true)
            {
                tok = tokens->peek();

                if (tok->type == TokenType::BRACKET_CLOSE)
                {
                    tokens->next();
                    break;
                }

                ASTNode *value = parseValueOrOperator(tokens);
                parameterValues->push_back(value);

                tok = tokens->peek();
                if (tok->type != TokenType::COMMA)
                {
                    if (tok->type != TokenType::BRACKET_CLOSE)
                    {
                        std::cout << "ERROR: Invocation parameters should be split using commas\n";
                    }
                }
                else
                {
                    tokens->next();
                    tokens->consume(TokenType::WHITESPACE);
                    tokens->consume(TokenType::NEWLINE);
                }
            }

            value = new ASTInvocation(value, parameterValues);
        }
        else if (tok->type == TokenType::PERIOD)
        {
            // Parse member dereference
            tokens->next();

            tok = tokens->peek();
            if (tok->type != TokenType::SYMBOL)
            {
                std::cout << "ERROR: only symbols can be used to dereference fields\n";
                return NULL;
            }

            value = new ASTMemberDereference(value, tok);
            tokens->next();
        }
        else if (tok->type == TokenType::SQUARE_BRACKET_OPEN)
        {
            // Parsse index dereference
            tokens->next();
            tokens->consume(TokenType::WHITESPACE);

            ASTNode *indexValue = parseValueOrOperator(tokens);
            if (indexValue == NULL)
            {
                return NULL;
            }

            tokens->consume(TokenType::WHITESPACE);
            tok = tokens->peek();
            if (tok->type != TokenType::SQUARE_BRACKET_CLOSE)
            {
                std::cout << "ERROR: index dereference must end with ]\n";
                return NULL;
            }

            value = new ASTIndexDereference(value, indexValue);
        }
        else if (tok->type == TokenType::OPERATOR_EXCLAMATION)
        {
            std::cout << "ERROR: ! suffix not implemented\n";
            return NULL;
        }
        else if (tok->type == TokenType::WHITESPACE)
        {
            tokens->next();
            ASTNode *castedValue = parseValueAndSuffix(tokens);
            if (castedValue != NULL)
            {
                value = new ASTCast(value, castedValue);
            }
        }
        else
        {
            break;
        }
    }

    return value;
}

ASTNode *parseValueOrOperator(TokenStream *tokens)
{
    int saved = tokens->getPosition();
    ASTNode *top = parseValueAndSuffix(tokens);
    if (top == NULL)
    {
        return NULL;
    }

    while (true)
    {
        const Token *tok = tokens->peek();
        if (tok == NULL)
        {
            return top;
        }

        if (tok->type == TokenType::OPERATOR_ASSIGNMENT)
        {
            tokens->next();
            tokens->consume(TokenType::WHITESPACE);

            ASTNode *right = parseValueOrOperator(tokens);
            if (right == NULL)
            {
                std::cout << "ERROR: could not parse assignment value\n";
                return NULL;
            }
            return new ASTAssignment(top, right);
        }

        int operatorImportance = getTokenOperatorImportance(tok->type);
        if (operatorImportance < 0)
        {
            // Isn't an operator, return
            return top;
        }

        tokens->next();
        tokens->consume(TokenType::WHITESPACE);

        ASTNode *right = parseValueAndSuffix(tokens);
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

ASTReturn *parseReturn(TokenStream *tokens)
{
    int saved = tokens->getPosition();
    const Token *tok = tokens->peek();

    if (tok->type != TokenType::RETURN_KEYWORD)
    {
        std::cout << "ERROR: Expected 'return'\n";
        return NULL;
    }
    tokens->next();
    tokens->consume(TokenType::WHITESPACE);

    ASTNode *value = parseValueOrOperator(tokens);
    return new ASTReturn(value);
}

ASTDeclaration *parseDeclaration(TokenStream *tokens)
{
    int saved = tokens->getPosition();
    const Token *tok = tokens->peek();

    if (tok->type != TokenType::CONST_KEYWORD && tok->type != TokenType::LET_KEYWORD)
    {
        std::cout << "ERROR: Declaration must start with 'const' or 'let'\n";
        return NULL;
    }
    tokens->next();
    tokens->consume(TokenType::WHITESPACE);

    tok = tokens->peek();

    const Token *nameToken = tok;
    if (nameToken->type != TokenType::SYMBOL)
    {
        std::cout << "ERROR: Constant name must be symbol, not " << getTokenTypeName(nameToken->type) << "\n";
        return NULL;
    }

    tokens->next();
    tokens->consume(TokenType::WHITESPACE);

    tok = tokens->peek();
    ASTNode *typeSpecifier;
    if (tok->type == TokenType::COLON)
    {
        tokens->next();
        tokens->consume(TokenType::WHITESPACE);

        typeSpecifier = parseInlineType(tokens);
    }
    else
    {
        typeSpecifier = NULL;
    }

    tok = tokens->peek();

    if (tok->type == TokenType::OPERATOR_ASSIGNMENT)
    {
        // Parse assignment
        tokens->next();
        tokens->consume(TokenType::WHITESPACE);

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

ASTFile *parseFile(TokenStream *tokens)
{
    int saved = tokens->getPosition();
    std::list<ASTNode *> *rootNodes = new std::list<ASTNode *>();

    while (!tokens->isEndOfFile())
    {
        tokens->consume(TokenType::WHITESPACE);
        tokens->consume(TokenType::NEWLINE);

        const Token *tok = tokens->peek();

        ASTNode *statement = NULL;
        switch (tok->type)
        {
        case TokenType::STRUCT_KEYWORD:
            statement = parseStructDeclaration(tokens);
            break;
        case TokenType::FUNC_KEYWORD:
        case TokenType::EXPORT_KEYWORD:
        case TokenType::EXTERN_KEYWORD:
            statement = parseFunction(tokens);
            break;
        case TokenType::CONST_KEYWORD:
        case TokenType::LET_KEYWORD:
            statement = parseDeclaration(tokens);
            break;
        }

        if (statement == NULL)
        {
            tokens->next();
            std::cout << "ERROR: Invalid statement, unexpected " << getTokenTypeName(tok->type) << "(" << (int)tok->type << ")"
                      << " at " << tok->position << "\n";
        }
        else if (statement->type == ASTNodeType::SYMBOL)
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

TypedValue *ASTSymbol::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint)
{
#ifdef DEBUG
    std::cout << "debug: ASTSymbol::generateLLVM " << this->nameToken->value << "\n";
#endif
    auto valuePointer = scope->getValue(this->nameToken->value);
    if (!valuePointer)
    {
        valuePointer = context->globalModule.getValueCascade(this->nameToken->value, context, scope);
        if (!valuePointer)
        {
            std::cout << "ERROR: Could not find '" << this->nameToken->value << "'\n";
            return NULL;
        }
    }
    valuePointer->setOriginVariable(this->nameToken->value);
    return valuePointer;
}

TypedValue *ASTLiteralNumber::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint)
{
#ifdef DEBUG
    std::cout << "debug: ASTLiteralNumber::generateLLVM\n";
#endif

    int integerBase = 10;
    std::string noPrefix = this->valueToken->value;
    if (this->valueToken->value.find("0x") == 0)
    {
        integerBase = 16;
        noPrefix = this->valueToken->value.substr(2);
    }
    else if (this->valueToken->value.find("0b") == 0)
    {
        integerBase = 2;
        noPrefix = this->valueToken->value.substr(2);
    }

    std::string cleaned = "";
    bool isFloating = false;
    for (char c : noPrefix)
    {
        if (c == '.')
        {
            if (isFloating)
            {
                std::cout << "ERROR: a number literal cannot have multiple periods\n";
                return NULL;
            }
            isFloating = true;
        }
        else if (c == '_')
        {
            continue;
        }
        cleaned += c;
    }

    if (isFloating)
    {
        FloatType *type;
        if (typeHint != NULL && typeHint->getTypeCode() == TypeCode::FLOAT)
        {
            type = static_cast<FloatType *>(typeHint);
        }
        else
        {
            type = new FloatType(64);
        }

        double floatingValue = strtod(cleaned.c_str(), NULL);
        auto value = llvm::ConstantFP::get(type->getLLVMType(*context->context), floatingValue);

        return new TypedValue(value, type);
    }
    else
    {
        unsigned long long integerValue = strtoull(cleaned.c_str(), NULL, integerBase);

        IntegerType *type;
        if (typeHint != NULL && typeHint->getTypeCode() == TypeCode::INTEGER)
        {
            type = static_cast<IntegerType *>(typeHint);
        }
        else
        {
            bool isSigned = integerValue <= INT64_MAX;
            type = new IntegerType(64, isSigned);
        }

        auto value = llvm::ConstantInt::get(type->getLLVMType(*context->context), integerValue, type->getSigned());

        return new TypedValue(value, type);
    }
}

TypedValue *ASTStruct::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint)
{
#ifdef DEBUG
    std::cout << "debug: ASTStruct::generateLLVM\n";
#endif

    std::map<std::string, TypedValue *> fieldValues;
    StructType *structType = NULL;
    bool isType = false;
    bool byValue = false;

    // Enforce type hint
    if (typeHint != NULL)
    {
        if (typeHint->getTypeCode() == TypeCode::POINTER)
        {
            PointerType *typeHintPointer = static_cast<PointerType *>(typeHint);
            if (typeHintPointer->getPointedType()->getTypeCode() == TypeCode::STRUCT)
            {
                structType = static_cast<StructType *>(typeHintPointer->getPointedType());
                byValue = typeHintPointer->isByValue();
            }
        }
        else if (typeHint->getTypeCode() == TypeCode::STRUCT)
        {
            structType = static_cast<StructType *>(typeHint);
            byValue = true;
        }
        else
        {
            std::cout << "ERROR: Unexpected struct, expected " << typeHint->toString() << "\n";
            return NULL;
        }

        if (this->value || !this->managed || this->packed)
        {
            std::cout << "ERROR: Struct modifier cannot be specified again\n";
            return NULL;
        }

        for (auto &field : this->fields)
        {
            auto hintField = structType->getField(field->getName());
            if (hintField == NULL)
            {
                std::cout << "ERROR: Struct field " << field->getName() << " does not exist on type " << typeHint->toString() << "\n";
                return NULL;
            }

            TypedValue *fieldValue = field->generateLLVM(context, scope, hintField->type);
            if (fieldValue->isType())
            {
                std::cout << "ERROR: Struct field cannot be initialized with a type\n";
                return NULL;
            }

            fieldValues[field->getName()] = fieldValue;
        }
    }

    // Infer struct type from specified value
    if (structType == NULL)
    {
        std::vector<StructTypeField> fieldTypes;
        bool first = true;
        for (auto &field : this->fields)
        {
            TypedValue *fieldValue = field->generateLLVM(context, scope, NULL);
            if (first)
            {
                first = false;
                isType = fieldValue->isType();
            }
            if (fieldValue->isType() != isType)
            {
                std::cout << "ERROR: cannot mix value and type structs\n";
                return NULL;
            }

            fieldValues[field->getName()] = fieldValue;

            Type *fieldType = NULL;
            if (fieldValue->getTypeCode() == TypeCode::POINTER)
            {
                PointerType *fieldPointerType = static_cast<PointerType *>(fieldValue->getType());
                if (fieldPointerType->isByValue())
                {
                    fieldType = fieldPointerType->getPointedType();
                }
            }
            if (fieldType == NULL)
            {
                fieldType = fieldValue->getType();
            }

            fieldTypes.push_back(StructTypeField(fieldType, field->getName()));
        }

        structType = new StructType(this->nameToken == NULL ? "" : this->nameToken->value, fieldTypes, this->managed, this->packed);
        byValue = this->value;
    }

    if (!isType)
    {
        // This is a struct value
        llvm::Type *llvmStructType = structType->getLLVMType(*context->context);

        auto structPointer = generateAllocaInCurrentFunction(context, llvmStructType, "allocstruct");

        for (auto &pair : fieldValues)
        {
            auto fieldName = pair.first;
            auto fieldValue = pair.second;
            auto fieldType = structType->getField(fieldName);
            auto fieldIndex = structType->getFieldIndex(fieldName);

            bool isVolatile = false;
            auto structFieldPointer = context->irBuilder->CreateStructGEP(llvmStructType, structPointer, fieldIndex, "structgep");
            TypedValue *structFieldPointerValue = new TypedValue(structFieldPointer, fieldType->type->getUnmanagedPointerToType(fieldType->type->getTypeCode() != TypeCode::POINTER));
            if (!generateAssignment(context, structFieldPointerValue, fieldValue, isVolatile))
            {
                std::cout << "ERROR: Cannot initialize struct field " << fieldName << " of " << structType->toString() << "\n";
                return NULL;
            }
        }

        // Check if all fields were set
        for (auto &field : structType->getFields())
        {
            if (fieldValues[field.name] == NULL)
            {
                std::cout << "ERROR: Struct field " << field.name << " of " << structType->toString() << " must be initialized\n";
                return NULL;
            }
        }

        return new TypedValue(structPointer, structType->getUnmanagedPointerToType(byValue));
    }
    else
    {
        // This struct is a type definition
        TypedValue *type = new TypedValue(NULL, structType->getUnmanagedPointerToType(byValue));

        if (this->nameToken != NULL)
        {
            if (!context->globalModule.addValue(this->nameToken->value, type))
            {
                std::cout << "ERROR: The struct '" << this->nameToken->value << "' has already been declared";
                return NULL;
            }
        }

        return type;
    }
}

TypedValue *ASTStructField::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint)
{
#ifdef DEBUG
    std::cout << "debug: ASTStructField::generateLLVM " << this->nameToken->value << "\n";
#endif
    return this->value->generateLLVM(context, scope, typeHint);
}

TypedValue *ASTUnaryOperator::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint)
{
#ifdef DEBUG
    std::cout << "debug: ASTUnaryOperator::generateLLVM\n";
#endif
    auto *operand = this->operand->generateLLVM(context, scope, NULL);

    switch (this->operatorToken->type)
    {
    case TokenType::OPERATOR_ADDITION:
        return operand;

    case TokenType::OPERATOR_SUBSTRACTION:
        // Negate
        if (operand->getTypeCode() == TypeCode::FLOAT)
        {
            llvm::Value *newValue = context->irBuilder->CreateFNeg(operand->getValue(), "fneg");
            return new TypedValue(newValue, operand->getType());
        }
        else if (operand->getTypeCode() == TypeCode::INTEGER)
        {
            IntegerType *intType = static_cast<IntegerType *>(operand->getType());
            if (!intType->getSigned())
            {
                std::cout << "ERROR: integer must be signed to be able to negate\n";
                return NULL;
            }

            llvm::Value *newValue = context->irBuilder->CreateNeg(operand->getValue(), "neg", false, false);
            return new TypedValue(newValue, operand->getType());
        }
        else
        {
            std::cout << "ERROR: Cannot use operator " << this->operatorToken->value << " on value\n";
            return NULL;
        }

    case TokenType::OPERATOR_EXCLAMATION:
        if (operand->getTypeCode() == TypeCode::INTEGER)
        {
            llvm::Value *newValue = context->irBuilder->CreateNot(operand->getValue(), "not");
            return new TypedValue(newValue, operand->getType());
        }
        else
        {
            std::cout << "ERROR: Cannot use operator " << this->operatorToken->value << " on value\n";
            return NULL;
        }

    default:
        std::cout << "ERROR: Unimplemented unary operator " << this->operatorToken->value << "\n";
        return NULL;
    }
}

TypedValue *ASTCast::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint)
{
#ifdef DEBUG
    std::cout << "debug: ASTCast::generateLLVM\n";
#endif

    auto targetType = this->targetType->generateLLVM(context, scope, NULL);
    if (targetType == NULL || !targetType->isType())
    {
        std::cout << "ERROR: Left-hand side of cast must be a type (got a value)\n";
        return NULL;
    }

    auto value = this->value->generateLLVM(context, scope, targetType->getType());
    if (value == NULL || value->isType())
    {
        std::cout << "ERROR: Right-hand side of cast must be a value (got a type)\n";
        return NULL;
    }

    return generateTypeConversion(context, value, targetType->getType(), true);
}

TypedValue *ASTOperator::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint)
{
    TokenType operatorType = this->operatorToken->type;

#ifdef DEBUG
    std::cout << "debug: ASTOperator::generateLLVM left\n";
#endif
    auto *left = this->left->generateLLVM(context, scope, NULL);
#ifdef DEBUG
    std::cout << "debug: ASTOperator::generateLLVM right\n";
#endif
    auto *right = this->right->generateLLVM(context, scope, NULL);

    if (!left || !right)
    {
        return NULL;
    }

    bool allowTypeJuggling;
    switch (operatorType)
    {
    case TokenType::OPERATOR_DOUBLE_AND:
    case TokenType::OPERATOR_DOUBLE_OR:
        allowTypeJuggling = false;
        break;
    default:
        allowTypeJuggling = true;
        break;
    }

    if (allowTypeJuggling)
    {
        if (!generateTypeJugging(context, &left, &right))
        {
            std::cout << "ERROR: Cannot " << this->operatorToken->value << " values, their types cannot be matched\n";
            return NULL;
        }
    }
    else
    {
        if (*left->getType() != *right->getType())
        {
            std::cout << "ERROR: Left and right operands must be the same type to perform " << this->operatorToken->value << "\n";
            return NULL;
        }
    }

    auto sharedType = left->getType(); // or right->getType()
    if (sharedType->getTypeCode() == TypeCode::POINTER)
    {
        left = generateDereferenceToValue(context, left);
        right = generateDereferenceToValue(context, right);
        sharedType = left->getType(); // or right->getType()
    }

    auto leftValue = left->getValue();
    auto rightValue = right->getValue();
    if (sharedType->getTypeCode() == TypeCode::FLOAT)
    {
        FloatType *sharedFloatType = static_cast<FloatType *>(sharedType);
        Type *resultingType = sharedType;
        llvm::Value *result;

        switch (operatorType)
        {
        case TokenType::OPERATOR_ADDITION:
            result = context->irBuilder->CreateFAdd(leftValue, rightValue, "opaddfp");
            break;
        case TokenType::OPERATOR_SUBSTRACTION:
            result = context->irBuilder->CreateFSub(leftValue, rightValue, "opsubfp");
            break;
        case TokenType::OPERATOR_MULTIPLICATION:
            result = context->irBuilder->CreateFMul(leftValue, rightValue, "opmulfp");
            break;
        case TokenType::OPERATOR_DIVISION:
            result = context->irBuilder->CreateFDiv(leftValue, rightValue, "opdivfp");
            break;
        case TokenType::OPERATOR_PERCENT:
            result = context->irBuilder->CreateFRem(leftValue, rightValue, "opmodfp");
            break;
        case TokenType::OPERATOR_LT:
            result = context->irBuilder->CreateFCmpULT(leftValue, rightValue, "opcmpltfp");
            resultingType = &BOOL_TYPE;
            break;
        case TokenType::OPERATOR_GT:
            result = context->irBuilder->CreateFCmpUGT(leftValue, rightValue, "opcmpgtfp");
            resultingType = &BOOL_TYPE;
            break;
        case TokenType::OPERATOR_LTE:
            result = context->irBuilder->CreateFCmpULE(leftValue, rightValue, "opcmplefp");
            resultingType = &BOOL_TYPE;
            break;
        case TokenType::OPERATOR_GTE:
            result = context->irBuilder->CreateFCmpUGE(leftValue, rightValue, "opcmpgefp");
            resultingType = &BOOL_TYPE;
            break;
        case TokenType::OPERATOR_EQUALS:
            result = context->irBuilder->CreateFCmpUEQ(leftValue, rightValue, "opcmpeqfp");
            resultingType = &BOOL_TYPE;
            break;
        case TokenType::OPERATOR_NOT_EQUALS:
            result = context->irBuilder->CreateFCmpUNE(leftValue, rightValue, "opcmpnefp");
            resultingType = &BOOL_TYPE;
            break;

        case TokenType::OPERATOR_DOUBLE_GT:
        case TokenType::OPERATOR_DOUBLE_LT:
        case TokenType::OPERATOR_AND:
        case TokenType::OPERATOR_OR:
        case TokenType::OPERATOR_DOUBLE_AND:
        case TokenType::OPERATOR_DOUBLE_OR:
        case TokenType::OPERATOR_CARET:
        default:
            std::cout << "ERROR: Invalid operator '" << this->operatorToken->value << "' on floats\n";
            return NULL;
        }

        return new TypedValue(result, resultingType);
    }
    else if (sharedType->getTypeCode() == TypeCode::INTEGER)
    {
        IntegerType *sharedIntType = static_cast<IntegerType *>(sharedType);
        Type *resultingType = sharedType;
        llvm::Value *result;

        switch (operatorType)
        {
        case TokenType::OPERATOR_ADDITION:
            result = context->irBuilder->CreateAdd(leftValue, rightValue, "addint");
            break;
        case TokenType::OPERATOR_SUBSTRACTION:
            result = context->irBuilder->CreateSub(leftValue, rightValue, "opsubint");
            break;
        case TokenType::OPERATOR_MULTIPLICATION:
            result = context->irBuilder->CreateMul(leftValue, rightValue, "opmulint");
            break;
        case TokenType::OPERATOR_DIVISION:
            if (sharedIntType->getSigned())
            {
                result = context->irBuilder->CreateSDiv(leftValue, rightValue, "opdivint");
            }
            else
            {
                result = context->irBuilder->CreateUDiv(leftValue, rightValue, "opdivint");
            }
            break;
        case TokenType::OPERATOR_PERCENT:
            if (sharedIntType->getSigned())
            {
                result = context->irBuilder->CreateSRem(leftValue, rightValue, "opmodint");
            }
            else
            {
                result = context->irBuilder->CreateURem(leftValue, rightValue, "opmodint");
            }
            break;
        case TokenType::OPERATOR_LT:
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
        case TokenType::OPERATOR_GT:
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
        case TokenType::OPERATOR_LTE:
            if (sharedIntType->getSigned())
            {
                result = context->irBuilder->CreateICmpSLE(leftValue, rightValue, "opcmpleint");
            }
            else
            {
                result = context->irBuilder->CreateICmpULE(leftValue, rightValue, "opcmpleint");
            }
            resultingType = &BOOL_TYPE;
            break;
        case TokenType::OPERATOR_GTE:
            if (sharedIntType->getSigned())
            {
                result = context->irBuilder->CreateICmpSGE(leftValue, rightValue, "opcmpgeint");
            }
            else
            {
                result = context->irBuilder->CreateICmpUGE(leftValue, rightValue, "opcmpgeint");
            }
            resultingType = &BOOL_TYPE;
            break;
        case TokenType::OPERATOR_EQUALS:
            result = context->irBuilder->CreateICmpEQ(leftValue, rightValue, "opcmpeqint");
            resultingType = &BOOL_TYPE;
            break;
        case TokenType::OPERATOR_NOT_EQUALS:
            result = context->irBuilder->CreateICmpNE(leftValue, rightValue, "opcmpneint");
            resultingType = &BOOL_TYPE;
            break;
        case TokenType::OPERATOR_DOUBLE_GT:
            // TODO: ashr instruction
            result = context->irBuilder->CreateLShr(leftValue, rightValue, "oplshrint");
            break;
        case TokenType::OPERATOR_DOUBLE_LT:
            result = context->irBuilder->CreateShl(leftValue, rightValue, "opshlint");
            break;
        case TokenType::OPERATOR_DOUBLE_AND:
        {
            IntegerType *leftIntegerType = static_cast<IntegerType *>(left->getType());
            IntegerType *rightIntegerType = static_cast<IntegerType *>(right->getType());
            if (leftIntegerType->getBitSize() != 1 || rightIntegerType->getBitSize() != 1)
            {
                std::cout << "ERROR: Logical and operator can only be used on booleans\n";
                return NULL;
            }
            result = context->irBuilder->CreateAnd(leftValue, rightValue, "opandint");
            break;
        }
        case TokenType::OPERATOR_DOUBLE_OR:
        {
            IntegerType *leftIntegerType = static_cast<IntegerType *>(left->getType());
            IntegerType *rightIntegerType = static_cast<IntegerType *>(right->getType());
            if (leftIntegerType->getBitSize() != 1 || rightIntegerType->getBitSize() != 1)
            {
                std::cout << "ERROR: Logical or operator can only be used on booleans\n";
                return NULL;
            }
            result = context->irBuilder->CreateOr(leftValue, rightValue, "oporint");
            break;
        }
        case TokenType::OPERATOR_AND:
            result = context->irBuilder->CreateAnd(leftValue, rightValue, "opandint");
            break;
        case TokenType::OPERATOR_OR:
            result = context->irBuilder->CreateOr(leftValue, rightValue, "oporint");
            break;
        case TokenType::OPERATOR_CARET:
            result = context->irBuilder->CreateXor(leftValue, rightValue, "opxorint");
            break;

        default:
            std::cout << "ERROR: Invalid operator '" << this->operatorToken->value << "' on integers\n";
            return NULL;
        }

        return new TypedValue(result, resultingType);
    }
    else
    {
        std::cout << "ERROR: Cannot " << this->operatorToken->value << " values, the type does not support this operator\n";
        return NULL;
    }
}

TypedValue *ASTLiteralString::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint)
{
#ifdef DEBUG
    std::cout << "debug: ASTLiteralString::generateLLVM\n";
#endif
    auto value = context->irBuilder->CreateGlobalString(this->valueToken->value, "str");
    Type *type = new ArrayType(&CHAR_TYPE, this->valueToken->value.length() + 1);
    return new TypedValue(value, type);
}

TypedValue *ASTDeclaration::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint)
{
#ifdef DEBUG
    std::cout << "debug: ASTDeclaration::generateLLVM\n";
#endif

    if (scope->hasValue(this->nameToken->value))
    {
        std::cout << "ERROR: Cannot redeclare '" << this->nameToken->value << "', it has already been declared\n";
        return NULL;
    }

    Type *specifiedType;
    if (this->typeSpecifier != NULL)
    {
        TypedValue *specifiedTypeValue = this->typeSpecifier->generateLLVM(context, scope, NULL);
        if (!specifiedTypeValue->isType())
        {
            std::cout << "ERROR: Declaration type specifier may not have value\n";
            return NULL;
        }
        specifiedType = specifiedTypeValue->getType();
    }
    else
    {
        specifiedType = NULL;
    }

    TypedValue *initialValue;
    if (this->value != NULL)
    {
        initialValue = this->value->generateLLVM(context, scope, specifiedType);
    }
    else
    {
        initialValue = NULL;
        // TODO: remove this error when unimplemented variables have been implemented
        std::cout << "ERROR: Declaration must have initial value (due to uninitialized variables not being implemented)\n";
        return NULL;
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
            std::cout << "ERROR: Declaration type must be specified when value is missing\n";
            return NULL;
        }
    }

    TypedValue *pointer = NULL;
    if (pointerType->getTypeCode() == TypeCode::POINTER)
    {
        PointerType *p = static_cast<PointerType *>(pointerType);
        if (p->isByValue())
        {
            llvm::Value *pointerValue = generateAllocaInCurrentFunction(context, p->getPointedType()->getLLVMType(*context->context), "allocavalue");
            pointer = new TypedValue(pointerValue, p->getPointedType()->getUnmanagedPointerToType(true));
        }
    }

    if (pointer == NULL)
    {
        llvm::Value *pointerValue = generateAllocaInCurrentFunction(context, pointerType->getLLVMType(*context->context), "alloca");
        pointer = new TypedValue(pointerValue, pointerType->getUnmanagedPointerToType(false));
    }

    scope->addValue(this->nameToken->value, pointer);

    bool isVolatile = false;
    if (!generateAssignment(context, pointer, initialValue, isVolatile))
    {
        std::cout << "ERROR: Cannot generate declaration at " << this->nameToken->position << "\n";
        return NULL;
    }

    return initialValue;
}

TypedValue *ASTAssignment::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint)
{
#ifdef DEBUG
    std::cout << "debug: ASTAssignment::generateLLVM\n";
#endif

    bool isVolatile = false;

    TypedValue *valuePointer = this->pointerValue->generateLLVM(context, scope, NULL);
    if (valuePointer == NULL)
    {
        std::cout << "ERROR: Cannot set variable '" << valuePointer->getOriginVariable() << "', it is not found\n";
        return NULL;
    }
    if (valuePointer->getTypeCode() != TypeCode::POINTER)
    {
        std::cout << "ERROR: Assert failed: valuePointer->getTypeCode() != TypeCode::POINTER\n";
        return NULL;
    }
    PointerType *valuePointerType = static_cast<PointerType *>(valuePointer->getType());

    // TODO: this code will segfault when the declaration hasn't specified a value (previous value is uninitialized)
    if (valuePointerType->getPointedType()->getTypeCode() == TypeCode::POINTER)
    {
        // If a previous pointer value will be overwritten, the reference count must be decremented if was a managed pointer
        PointerType *storedPointerType = static_cast<PointerType *>(valuePointerType->getPointedType());
        if (storedPointerType->isManaged())
        {
            llvm::Value *storedManagedPointer = context->irBuilder->CreateLoad(storedPointerType->getLLVMType(*context->context), valuePointer->getValue(), "deref");
            if (!generateDecrementReference(context, new TypedValue(storedManagedPointer, storedPointerType)))
            {
                std::cout << "ERROR: Could not generate decrement managed pointer code for assignment\n";
                return NULL;
            }
        }
    }

    TypedValue *newValue = this->value->generateLLVM(context, scope, valuePointerType->getPointedType());

    if (!generateAssignment(context, valuePointer, newValue, isVolatile))
    {
        std::cout << "ERROR: Cannot generate assignment\n";
        return NULL;
    }

    return newValue;
}

TypedValue *ASTReturn::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint)
{
#ifdef DEBUG
    std::cout << "debug: ASTReturn::generateLLVM\n";
#endif
    if (this->value != NULL)
    {
        Type *returnType = context->currentFunction->getReturnType();
        if (returnType == NULL)
        {
            std::cout << "ERROR: Function does not return value\n";
            return NULL;
        }

        auto value = this->value->generateLLVM(context, scope, returnType);

        TypedValue *newValue = generateTypeConversion(context, value, returnType, true); // TODO: remove allowLosePrecision when casts are supported
        if (newValue == NULL)
        {
            std::cout << "ERROR: Cannot convert return value in function\n";
            return NULL;
        }

        if (returnType->getTypeCode() == TypeCode::POINTER)
        {
            // Create sret
            if (!generateAssignment(context, context->currentFunctionSRet, value, false))
            {
                std::cout << "ERROR: Could not assign return value (sret)\n";
                return NULL;
            }

            context->irBuilder->CreateRetVoid();
        }
        else
        {
            context->irBuilder->CreateRet(newValue->getValue());
        }

        return value;
    }
    else
    {
        context->irBuilder->CreateRetVoid();
        return NULL;
    }
}

TypedValue *ASTBlock::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint)
{
#ifdef DEBUG
    std::cout << "debug: ASTBlock::generateLLVM\n";
#endif
    TypedValue *lastValue = NULL;
    for (ASTNode *statement : *this->statements)
    {
#ifdef DEBUG
        std::cout << "debug: ASTBlock::generateLLVM generate " << astNodeTypeToString(statement->type) << "\n";
#endif
        lastValue = statement->generateLLVM(context, scope, NULL);
    }
    return lastValue;
}

TypedValue *ASTParameter::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint)
{
#ifdef DEBUG
    std::cout << "debug: ASTParameter::generateLLVM\n";
#endif
    return this->typeSpecifier->generateLLVM(context, scope, typeHint);
}

TypedValue *ASTFunction::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint)
{
#ifdef DEBUG
    std::cout << "debug: ASTFunction::generateLLVM " << this->nameToken->value << "\n";
#endif

    std::vector<FunctionParameter> parameters;
    for (ASTParameter *parameter : *this->parameters)
    {
        TypedValue *parameterTypeValue = parameter->generateLLVM(context, scope, NULL);
        if (!parameterTypeValue->isType())
        {
            std::cout << "ERROR: parameter type specifier may not have value\n";
            return NULL;
        }
        parameters.push_back(FunctionParameter(parameterTypeValue->getType(), parameter->getParameterName()));
    }

    Type *returnType;
    if (this->returnType != NULL)
    {
        TypedValue *returnTypeValue = this->returnType->generateLLVM(context, scope, NULL);
        if (!returnTypeValue->isType())
        {
            std::cout << "ERROR: return type specifier may not have value\n";
            return NULL;
        }
        returnType = returnTypeValue->getType();
    }
    else
    {
        returnType = NULL;
    }

    FunctionType *newFunctionType = new FunctionType(returnType, parameters);

    bool isVarArg = false;
    llvm::GlobalValue::LinkageTypes linkage = this->exported ? llvm::Function::ExternalLinkage : llvm::Function::PrivateLinkage;
    llvm::FunctionType *functionType = static_cast<llvm::FunctionType *>(newFunctionType->getLLVMType(*context->context));
    llvm::Function *function = llvm::Function::Create(functionType, linkage, this->nameToken->value, *context->module);
    if (function == NULL)
    {
        std::cout << "ERROR: Function::Create returned null\n";
        return NULL;
    }

    // Add function attributes
    auto fnAttributeBuilder = llvm::AttrBuilder(*context->context);
    fnAttributeBuilder.addAttribute(llvm::Attribute::NoUnwind);
    if (this->exported)
    {
        // TODO: do this only if targetting WASM
        fnAttributeBuilder.addAttribute("wasm-export-name", this->nameToken->value);
    }
    function->addFnAttrs(fnAttributeBuilder);

    // When the return type is a pointer, the return value is stored in the first argument passed to the function
    bool useSRet = returnType != NULL && returnType->getTypeCode() == TypeCode::POINTER;
    if (useSRet)
    {
        PointerType *returnPointerType = static_cast<PointerType *>(returnType);
        auto returnAttributebuilder = llvm::AttrBuilder(*context->context);
        returnAttributebuilder.addStructRetAttr(returnPointerType->getPointedType()->getLLVMType(*context->context));
        returnAttributebuilder.addAttribute(llvm::Attribute::NoAlias);
        returnAttributebuilder.addAttribute(llvm::Attribute::WriteOnly);
        returnAttributebuilder.addAttribute(llvm::Attribute::NoCapture);
        // The sret argument is the first one (0)
        function->addParamAttrs(0, returnAttributebuilder);
    }

    // Name parameters and add parameter attributes when needed
    for (int i = 0; i < parameters.size(); i++)
    {
        auto parameter = parameters[i];
        auto parameterValue = function->getArg(useSRet ? i + 1 : i);
        parameterValue->setName(parameter.name);

        if (parameter.type->getTypeCode() == TypeCode::POINTER)
        {
            PointerType *parameterPointerType = static_cast<PointerType *>(parameter.type);
            if (parameterPointerType->isByValue())
            {
                // Add ByVal attribute to pointers that should be passed by value
                auto attributeBuilder = llvm::AttrBuilder(*context->context);
                attributeBuilder.addByValAttr(parameterPointerType->getPointedType()->getLLVMType(*context->context));
                parameterValue->addAttrs(attributeBuilder);
            }
        }
    }

    TypedValue *newFunctionPointerType = new TypedValue(function, newFunctionType->getUnmanagedPointerToType(false));

    if (!context->globalModule.addValue(this->nameToken->value, newFunctionPointerType))
    {
        std::cout << "ERROR: The function '" << this->nameToken->value << "' has already been declared";
        return NULL;
    }

    if (this->body != NULL)
    {
        llvm::Function *function = static_cast<llvm::Function *>(newFunctionPointerType->getValue());
        if (!function->empty())
        {
            std::cout << "ERROR: Cannot implement the '" << this->nameToken->value << "' function a second time\n";
            return NULL;
        }

        PointerType *functionPointerType = static_cast<PointerType *>(newFunctionPointerType->getType());
        context->currentFunction = static_cast<FunctionType *>(functionPointerType->getPointedType());

        FunctionScope *functionScope = new FunctionScope(*scope);
        llvm::BasicBlock *functionStartBlock = llvm::BasicBlock::Create(*context->context, "block", function);
        context->irBuilder->SetInsertPoint(functionStartBlock);

        for (int i = 0; i < this->parameters->size(); i++)
        {
            ASTParameter *parameter = (*this->parameters)[i];
            auto parameterValue = function->getArg(useSRet ? i + 1 : 0);

            TypedValue *parameterTypeValue = parameter->generateLLVM(context, scope, NULL);
            if (!parameterTypeValue->isType())
            {
                std::cout << "ERROR: parameter type specifier may not have value\n";
                return NULL;
            }
            Type *parameterType = parameterTypeValue->getType();

            auto parameterPointer = context->irBuilder->CreateAlloca(parameterType->getLLVMType(*context->context), NULL, "loadarg");
            context->irBuilder->CreateStore(parameterValue, parameterPointer, false);
            functionScope->addValue(parameter->getParameterName(), new TypedValue(parameterPointer, parameterType->getUnmanagedPointerToType(false)));
        }

        if (useSRet)
        {
            // Use the sret parameter (first parameter)
            auto sretParameter = function->getArg(0);
            sretParameter->setName("sret");
            context->currentFunctionSRet = new TypedValue(sretParameter, returnType);
        }
        else
        {
            context->currentFunctionSRet = NULL;
        }

        this->body->generateLLVM(context, functionScope, NULL);

        // Check if the function was propery terminated
        // llvm::BasicBlock *functionEndBlock = context->irBuilder->GetInsertBlock();
        if (!this->body->isTerminating())
        {
            if (this->returnType == NULL)
            {
                context->irBuilder->CreateRetVoid();
            }
            else
            {
                std::cout << "ERROR: Function '" << this->nameToken->value << "' must return a value\n";
                return NULL;
            }
        }

        // Check generated IR for issues
        if (llvm::verifyFunction(*function, &llvm::errs()))
        {
            std::cout << "ERROR: LLVM reported invalid function\n";
            return NULL;
        }

        // Optimize the function code
        context->passManager->run(*function);
    }

    return newFunctionPointerType;
}

TypedValue *ASTWhileStatement::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint)
{
#ifdef DEBUG
    std::cout << "debug: ASTWhileStatement::generateLLVM\n";
#endif
    TypedValue *preConditionValue = this->condition->generateLLVM(context, scope, &BOOL_TYPE);
    if (*preConditionValue->getType() != BOOL_TYPE)
    {
        std::cout << "While condition must be a bool (UInt1) type!\n";
        return NULL;
    }

    llvm::BasicBlock *originalBlock = context->irBuilder->GetInsertBlock();
    llvm::Function *parentFunction = originalBlock->getParent();
    llvm::BasicBlock *elseStartBlock = llvm::BasicBlock::Create(*context->context, "whileelse");
    llvm::BasicBlock *loopStartBlock = llvm::BasicBlock::Create(*context->context, "whilebody");
    llvm::BasicBlock *continueBlock = llvm::BasicBlock::Create(*context->context, "whilecont");

    // llvm::Value *preConditionResult = context->irBuilder->CreateFCmpONE(preConditionValue, llvm::ConstantFP::get(*context->context, llvm::APFloat(0.0)), "whileprecond");
    if (originalBlock->getTerminator() == NULL)
    {
        context->irBuilder->CreateCondBr(preConditionValue->getValue(), loopStartBlock, elseStartBlock);
    }

    context->irBuilder->SetInsertPoint(loopStartBlock);
    loopStartBlock->insertInto(parentFunction);
    auto loopScope = new FunctionScope(*scope);
    this->loopBody->generateLLVM(context, loopScope, NULL);

    TypedValue *conditionValue = this->condition->generateLLVM(context, loopScope, &BOOL_TYPE);
    if (*conditionValue->getType() != BOOL_TYPE)
    {
        std::cout << "While condition must be a bool (UInt1) type!\n";
        return NULL;
    }

    // llvm::Value *conditionResult = context->irBuilder->CreateFCmpONE(conditionValue->getValue(), llvm::ConstantFP::get(*context->context, llvm::APFloat(0.0)), "whilecond");
    llvm::BasicBlock *loopEndBlock = context->irBuilder->GetInsertBlock(); // Current block could have changed in generateLLVM calls above, update it here
    if (loopEndBlock->getTerminator() == NULL)
    {
        context->irBuilder->CreateCondBr(conditionValue->getValue(), loopStartBlock, continueBlock);
    }

    context->irBuilder->SetInsertPoint(elseStartBlock);
    elseStartBlock->insertInto(parentFunction);
    auto elseScope = new FunctionScope(*scope);
    if (this->elseBody != NULL)
    {
        this->elseBody->generateLLVM(context, elseScope, NULL);
    }
    llvm::BasicBlock *elseEndBlock = context->irBuilder->GetInsertBlock(); // Current block could have changed in generateLLVM calls above, update it here
    if (elseEndBlock->getTerminator() == NULL)
    {
        context->irBuilder->CreateBr(continueBlock);
    }

    context->irBuilder->SetInsertPoint(continueBlock);
    if (continueBlock->hasNPredecessorsOrMore(1))
    {
        continueBlock->insertInto(parentFunction);
    }
    return NULL;
}

TypedValue *ASTIndexDereference::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint)
{
#ifdef DEBUG
    std::cout << "debug: ASTIndexDereference::generateLLVM\n";
#endif

    TypedValue *valueToIndex = this->toIndex->generateLLVM(context, scope, NULL);
    TypedValue *indexValue = this->index->generateLLVM(context, scope, &UINT32_TYPE);
    std::cout << "ERROR: Index dereference not implemented\n";
    return NULL;
}

TypedValue *ASTMemberDereference::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint)
{
#ifdef DEBUG
    std::cout << "debug: ASTMemberDereference::generateLLVM\n";
#endif

    TypedValue *valueToIndex = this->toIndex->generateLLVM(context, scope, NULL);

    if (valueToIndex->isType())
    {
        if (valueToIndex->getType()->getTypeCode() == TypeCode::MODULE)
        {
            ModuleType *mod = static_cast<ModuleType *>(valueToIndex->getType());
            TypedValue *moduleValue = mod->getValue(this->nameToken->value, context, scope);
            if (moduleValue == NULL)
            {
                std::cout << "ERROR: '" << this->nameToken->value << "' cannot be found in module '" << mod->getFullName() << "'\n";
                return NULL;
            }
            return moduleValue;
        }
        else
        {
            std::cout << "ERROR: Type has no members to dereference\n";
            return NULL;
        }
    }

    TypedValue *pointerToIndex = generateDereferenceToPointer(context, valueToIndex);
    if (pointerToIndex == NULL)
    {
        std::cout << "ERROR: Member dereference only supports pointers\n";
        return NULL;
    }

    PointerType *pointerTypeToIndex = static_cast<PointerType *>(pointerToIndex->getType());

    if (pointerTypeToIndex->getPointedType()->getTypeCode() == TypeCode::STRUCT)
    {
        StructType *structType = static_cast<StructType *>(pointerTypeToIndex->getPointedType());
        int fieldIndex = structType->getFieldIndex(this->nameToken->value);
        if (fieldIndex < 0)
        {
            std::cout << "ERROR: cannot access member '" << this->nameToken->value << "' of struct\n";
            return NULL;
        }

        StructTypeField *structField = structType->getField(this->nameToken->value);

        // A struct is indexed
        std::vector<llvm::Value *> indices;
        indices.push_back(llvm::Constant::getIntegerValue(llvm::IntegerType::getInt32Ty(*context->context), llvm::APInt(32, 0, false)));
        indices.push_back(llvm::Constant::getIntegerValue(llvm::IntegerType::getInt32Ty(*context->context), llvm::APInt(32, fieldIndex, false)));

        llvm::Value *fieldPointer = context->irBuilder->CreateGEP(pointerTypeToIndex->getPointedType()->getLLVMType(*context->context), pointerToIndex->getValue(), indices, "memberstruct");
        return new TypedValue(fieldPointer, structField->type->getUnmanagedPointerToType(structField->type->getTypeCode() != TypeCode::POINTER));
    }
    else
    {
        std::cout << "ERROR: member dereference only supports structs\n";
        return NULL;
    }
}

TypedValue *ASTIfStatement::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint)
{
#ifdef DEBUG
    std::cout << "debug: ASTIfStatement::generateLLVM\n";
#endif

    TypedValue *condition = this->condition->generateLLVM(context, scope, &BOOL_TYPE);
    // llvm::Value *condition = context->irBuilder->CreateFCmpONE(conditionFloat, llvm::ConstantFP::get(*context->context, llvm::APFloat(0.0)), "ifcond");
    if (*condition->getType() != BOOL_TYPE)
    {
        std::cout << "While condition must be a bool (UInt1) type!\n";
        return NULL;
    }

    llvm::Function *parentFunction = context->irBuilder->GetInsertBlock()->getParent();
    llvm::BasicBlock *thenStartBlock = llvm::BasicBlock::Create(*context->context, "ifthen");
    llvm::BasicBlock *elseStartBlock = llvm::BasicBlock::Create(*context->context, "ifelse");
    llvm::BasicBlock *continueBlock = llvm::BasicBlock::Create(*context->context, "ifcont");

    context->irBuilder->CreateCondBr(condition->getValue(), thenStartBlock, elseStartBlock);

    context->irBuilder->SetInsertPoint(thenStartBlock);
    thenStartBlock->insertInto(parentFunction);
    auto thenScope = new FunctionScope(*scope);
    this->thenBody->generateLLVM(context, thenScope, NULL);

    llvm::BasicBlock *thenEndBlock = context->irBuilder->GetInsertBlock(); // Current block could have changed in generateLLVM calls above, update it here
    if (thenEndBlock->getTerminator() == NULL)
    {
        context->irBuilder->CreateBr(continueBlock);
    }

    context->irBuilder->SetInsertPoint(elseStartBlock);
    elseStartBlock->insertInto(parentFunction);
    auto elseScope = new FunctionScope(*scope);
    if (this->elseBody != NULL)
    {
        this->elseBody->generateLLVM(context, elseScope, NULL);
    }

    llvm::BasicBlock *elseEndBlock = context->irBuilder->GetInsertBlock(); // Current block could have changed in generateLLVM calls above, update it here
    if (elseEndBlock->getTerminator() == NULL)
    {
        context->irBuilder->CreateBr(continueBlock);
    }

    context->irBuilder->SetInsertPoint(continueBlock);
    if (continueBlock->hasNPredecessorsOrMore(1))
    {
        continueBlock->insertInto(parentFunction);
    }

    return NULL;
}

TypedValue *ASTInvocation::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint)
{
#ifdef DEBUG
    std::cout << "debug: ASTInvocation::generateLLVM\n";
#endif
    TypedValue *functionValue = this->functionPointerValue->generateLLVM(context, scope, NULL);
    if (functionValue == NULL)
    {
        std::cout << "ERROR: Function to call not found\n";
        return NULL;
    }

    if (functionValue->getTypeCode() != TypeCode::POINTER)
    {
        std::cout << "ERROR: Cannot invoke '" << functionValue->getOriginVariable() << "', it must be a pointer\n";
        return NULL;
    }
    PointerType *functionPointerType = static_cast<PointerType *>(functionValue->getType());
    if (functionPointerType->getPointedType()->getTypeCode() != TypeCode::FUNCTION)
    {
        std::cout << "ERROR: Cannot invoke '" << functionValue->getOriginVariable() << "', it must be a function pointer\n";
        return NULL;
    }

    llvm::Function *function = static_cast<llvm::Function *>(functionValue->getValue());
    FunctionType *functionType = static_cast<FunctionType *>(functionPointerType->getPointedType());

    std::vector<FunctionParameter> &parameters = functionType->getParameters();
    int parameterCount = this->parameterValues->size();
    int actualParameterCount = parameters.size();
    if (actualParameterCount != parameterCount)
    {
        std::cout << "ERROR: Invalid amount of parameters for function '" << functionValue->getOriginVariable() << "' invocation, expected " << actualParameterCount << ", got " << parameterCount << "\n";
        return NULL;
    }

    std::vector<llvm::Value *> parameterValues;

    // Check if return value will be passed in sret parameter (is always the first parameter)
    llvm::Value *sretPointer = NULL;
    if (functionType->getReturnType() != NULL && functionType->getReturnType()->getTypeCode() == TypeCode::POINTER)
    {
        PointerType *returnPointerType = static_cast<PointerType *>(functionType->getReturnType());
        sretPointer = generateAllocaInCurrentFunction(context, returnPointerType->getPointedType()->getLLVMType(*context->context), "allocasret");
        parameterValues.push_back(sretPointer);
    }

    for (int p = 0; p < actualParameterCount; p++)
    {
        FunctionParameter &parameter = parameters[p];
        ASTNode *parameterNode = (*this->parameterValues)[p];
        TypedValue *parameterValue = parameterNode->generateLLVM(context, scope, parameter.type);
        if (parameterValue == NULL || parameterValue->getValue() == NULL)
        {
            return NULL;
        }

        TypedValue *convertedValue = generateTypeConversion(context, parameterValue, parameter.type, false);
        if (convertedValue == NULL)
        {
            std::cout << "ERROR: Cannot convert value '" << parameter.name << "' for invoke '" << functionValue->getOriginVariable() << "'\n";
            return NULL;
        }
        if (convertedValue->isType())
        {
            std::cout << "ERROR: Types cannot be passed as parameters\n";
            return NULL;
        }
        parameterValues.push_back(convertedValue->getValue());
    }

    llvm::Value *callResult;
    if (sretPointer != NULL)
    {
        // Function does not return an llvm::Value because sret is used, don't name the result (twine it)
        context->irBuilder->CreateCall(function, parameterValues);
        callResult = sretPointer;
    }
    else
    {
        callResult = context->irBuilder->CreateCall(function, parameterValues, functionType->getReturnType() == NULL ? "" : "call");
    }

    return new TypedValue(callResult, functionType->getReturnType());
}

TypedValue *ASTBrackets::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint)
{
#ifdef DEBUG
    std::cout << "debug: ASTBrackets::generateLLVM\n";
#endif
    return this->inner->generateLLVM(context, scope, typeHint);
}

TypedValue *ASTFile::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint)
{
#ifdef DEBUG
    std::cout << "debug: ASTFile::generateLLVM\n";
#endif
    FunctionScope *fileScope = new FunctionScope(*scope);
    for (ASTNode *statement : *this->statements)
    {
        statement->generateLLVM(context, fileScope, NULL);
    }
    return NULL;
}

std::string astNodeTypeToString(ASTNodeType type)
{
    switch (type)
    {
    case ASTNodeType::OPERATOR:
        return "OPERATOR";
    case ASTNodeType::UNARY_OPERATOR:
        return "UNARY_OPERATOR";
    case ASTNodeType::LITERAL_NUMBER:
        return "LITERAL_NUMBER";
    case ASTNodeType::LITERAL_STRING:
        return "LITERAL_STRING";
    case ASTNodeType::FUNCTION:
        return "FUNCTION";
    case ASTNodeType::DECLARATION:
        return "DECLARATION";
    case ASTNodeType::ASSIGNMENT:
        return "ASSIGNMENT";
    case ASTNodeType::INVOCATION:
        return "INVOCATION";
    case ASTNodeType::SYMBOL:
        return "SYMBOL";
    case ASTNodeType::BRACKETS:
        return "BRACKETS";
    case ASTNodeType::FILE:
        return "FILE";
    case ASTNodeType::RETURN:
        return "RETURN";
    case ASTNodeType::IF:
        return "IF";
    case ASTNodeType::BLOCK:
        return "BLOCK";
    case ASTNodeType::FOR:
        return "FOR";
    case ASTNodeType::TYPE:
        return "TYPE";
    case ASTNodeType::PARAMETER:
        return "PARAMETER";
    case ASTNodeType::STRUCT:
        return "STRUCT";
    case ASTNodeType::STRUCT_FIELD:
        return "STRUCT_FIELD";
    case ASTNodeType::DEREFERENCE:
        return "DEREFERENCE";
    case ASTNodeType::DEREFERENCE_MEMBER:
        return "DEREFERENCE_MEMBER";
    case ASTNodeType::DEREFERENCE_INDEX:
        return "DEREFERENCE_INDEX";
    case ASTNodeType::CAST:
        return "CAST";
    default:
        return "Unknown";
    }
}