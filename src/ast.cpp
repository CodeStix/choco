#include "ast.hpp"
#include "context.hpp"
#include "util.hpp"

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
            statement = parseValueOrOperator(tokens, false);
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

    ASTNode *condition = parseValueOrOperator(tokens, false);
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

    ASTNode *condition = parseValueOrOperator(tokens, false);
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

    ASTNode *operand = parseValueAndSuffix(tokens, false);
    if (operand == NULL)
    {
        // std::cout << "ERROR: Invalid unary operand at " << tok->position << " type " << getTokenTypeName(tok->type) << "\n";
        tokens->setPosition(saved);
        return NULL;
    }

    return new ASTUnaryOperator(operandToken, operand);
}

ASTNode *parseArray(TokenStream *tokens)
{
    int saved = tokens->getPosition();

    bool managed = true, value = false;
    bool readingModifiers = true;
    while (readingModifiers)
    {
        const Token *tok = tokens->peek();
        switch (tok->type)
        {
        case TokenType::VALUE_KEYWORD:
            value = true;
            managed = false;
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

    const Token *tok = tokens->peek();
    if (tok->type != TokenType::SQUARE_BRACKET_OPEN)
    {
        std::cout << "ERROR: Array value must start with [\n";
        tokens->setPosition(saved);
        return NULL;
    }
    tokens->next();
    tokens->consume(TokenType::WHITESPACE);
    tokens->consume(TokenType::NEWLINE);

    std::vector<ASTArraySegment *> values;
    while (1)
    {
        tok = tokens->peek();
        if (tok->type == TokenType::SQUARE_BRACKET_CLOSE)
        {
            tokens->next();
            break;
        }

        ASTNode *value = parseValueOrOperator(tokens, false);
        if (value == NULL)
        {
            std::cout << "ERROR: Could not parse array value\n";
            return NULL;
        }

        tokens->consume(TokenType::WHITESPACE);
        tokens->consume(TokenType::NEWLINE);
        tok = tokens->peek();

        ASTNode *times = NULL;
        if (tok->type == TokenType::OPERATOR_HASHTAG)
        {
            tokens->next();
            tokens->consume(TokenType::WHITESPACE);
            tokens->consume(TokenType::NEWLINE);

            times = value;
            value = NULL;
            value = parseValueOrOperator(tokens, false);
            if (value == NULL)
            {
                std::cout << "ERROR: Could not parse array times\n";
                return NULL;
            }

            tokens->consume(TokenType::WHITESPACE);
            tokens->consume(TokenType::NEWLINE);
        }

        tokens->consume(TokenType::COMMA);
        tokens->consume(TokenType::WHITESPACE);
        tokens->consume(TokenType::NEWLINE);

        values.push_back(new ASTArraySegment(value, times));
    }

    return new ASTArray(values, managed, value);
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
            managed = false;
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

        ASTNode *fieldValue = parseValueOrOperator(tokens, false);
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
    return parseValueOrOperator(tokens, true);
}

ASTNode *parseValueOrType(TokenStream *tokens, bool parseType)
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
    case TokenType::SQUARE_BRACKET_OPEN:
    {
        value = parseStruct(tokens, NULL);
        if (value == NULL)
        {
            value = parseArray(tokens);
        }
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

        ASTNode *innerValue = parseValueOrOperator(tokens, parseType);
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

        // case TokenType::LET_KEYWORD:
        // {
        //     value = parseDeclaration(tokens);
        //     if (value == NULL)
        //     {
        //         std::cout << "ERROR: Unexpected let \n";
        //         return NULL;
        //     }
        //     break;
        // }

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

ASTNode *parseValueAndSuffix(TokenStream *tokens, bool parseType)
{
    int saved = tokens->getPosition();
    ASTNode *value = parseValueOrType(tokens, parseType);
    if (value == NULL)
    {
        return value;
    }

    while (1)
    {
        const Token *tok = tokens->peek();
        if (!parseType && tok->type == TokenType::BRACKET_OPEN)
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

                ASTNode *value = parseValueOrOperator(tokens, false);
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

            ASTNode *indexValue = parseValueOrOperator(tokens, parseType);
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
            tokens->next();

            value = new ASTIndexDereference(value, indexValue);
        }
        else if (tok->type == TokenType::OPERATOR_QUESTION_MARK)
        {
            tokens->next();
            tokens->consume(TokenType::WHITESPACE);
            return new ASTNullCoalesce(value);
        }
        else if (tok->type == TokenType::OPERATOR_EXCLAMATION)
        {
            std::cout << "ERROR: ! suffix not implemented\n";
            return NULL;
        }
        else if (!parseType && tok->type == TokenType::WHITESPACE)
        {
            tokens->next();
            ASTNode *castedValue = parseValueAndSuffix(tokens, false);
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

ASTNode *parseValueOrOperator(TokenStream *tokens, bool parseType)
{
    int saved = tokens->getPosition();
    ASTNode *top = parseValueAndSuffix(tokens, parseType);
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

            ASTNode *right = parseValueOrOperator(tokens, false);
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

        ASTNode *right = parseValueAndSuffix(tokens, parseType);
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

    ASTNode *value = parseValueOrOperator(tokens, false);
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
        if (typeSpecifier == NULL)
        {
            std::cout << "ERROR: Invalid declaration type specifier\n";
            return NULL;
        }

        tokens->consume(TokenType::WHITESPACE);
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

        ASTNode *value = parseValueOrOperator(tokens, false);
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

TypedValue *ASTSymbol::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint, bool expectPointer)
{
#ifdef DEBUG
    std::cout << "debug: ASTSymbol::generateLLVM " << this->nameToken->value << "\n";
#endif

    if (this->nameToken->value == "null")
    {
        if (typeHint == NULL)
        {
            return new TypedValue(NULL, new NullType());
        }
        else if (typeHint->getTypeCode() == TypeCode::POINTER)
        {
            std::cout << "ERROR: null can only be assigned to a nullable pointer (please union it with null)\n";
            exit(-1);

            return NULL;
        }
        else if (typeHint->getTypeCode() == TypeCode::UNION)
        {
            UnionType *unionType = static_cast<UnionType *>(typeHint);
            if (unionType->containsNullType())
            {
                auto llvmPointerType = static_cast<llvm::PointerType *>(typeHint->getLLVMType(context));
                return new TypedValue(llvm::ConstantPointerNull::get(llvmPointerType), typeHint);
            }
            else
            {
                std::cout << "ERROR: null not assignable to union " << typeHint->toString() << " (please add it)\n";
                exit(-1);

                return NULL;
            }
        }
        else
        {
            std::cout << "ERROR: null is not assignable to " << typeHint->toString() << "\n";
            exit(-1);

            return NULL;
        }
    }

    auto valuePointer = scope == NULL ? NULL : scope->getValue(this->nameToken->value);
    if (valuePointer == NULL)
    {
        valuePointer = context->globalModule->getValueCascade(this->nameToken->value, context, scope);
        if (!valuePointer)
        {
            std::cout << "ERROR: Could not find '" << this->nameToken->value << "'\n";
            exit(-1);

            return NULL;
        }
    }

    valuePointer->setOriginVariable(this->nameToken->value);

    if (valuePointer->isType())
    {
        return valuePointer;
    }

    if (expectPointer)
    {
        return valuePointer;
    }
    else
    {
        return generateReferenceAwareLoad(context, valuePointer);
    }
}

TypedValue *ASTLiteralNumber::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint, bool expectPointer)
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
                exit(-1);

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
        auto value = llvm::ConstantFP::get(type->getLLVMType(context), floatingValue);

#ifdef DEBUG
        std::cout << "debug: ASTLiteralNumber::generateLLVM float\n";
#endif
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

        auto value = llvm::ConstantInt::get(type->getLLVMType(context), integerValue, type->getSigned());

#ifdef DEBUG
        std::cout << "debug: ASTLiteralNumber::generateLLVM integer\n";
#endif
        return new TypedValue(value, type);
    }
}

TypedValue *ASTArray::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint, bool expectPointer)
{
    std::vector<TypedValue *> segmentValues;
    for (auto v : this->values)
    {
        segmentValues.push_back(v->getValue()->generateLLVM(context, scope, NULL, false));
    }

    if (segmentValues.size() == 1 && segmentValues[0]->isType())
    {
        // This is an array type

        ArrayType *arrayType = NULL;
        auto times = this->values[0]->getTimes();
        if (times != NULL)
        {
            TypedValue *timesValue = times->generateLLVM(context, scope, &UINT64_TYPE, false);
            llvm::ConstantInt *llvmTimesValue = llvm::cast<llvm::ConstantInt>(timesValue->getValue());
            uint64_t timesInt = llvmTimesValue->getValue().getZExtValue();

            arrayType = new ArrayType(segmentValues[0]->getType(), timesInt, this->value, this->managed);
        }
        else
        {
            if (this->value)
            {
                std::cout << "ERROR: Value array must have known item count\n";
                exit(-1);
                return NULL;
            }

            arrayType = new ArrayType(segmentValues[0]->getType(), this->value, this->managed);
        }

        return new TypedValue(NULL, arrayType);
    }
    else
    {
        assert(segmentValues.size() > 0 && "TODO allow empty array");

        uint64_t arrayItemCount = 0;
        Type *arrayItemType = segmentValues[0]->getType();
        std::vector<llvm::Value *> llvmArrayValues;
        for (int i = 0; i < segmentValues.size(); i++)
        {
            if (*segmentValues[i]->getType() != *arrayItemType)
            {
                std::cout << "ERROR: All values in the array must be of the same type " << arrayItemType->toString() << "\n";
                exit(-1);
                return NULL;
            }

            ASTNode *timesNode = this->values[i]->getTimes();

            uint64_t timesInt;
            if (timesNode != NULL)
            {
                TypedValue *timesValue = timesNode->generateLLVM(context, scope, &UINT64_TYPE, false);
                llvm::ConstantInt *llvmTimesValue = llvm::cast<llvm::ConstantInt>(timesValue->getValue());
                timesInt = llvmTimesValue->getValue().getZExtValue();
            }
            else
            {
                timesInt = 1;
            }

            for (uint64_t j = 0; j < timesInt; j++)
            {
                llvmArrayValues.push_back(segmentValues[i]->getValue());
            }

            arrayItemCount += timesInt;
        }

        if (typeHint == NULL)
        {
            // Infer type
            if (segmentValues.size() == 0)
            {
                std::cout << "ERROR: It is impossible to infer empty array type\n";
                exit(-1);
                return NULL;
            }

            typeHint = new ArrayType(arrayItemType, arrayItemCount, this->value, this->managed);
        }
        else
        {
            if (typeHint->getTypeCode() != TypeCode::ARRAY)
            {
                std::cout << "ERROR: Array cannot assign to type " << typeHint->toString() << "\n";
                return NULL;
            }

            ArrayType *arrayTypeHint = static_cast<ArrayType *>(typeHint);
            if (*arrayItemType != *arrayTypeHint->getItemType() || (arrayTypeHint->hasKnownCount() && arrayTypeHint->getCount() != arrayItemCount) /* || arrayTypeHint->getManaged() != this->managed || arrayTypeHint->getByValue() != this->value*/)
            {
                std::cout << "ERROR: Array cannot assign to type " << typeHint->toString() << ", invalid count, item type, managed or value\n";
                return NULL;
            }
        }

        ArrayType *arrayType = static_cast<ArrayType *>(typeHint);

        if (arrayType->getByValue())
        {
            llvm::ArrayType *llvmArrayType = llvm::cast<llvm::ArrayType>(arrayType->getLLVMType(context));
            llvm::Type *llvmArrayItemType = arrayItemType->getLLVMType(context);

            std::vector<llvm::Constant *> llvmUndefValues;
            for (auto v : llvmArrayValues)
            {
                llvmUndefValues.push_back(llvm::UndefValue::get(llvmArrayItemType));
            }
            llvm::Value *llvmArrayValue = llvm::ConstantArray::get(llvmArrayType, llvmUndefValues);

            for (uint64_t i = 0; i < arrayItemCount; i++)
            {
                llvm::Value *llvmValue = llvmArrayValues[i];
                llvmArrayValue = context->irBuilder->CreateInsertValue(llvmArrayValue, llvmValue, (unsigned int)i, "array.set." + std::to_string(i));
            }

            return new TypedValue(llvmArrayValue, arrayType);
        }
        else
        {
            auto arrayPointerType = (new PointerType(new ArrayType(arrayType->getItemType(), true, false), arrayType->getManaged()));
            auto llvmArrayPointer = generateMalloc(context, arrayPointerType->getLLVMPointedType(context), "array.malloc");
            // auto llvmArrayPointer = context->irBuilder->CreateBitCast(llvmArrayPointerUncasted, arrayType->getLLVMArrayPointerType(context), "array.malloc");

            // llvmArrayPointerUncasted->print(llvm::outs());
            // std::cout << "\n";
            // llvmArrayPointer->print(llvm::outs());
            // std::cout << "\n";

            for (uint64_t i = 0; i < arrayItemCount; i++)
            {
                llvm::Value *llvmValue = llvmArrayValues[i];

                std::vector<llvm::Value *> indices;
                indices.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context->context), 0, false));
                if (arrayType->getManaged())
                {
                    // TODO: move this to shared code
                    indices.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context->context), 1, false));
                }
                indices.push_back(llvm::ConstantInt::get(ArrayType::getLLVMLengthFieldType(context), i, false));

                auto llvmArrayItemPtr = context->irBuilder->CreateGEP(arrayPointerType->getLLVMPointedType(context), llvmArrayPointer, indices, "array.set." + std::to_string(i) + ".gep");
                context->irBuilder->CreateStore(llvmValue, llvmArrayItemPtr, false);
            }

            if (arrayType->getManaged())
            {
                // Set initial ref count to 1
                // TODO: move this to shared code
                std::vector<llvm::Value *> refCountIndices;
                refCountIndices.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context->context), 0, false));
                refCountIndices.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context->context), 0, false));
                auto llvmRefCountPointer = context->irBuilder->CreateGEP(arrayPointerType->getLLVMPointedType(context), llvmArrayPointer, refCountIndices, "array.ref");
                context->irBuilder->CreateStore(llvm::ConstantInt::get(getRefCountType(*context->context), 1, false), llvmRefCountPointer, false);

                std::vector<llvm::Constant *> llvmLengthStructFields;
                llvmLengthStructFields.push_back(llvm::ConstantInt::get(ArrayType::getLLVMLengthFieldType(context), arrayItemCount, false));
                llvmLengthStructFields.push_back(llvm::UndefValue::get(arrayPointerType->getLLVMType(context)));
                llvm::Value *llvmLengthStruct = llvm::ConstantStruct::get(arrayType->getLLVMLengthStructType(context), llvmLengthStructFields);

                std::vector<unsigned int> indices;
                indices.push_back(1);

                llvmLengthStruct = context->irBuilder->CreateInsertValue(llvmLengthStruct, llvmArrayPointer, indices, "array.sized");

                return new TypedValue(llvmLengthStruct, arrayType);
            }
            else
            {
                return new TypedValue(llvmArrayPointer, arrayType);
            }
        }
    }
}

TypedValue *ASTArraySegment::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint, bool expectPointer)
{
    assert(false && "ASTArraySegment->getValue() and ASTArraySegment->getTimes() should be used to generate LLVM");
    return NULL;
    // return this->value->generateLLVM(context, scope, typeHint, expectPointer);
}

TypedValue *ASTNullCoalesce::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint, bool expectPointer)
{
    assert(false && "Null coalesce isn't used");
    return NULL;
}

TypedValue *ASTStruct::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint, bool expectPointer)
{
#ifdef DEBUG
    std::cout << "debug: ASTStruct::generateLLVM\n";
#endif

    std::map<std::string, TypedValue *> fieldValues;
    StructType *structType = NULL;
    bool isType = false;
    bool byValue = false;
    bool managed = false;

    // Enforce type hint
    if (typeHint != NULL)
    {
        if (typeHint->getTypeCode() == TypeCode::POINTER)
        {
            PointerType *typeHintPointer = static_cast<PointerType *>(typeHint);
            if (typeHintPointer->getPointedType()->getTypeCode() == TypeCode::STRUCT)
            {
                structType = static_cast<StructType *>(typeHintPointer->getPointedType());
                byValue = false;
                managed = typeHintPointer->isManaged();
            }
        }
        else if (typeHint->getTypeCode() == TypeCode::STRUCT)
        {
            structType = static_cast<StructType *>(typeHint);
            byValue = true;
            managed = false;
        }
        else
        {
            std::cout << "ERROR: Unexpected struct, expected " << typeHint->toString() << "\n";
            exit(-1);

            return NULL;
        }

        if (this->value || !this->managed || this->packed)
        {
            std::cout << "ERROR: Struct modifier cannot be specified again\n";
            exit(-1);

            return NULL;
        }

        for (auto &field : this->fields)
        {
            auto hintField = structType->getField(field->getName());
            if (hintField == NULL)
            {
                std::cout << "ERROR: Struct field " << field->getName() << " does not exist on type " << typeHint->toString() << "\n";
                exit(-1);

                return NULL;
            }

            TypedValue *fieldValue = field->generateLLVM(context, scope, hintField->type, false);
            if (fieldValue->isType())
            {
                std::cout << "ERROR: Struct field cannot be initialized with a type\n";
                exit(-1);

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
            TypedValue *fieldValue = field->generateLLVM(context, scope, NULL, false);
            if (first)
            {
                first = false;
                isType = fieldValue->isType();
            }
            if (fieldValue->isType() != isType)
            {
                std::cout << "ERROR: cannot mix value and type structs\n";
                exit(-1);

                return NULL;
            }

            fieldValues[field->getName()] = fieldValue;

            Type *fieldType = fieldValue->getType();
            fieldTypes.push_back(StructTypeField(fieldType, field->getName()));
        }

        structType = new StructType(this->nameToken == NULL ? "" : this->nameToken->value, fieldTypes, this->packed);
        byValue = this->value;
        managed = this->managed;
    }

    if (!isType)
    {
        TypedValue *result;
        if (byValue)
        {
            // Allocate struct in registers
            llvm::Value *structValue = llvm::UndefValue::get(structType->getLLVMType(context));

            for (auto &pair : fieldValues)
            {
                auto fieldName = pair.first;
                auto fieldValue = pair.second;
                auto fieldType = structType->getField(fieldName);
                auto fieldIndex = structType->getFieldIndex(fieldName);

                TypedValue *convertedFieldValue = generateTypeConversion(context, fieldValue, fieldType->type, false);
                if (!convertedFieldValue)
                {
                    std::cout << "ERROR: Could not set field " << fieldName << " of value struct initialization\n";
                    exit(-1);

                    return NULL;
                }

                std::vector<unsigned int> indices;
                indices.push_back((unsigned int)fieldIndex);
                structValue = context->irBuilder->CreateInsertValue(structValue, convertedFieldValue->getValue(), indices, structType->getName());
            }

            result = new TypedValue(structValue, structType);
        }
        else
        {
            // Allocate struct on the heap (or stack)

            if (managed)
            {
                result = new TypedValue(generateMalloc(context, structType->getManagedPointerToType()->getLLVMPointedType(context), structType->getName()), new PointerType(structType, managed));

                // Set initial ref count to 1
                std::vector<llvm::Value *> indices;
                indices.push_back(llvm::ConstantInt::get(llvm::IntegerType::getInt32Ty(*context->context), 0, false));
                indices.push_back(llvm::ConstantInt::get(llvm::IntegerType::getInt32Ty(*context->context), 0, false));
                PointerType *structPointerType = static_cast<PointerType *>(result->getType());
                llvm::Value *refCountFieldPointer = context->irBuilder->CreateGEP(structPointerType->getLLVMPointedType(context), result->getValue(), indices, structType->getName() + ".refcount.ptr");
                context->irBuilder->CreateStore(llvm::ConstantInt::get(getRefCountType(*context->context), 1, false), refCountFieldPointer, false);
            }
            else
            {
                result = new TypedValue(generateAllocaInCurrentFunction(context, structType->getLLVMType(context), structType->getName()), new PointerType(structType, managed));
            }

            for (auto &pair : fieldValues)
            {
                auto fieldName = pair.first;
                auto fieldValue = pair.second;
                auto fieldType = structType->getField(fieldName);
                auto fieldIndex = structType->getFieldIndex(fieldName);

                std::vector<llvm::Value *> indices;
                indices.push_back(llvm::ConstantInt::get(llvm::IntegerType::getInt32Ty(*context->context), 0, false));
                if (managed)
                {
                    // If the pointer is managed, the actual struct is loaded under the second field (the first field is the reference count)
                    indices.push_back(llvm::ConstantInt::get(llvm::IntegerType::getInt32Ty(*context->context), 1, false));
                }
                indices.push_back(llvm::ConstantInt::get(llvm::IntegerType::getInt32Ty(*context->context), fieldIndex, false));

                PointerType *structPointerType = static_cast<PointerType *>(result->getType());
                llvm::Value *fieldPointer = context->irBuilder->CreateGEP(structPointerType->getLLVMPointedType(context), result->getValue(), indices, structType->getName() + "." + fieldName + ".ptr");

                // auto structFieldPointer = context->irBuilder->CreateStructGEP(llvmStructType, structPointer, fieldIndex, "structgep");
                TypedValue *structFieldPointerValue = new TypedValue(fieldPointer, fieldType->type->getUnmanagedPointerToType(), fieldName);
                bool isVolatile = false;
                if (!generateAssignment(context, structFieldPointerValue, fieldValue, isVolatile))
                {
                    std::cout << "ERROR: Cannot initialize struct field " << fieldName << " of " << structType->toString() << "\n";
                    exit(-1);

                    return NULL;
                }
            }
        }

        // Check if all fields were set
        for (auto &field : structType->getFields())
        {
            if (fieldValues[field.name] == NULL)
            {
                std::cout << "ERROR: Struct field " << field.name << " of " << structType->toString() << " must be initialized\n";
                exit(-1);

                return NULL;
            }
        }

        return result;
    }
    else
    {
        TypedValue *type;
        if (byValue)
        {
            type = new TypedValue(NULL, structType);
        }
        else
        {
            type = new TypedValue(NULL, new PointerType(structType, managed));
        }

        if (this->nameToken != NULL)
        {
            if (!context->globalModule->addValue(this->nameToken->value, type))
            {
                std::cout << "ERROR: The struct '" << this->nameToken->value << "' has already been declared";
                exit(-1);

                return NULL;
            }
        }

        return type;
    }
}

TypedValue *ASTStructField::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint, bool expectPointer)
{
#ifdef DEBUG
    std::cout << "debug: ASTStructField::generateLLVM " << this->nameToken->value << "\n";
#endif
    return this->value->generateLLVM(context, scope, typeHint, expectPointer);
}

TypedValue *ASTUnaryOperator::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint, bool expectPointer)
{
#ifdef DEBUG
    std::cout << "debug: ASTUnaryOperator::generateLLVM\n";
#endif
    auto *operand = this->operand->generateLLVM(context, scope, NULL, expectPointer);

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
                exit(-1);

                return NULL;
            }

            llvm::Value *newValue = context->irBuilder->CreateNeg(operand->getValue(), "neg", false, false);
            return new TypedValue(newValue, operand->getType());
        }
        else
        {
            std::cout << "ERROR: Cannot use operator " << this->operatorToken->value << " on value\n";
            exit(-1);

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
            exit(-1);

            return NULL;
        }

    default:
        std::cout << "ERROR: Unimplemented unary operator " << this->operatorToken->value << "\n";
        exit(-1);

        return NULL;
    }
}

TypedValue *ASTCast::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint, bool expectPointer)
{
#ifdef DEBUG
    std::cout << "debug: ASTCast::generateLLVM\n";
#endif

    auto targetType = this->targetType->generateLLVM(context, scope, NULL, false);
    if (targetType == NULL || !targetType->isType())
    {
        std::cout << "ERROR: Left-hand side of cast must be a type (got a value)\n";
        exit(-1);

        return NULL;
    }

    auto value = this->value->generateLLVM(context, scope, targetType->getType(), expectPointer);
    if (value == NULL || value->isType())
    {
        std::cout << "ERROR: Right-hand side of cast must be a value (got a type)\n";
        exit(-1);

        return NULL;
    }

    return generateTypeConversion(context, value, targetType->getType(), true);
}

TypedValue *ASTOperator::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint, bool expectPointer)
{
    TokenType operatorType = this->operatorToken->type;

#ifdef DEBUG
    std::cout << "debug: ASTOperator::generateLLVM left\n";
#endif
    auto *left = this->left->generateLLVM(context, scope, NULL, false);
#ifdef DEBUG
    std::cout << "debug: ASTOperator::generateLLVM right\n";
#endif
    auto *right = this->right->generateLLVM(context, scope, NULL, false);

    if (!left || !right)
    {
        return NULL;
    }

    if (left->isType() || right->isType())
    {
        if (operatorType == TokenType::OPERATOR_EQUALS)
        {
            if (!(left->isType() && right->isType()))
            {
                std::cout << "ERROR: Cannot perform operator " << this->operatorToken->value << " on type and value\n";
                exit(-1);

                return NULL;
            }

            bool value = *right->getType() == *left->getType();
            auto llvmValue = llvm::ConstantInt::get(BOOL_TYPE.getLLVMType(context), value ? 1 : 0, false);
            return new TypedValue(llvmValue, &BOOL_TYPE);
        }
        else if (operatorType == TokenType::IS_KEYWORD)
        {
            if (!right->isType())
            {
                std::cout << "ERROR: Cannot perform operator " << this->operatorToken->value << " on type and value\n";
                exit(-1);

                return NULL;
            }

            if (left->isType())
            {
                assert(false && "'<type> is <type>' is currently unimplemented");
            }
            else
            {
                if (left->getTypeCode() == TypeCode::UNION)
                {
                    auto isValue = generateUnionIs(context, left, right->getType());
                    generateDecrementReferenceIfPointer(context, left, false);
                    return isValue;
                }
                else
                {
                    std::cout << "ERROR: Cannot perform is operator on " << left->getType()->toString() << "\n";
                    exit(-1);

                    return NULL;
                }
            }
        }
        else if (operatorType == TokenType::OPERATOR_OR)
        {
            if (!(left->isType() && right->isType()))
            {
                std::cout << "ERROR: Cannot perform operator " << this->operatorToken->value << " on type and value\n";
                exit(-1);

                return NULL;
            }

            Type *newType = NULL;
            if (left->getTypeCode() == TypeCode::UNION && right->getTypeCode() == TypeCode::UNION)
            {
                UnionType *leftUnion = static_cast<UnionType *>(left->getType());
                UnionType *rightUnion = static_cast<UnionType *>(right->getType());
                leftUnion->addTypes(rightUnion->getTypes());
                newType = leftUnion;
            }
            else if (left->getTypeCode() == TypeCode::UNION)
            {
                UnionType *leftUnion = static_cast<UnionType *>(left->getType());
                leftUnion->addType(right->getType());
                newType = leftUnion;
            }
            else if (right->getTypeCode() == TypeCode::UNION)
            {
                UnionType *rightUnion = static_cast<UnionType *>(right->getType());
                rightUnion->addType(left->getType());
                newType = rightUnion;
            }
            else
            {
                if (*right->getType() == *left->getType())
                {
                    newType = right->getType();
                }
                else
                {
                    auto t = new UnionType();
                    t->addType(right->getType());
                    t->addType(left->getType());
                    newType = t;
                }
            }
            return new TypedValue(NULL, newType);
        }
        else
        {
            std::cout << "ERROR: Cannot perform operator " << this->operatorToken->value << " on types\n";
            exit(-1);

            return NULL;
        }

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
            exit(-1);

            return NULL;
        }
    }
    else
    {
        if (*left->getType() != *right->getType())
        {
            std::cout << "ERROR: Left and right operands must be the same type to perform " << this->operatorToken->value << "\n";
            exit(-1);

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
            exit(-1);

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
                exit(-1);

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
                exit(-1);

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
            exit(-1);

            return NULL;
        }

        return new TypedValue(result, resultingType);
    }
    else
    {
        std::cout << "ERROR: Cannot " << this->operatorToken->value << " values, the type does not support this operator\n";
        exit(-1);

        return NULL;
    }
}

TypedValue *ASTLiteralString::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint, bool expectPointer)
{
#ifdef DEBUG
    std::cout << "debug: ASTLiteralString::generateLLVM\n";
#endif
    auto value = context->irBuilder->CreateGlobalString(this->valueToken->value, "str");
    Type *type = new ArrayType(&CHAR_TYPE, this->valueToken->value.length() + 1, false, true);
    return new TypedValue(value, type);
}

TypedValue *ASTDeclaration::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint, bool expectPointer)
{
#ifdef DEBUG
    std::cout << "debug: ASTDeclaration::generateLLVM\n";
#endif

    if (scope->hasValue(this->nameToken->value))
    {
        std::cout << "ERROR: Cannot redeclare '" << this->nameToken->value << "', it has already been declared\n";
        exit(-1);

        return NULL;
    }

    Type *specifiedType;
    if (this->typeSpecifier != NULL)
    {
        TypedValue *specifiedTypeValue = this->typeSpecifier->generateLLVM(context, scope, NULL, false);
        if (!specifiedTypeValue->isType())
        {
            std::cout << "ERROR: Declaration type specifier may not have value\n";
            exit(-1);

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
        initialValue = this->value->generateLLVM(context, scope, specifiedType, false);
    }
    else
    {
        initialValue = NULL;
        // TODO: remove this error when unimplemented variables have been implemented
        std::cout << "ERROR: Declaration must have initial value (due to uninitialized variables not being implemented)\n";
        exit(-1);

        return NULL;
    }

    Type *storedType;
    if (specifiedType != NULL)
    {
        storedType = specifiedType;
    }
    else
    {
        if (initialValue != NULL)
        {
            storedType = initialValue->getType();
        }
        else
        {
            std::cout << "ERROR: Declaration type must be specified when value is missing\n";
            exit(-1);

            return NULL;
        }
    }

    llvm::Value *pointerValue = generateAllocaInCurrentFunction(context, storedType->getLLVMType(context), this->nameToken->value);
    TypedValue *valuePointer = new TypedValue(pointerValue, storedType->getUnmanagedPointerToType());

    if (!scope->addValue(this->nameToken->value, valuePointer))
    {
        std::cout << "ERROR: Cannot generate declaration for " << this->nameToken->value << "\n";
        exit(-1);

        return NULL;
    }

    bool isVolatile = false;
    if (!generateAssignment(context, valuePointer, initialValue, isVolatile))
    {
        std::cout << "ERROR: Cannot generate declaration for " << this->nameToken->value << "\n";
        exit(-1);

        return NULL;
    }

    return valuePointer;
}

TypedValue *ASTAssignment::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint, bool expectPointer)
{
#ifdef DEBUG
    std::cout << "debug: ASTAssignment::generateLLVM\n";
#endif

    bool isVolatile = false;

    TypedValue *valuePointer = this->pointerValue->generateLLVM(context, scope, NULL, true);
    if (valuePointer == NULL)
    {
        std::cout << "ERROR: Cannot set variable '" << valuePointer->getOriginVariable() << "', it is not found\n";
        exit(-1);

        return NULL;
    }
    if (valuePointer->getTypeCode() != TypeCode::POINTER)
    {
        std::cout << "ERROR: Assert failed: valuePointer->getTypeCode() != TypeCode::POINTER\n";
        exit(-1);

        return NULL;
    }
    PointerType *valuePointerType = static_cast<PointerType *>(valuePointer->getType());

    // TODO: this code will segfault when the declaration hasn't specified a value (previous value is uninitialized)
    llvm::Value *llvmPreviousStoredValue = context->irBuilder->CreateLoad(valuePointerType->getPointedType()->getLLVMType(context), valuePointer->getValue(), valuePointer->getOriginVariable() + ".load");
    generateDecrementReferenceIfPointer(context, new TypedValue(llvmPreviousStoredValue, valuePointerType->getPointedType()), false);

    // if (valuePointerType->getPointedType()->getTypeCode() == TypeCode::POINTER)
    // {
    //     // If a previous pointer value will be overwritten, the reference count must be decremented if was a managed pointer
    //     PointerType *storedPointerType = static_cast<PointerType *>(valuePointerType->getPointedType());
    //     if (storedPointerType->isManaged())
    //     {
    //         llvm::Value *storedManagedPointer = context->irBuilder->CreateLoad(storedPointerType->getLLVMType(context), valuePointer->getValue(), valuePointer->getOriginVariable() + ".load");
    //         if (!generateDecrementReference(context, new TypedValue(storedManagedPointer, storedPointerType), false))
    //         {
    //             std::cout << "ERROR: Could not generate decrement managed pointer code for assignment\n";
    // exit(-1);

    //             return NULL;
    //         }
    //     }
    // }

    TypedValue *newValue = this->value->generateLLVM(context, scope, valuePointerType->getPointedType(), false);

    if (!generateAssignment(context, valuePointer, newValue, isVolatile))
    {
        std::cout << "ERROR: Cannot generate assignment\n";
        exit(-1);
        return NULL;
    }

    return valuePointer;
}

TypedValue *ASTReturn::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint, bool expectPointer)
{
#ifdef DEBUG
    std::cout << "debug: ASTReturn::generateLLVM\n";
#endif
    Type *returnType = context->currentFunction->getReturnType();
    if (this->value != NULL)
    {
        if (returnType == NULL)
        {
            std::cout << "ERROR: Function does not return value\n";
            exit(-1);
            return NULL;
        }

        auto value = this->value->generateLLVM(context, scope, returnType, false);
        if (value == NULL)
        {
            std::cout << "ERROR: Could not generate return value\n";
            exit(-1);

            return NULL;
        }

        TypedValue *newValue = generateTypeConversion(context, value, returnType, true); // TODO: remove allowLosePrecision when casts are supported
        if (newValue == NULL)
        {
            std::cout << "ERROR: Cannot convert return value in function\n";
            exit(-1);

            return NULL;
        }

        context->irBuilder->CreateStore(newValue->getValue(), context->currentFunctionReturnValuePointer, false);
        context->irBuilder->CreateBr(context->currentFunctionReturnBlock);
        return NULL;
    }
    else
    {
        if (returnType != NULL)
        {
            std::cout << "ERROR: Return statement must provide a value\n";
            exit(-1);

            return NULL;
        }
        context->irBuilder->CreateBr(context->currentFunctionReturnBlock);
        return NULL;
    }
}

TypedValue *ASTBlock::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint, bool expectPointer)
{
#ifdef DEBUG
    std::cout << "debug: ASTBlock::generateLLVM\n";
#endif
    for (ASTNode *statement : *this->statements)
    {
#ifdef DEBUG
        std::cout << "debug: ASTBlock::generateLLVM generate " << astNodeTypeToString(statement->type) << "\n";
#endif
        TypedValue *value = statement->generateLLVM(context, scope, NULL, true);
#ifdef DEBUG
        std::cout << "debug: ASTBlock::generateLLVM generate " << astNodeTypeToString(statement->type) << "done \n";
#endif
        if (value != NULL)
        {
#ifdef DEBUG
            std::cout << "debug: ASTBlock::generateLLVM generate " << value->getValue() << " " << value->getType()->toString() << " done2\n";
#endif
            generateDecrementReferenceIfPointer(context, value, false);
        }
    }
#ifdef DEBUG
    std::cout << "debug: ASTBlock::generateLLVM cdone\n";
#endif
    return NULL;
}

TypedValue *ASTParameter::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint, bool expectPointer)
{
#ifdef DEBUG
    std::cout << "debug: ASTParameter::generateLLVM\n";
#endif
    return this->typeSpecifier->generateLLVM(context, scope, typeHint, expectPointer);
}

TypedValue *ASTFunction::generateLLVM(GenerationContext *context, FunctionScope *_, Type *typeHint, bool expectPointer)
{
#ifdef DEBUG
    std::cout << "debug: ASTFunction::generateLLVM " << this->nameToken->value << "\n";
#endif

    std::vector<FunctionParameter> parameters;
    for (ASTParameter *parameter : *this->parameters)
    {
        TypedValue *parameterTypeValue = parameter->generateLLVM(context, NULL, NULL, false);
        if (!parameterTypeValue->isType())
        {
            std::cout << "ERROR: parameter type specifier may not have value\n";
            exit(-1);

            return NULL;
        }
        parameters.push_back(FunctionParameter(parameterTypeValue->getType(), parameter->getParameterName()));
    }

    Type *returnType;
    if (this->returnType != NULL)
    {
        TypedValue *returnTypeValue = this->returnType->generateLLVM(context, NULL, NULL, false);
        if (!returnTypeValue->isType())
        {
            std::cout << "ERROR: return type specifier may not have value\n";
            exit(-1);

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
    llvm::FunctionType *functionType = static_cast<llvm::FunctionType *>(newFunctionType->getLLVMType(context));
    llvm::Function *function = llvm::Function::Create(functionType, linkage, this->nameToken->value, *context->module);
    if (function == NULL)
    {
        std::cout << "ERROR: Function::Create returned null\n";
        exit(-1);

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

    // Name parameters and add parameter attributes when needed
    for (int i = 0; i < parameters.size(); i++)
    {
        auto parameter = parameters[i];
        auto parameterValue = function->getArg(i);
        parameterValue->setName(parameter.name);

        // if (parameter.type->getTypeCode() == TypeCode::POINTER)
        // {
        //     PointerType *parameterPointerType = static_cast<PointerType *>(parameter.type);
        //     if (parameterPointerType->isByValue())
        //     {
        //         // Add ByVal attribute to pointers that should be passed by value
        //         auto attributeBuilder = llvm::AttrBuilder(*context->context);
        //         attributeBuilder.addByValAttr(parameterPointerType->getPointedType()->getLLVMType(context));
        //         parameterValue->addAttrs(attributeBuilder);
        //     }
        // }
    }

    TypedValue *newFunctionPointerType = new TypedValue(function, newFunctionType->getUnmanagedPointerToType());

    if (!context->globalModule->addValue(this->nameToken->value, newFunctionPointerType))
    {
        std::cout << "ERROR: The function '" << this->nameToken->value << "' has already been declared";
        exit(-1);

        return NULL;
    }

    if (this->body != NULL)
    {
        llvm::Function *function = static_cast<llvm::Function *>(newFunctionPointerType->getValue());
        if (!function->empty())
        {
            std::cout << "ERROR: Cannot implement the '" << this->nameToken->value << "' function a second time\n";
            exit(-1);

            return NULL;
        }

        FunctionScope *functionScope = new FunctionScope();
        llvm::BasicBlock *functionStartBlock = llvm::BasicBlock::Create(*context->context, this->nameToken->value + ".entry", function);
        context->irBuilder->SetInsertPoint(functionStartBlock);

        PointerType *functionPointerType = static_cast<PointerType *>(newFunctionPointerType->getType());
        context->currentFunction = static_cast<FunctionType *>(functionPointerType->getPointedType());
        context->currentFunctionReturnBlock = llvm::BasicBlock::Create(*context->context, this->nameToken->value + ".return", function);
        context->currentFunctionReturnValuePointer = returnType == NULL ? NULL : generateAllocaInCurrentFunction(context, returnType->getLLVMType(context), "return");

        for (int i = 0; i < this->parameters->size(); i++)
        {
            ASTParameter *parameter = (*this->parameters)[i];
            auto parameterValue = function->getArg(i);

            TypedValue *parameterTypeValue = parameter->generateLLVM(context, NULL, NULL, false);
            if (!parameterTypeValue->isType())
            {
                std::cout << "ERROR: parameter type specifier may not have value\n";
                exit(-1);

                return NULL;
            }
            Type *parameterType = parameterTypeValue->getType();

            auto parameterPointer = context->irBuilder->CreateAlloca(parameterType->getLLVMType(context), NULL, "loadarg");
            context->irBuilder->CreateStore(parameterValue, parameterPointer, false);
            functionScope->addValue(parameter->getParameterName(), new TypedValue(parameterPointer, parameterType->getUnmanagedPointerToType()));
        }

        this->body->generateLLVM(context, functionScope, NULL, true);

        // Check if the function was propery terminated
        // llvm::BasicBlock *functionEndBlock = context->irBuilder->GetInsertBlock();
        if (!this->body->isTerminating())
        {
            if (returnType == NULL)
            {
                context->irBuilder->CreateBr(context->currentFunctionReturnBlock);
            }
            else
            {
                std::cout << "ERROR: Function '" << this->nameToken->value << "' must return a value in all execution paths\n";
                exit(-1);

                return NULL;
            }
        }

        // Create return block and free values
        context->irBuilder->SetInsertPoint(context->currentFunctionReturnBlock);

        for (auto &p : functionScope->namedValues)
        {
            if (p.second->isType())
            {
                // std::map value could be null
                continue;
            }

            PointerType *valuePointerType = static_cast<PointerType *>(p.second->getType());

            llvm::Value *finalizedValue = context->irBuilder->CreateLoad(valuePointerType->getPointedType()->getLLVMType(context), p.second->getValue(), p.second->getOriginVariable() + ".load");
            generateDecrementReferenceIfPointer(context, new TypedValue(finalizedValue, valuePointerType->getPointedType()), true);

            // PointerType *valuePointerType = static_cast<PointerType *>(p.second->getType());
            // if (valuePointerType->getPointedType()->getTypeCode() == TypeCode::POINTER)
            // {
            //     PointerType *storedPointerType = static_cast<PointerType *>(valuePointerType->getPointedType());
            //     if (storedPointerType->isManaged())
            //     {
            //         llvm::Value *storedManagedPointer = context->irBuilder->CreateLoad(storedPointerType->getLLVMType(context), p.second->getValue(), p.second->getOriginVariable() + ".load");
            //         if (!generateDecrementReference(context, new TypedValue(storedManagedPointer, storedPointerType), true))
            //         {
            //             std::cout << "ERROR: Could not generate decrement managed pointer code for return\n";
            // exit(-1);

            //             return NULL;
            //         }
            //     }
            // }
        }

        if (context->currentFunctionReturnValuePointer != NULL)
        {
            // auto returnValue = generateReferenceAwareLoad(context, new TypedValue(context->currentFunctionReturnValuePointer, returnType->getUnmanagedPointerToType(false)));
            auto returnValue = generateLoad(context, new TypedValue(context->currentFunctionReturnValuePointer, returnType->getUnmanagedPointerToType()));
            context->irBuilder->CreateRet(returnValue->getValue());
        }
        else
        {
            context->irBuilder->CreateRetVoid();
        }

        // Check generated IR for issues
        if (llvm::verifyFunction(*function, &llvm::errs()))
        {
            std::cout << "ERROR: LLVM reported invalid function\n";
            exit(-1);

            return NULL;
        }

        // Optimize the function code
        context->passManager->run(*function);
    }

    return newFunctionPointerType;
}

TypedValue *ASTWhileStatement::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint, bool expectPointer)
{
#ifdef DEBUG
    std::cout << "debug: ASTWhileStatement::generateLLVM\n";
#endif
    TypedValue *preConditionValue = this->condition->generateLLVM(context, scope, &BOOL_TYPE, false);
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
    auto loopScope = scope; // TODO: new FunctionScope(*scope); This does not work because finalizers will not free
    this->loopBody->generateLLVM(context, loopScope, NULL, true);

    TypedValue *conditionValue = this->condition->generateLLVM(context, loopScope, &BOOL_TYPE, false);
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
    auto elseScope = scope; // TODO: new FunctionScope(*scope); This does not work because finalizers will not free
    if (this->elseBody != NULL)
    {
        this->elseBody->generateLLVM(context, elseScope, NULL, true);
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

TypedValue *ASTIndexDereference::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint, bool expectPointer)
{
#ifdef DEBUG
    std::cout << "debug: ASTIndexDereference::generateLLVM\n";
#endif

    TypedValue *valueToIndex = this->toIndex->generateLLVM(context, scope, NULL, false);
    TypedValue *indexValue = this->index->generateLLVM(context, scope, &UINT64_TYPE, false);

    if (valueToIndex->getTypeCode() == TypeCode::ARRAY)
    {
        ArrayType *arrayType = static_cast<ArrayType *>(valueToIndex->getType());

        // TODO check indexValue->getValue() array bounds

        llvm::Value *llvmArrayPointer;
        std::vector<llvm::Value *> indices;
        if (arrayType->getManaged())
        {
            // TODO: move this to shared code
            // Select array pointer (not length field)
            indices.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context->context), 0, false));
            // Select array (not ref count field)
            indices.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context->context), 1, false));

            std::vector<unsigned int> lengthIndices;
            lengthIndices.push_back(1);
            llvmArrayPointer = context->irBuilder->CreateExtractValue(valueToIndex->getValue(), lengthIndices, "array.ptr");
        }
        else
        {
            llvmArrayPointer = indexValue->getValue();
        }
        indices.push_back(indexValue->getValue());

        auto llvmArrayItemPtr = context->irBuilder->CreateGEP(arrayType->getArrayPointerType()->getLLVMPointedType(context), llvmArrayPointer, indices, "array.index.gep");

        auto itemPointer = new TypedValue(llvmArrayItemPtr, arrayType->getItemType()->getUnmanagedPointerToType());
        if (expectPointer)
        {
            return itemPointer;
        }
        else
        {
            return generateReferenceAwareLoad(context, itemPointer);
        }
    }
    else
    {
        std::cout << "ERROR: Can only index dereference arrays\n";
        exit(-1);
        return NULL;
    }
}

TypedValue *ASTMemberDereference::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint, bool expectPointer)
{
#ifdef DEBUG
    std::cout << "debug: ASTMemberDereference::generateLLVM\n";
#endif

    TypedValue *valueToIndex = this->toIndex->generateLLVM(context, scope, NULL, true);

    if (valueToIndex->isType())
    {
        if (valueToIndex->getType()->getTypeCode() == TypeCode::MODULE)
        {
            ModuleType *mod = static_cast<ModuleType *>(valueToIndex->getType());
            TypedValue *moduleValue = mod->getValue(this->nameToken->value, context, scope);
            if (moduleValue == NULL)
            {
                std::cout << "ERROR: '" << this->nameToken->value << "' cannot be found in module '" << mod->getFullName() << "'\n";
                exit(-1);

                return NULL;
            }
            return moduleValue;
        }
        else
        {
            std::cout << "ERROR: Type has no members to dereference\n";
            exit(-1);

            return NULL;
        }
    }

    TypedValue *pointerToIndex = generateDereferenceToPointer(context, valueToIndex);
    if (pointerToIndex == NULL)
    {
        std::cout << "ERROR: Member dereference only supports pointers\n";
        exit(-1);

        return NULL;
    }

    PointerType *pointerTypeToIndex = static_cast<PointerType *>(pointerToIndex->getType());

    if (pointerTypeToIndex->getPointedType()->getTypeCode() == TypeCode::STRUCT)
    {
        StructType *structType = static_cast<StructType *>(pointerTypeToIndex->getPointedType());

        // The builtin 'refs' field contains the reference count
        if (this->nameToken->value == "refs")
        {
            if (!pointerTypeToIndex->isManaged())
            {
                std::cout << "ERROR: Cannot read reference count of unmanaged object\n";
                exit(-1);

                return NULL;
            }

            std::vector<llvm::Value *> indices;
            indices.push_back(llvm::Constant::getIntegerValue(llvm::IntegerType::getInt32Ty(*context->context), llvm::APInt(32, 0, false)));
            indices.push_back(llvm::Constant::getIntegerValue(llvm::IntegerType::getInt32Ty(*context->context), llvm::APInt(32, 0, false)));

            std::string twine = pointerToIndex->getOriginVariable() + ".refs";
            llvm::Value *fieldPointer = context->irBuilder->CreateGEP(pointerTypeToIndex->getLLVMPointedType(context), pointerToIndex->getValue(), indices, twine);

            generateDecrementReferenceIfPointer(context, pointerToIndex, false);

            return new TypedValue(fieldPointer, (new IntegerType(64, false))->getUnmanagedPointerToType());
        }

        int fieldIndex = structType->getFieldIndex(this->nameToken->value);
        if (fieldIndex < 0)
        {
            std::cout << "ERROR: Cannot access member '" << this->nameToken->value << "' of struct\n";
            exit(-1);

            return NULL;
        }
        StructTypeField *structField = structType->getField(this->nameToken->value);

        // A struct is indexed
        std::vector<llvm::Value *> indices;
        indices.push_back(llvm::Constant::getIntegerValue(llvm::IntegerType::getInt32Ty(*context->context), llvm::APInt(32, 0, false)));
        if (pointerTypeToIndex->isManaged())
        {
            // If the pointer is managed, the actual struct is loaded under the second field (the first field is the reference count)
            indices.push_back(llvm::Constant::getIntegerValue(llvm::IntegerType::getInt32Ty(*context->context), llvm::APInt(32, 1, false)));
        }
        indices.push_back(llvm::Constant::getIntegerValue(llvm::IntegerType::getInt32Ty(*context->context), llvm::APInt(32, fieldIndex, false)));

        // std::cout << "DEBUG: GEP 3 \n";
        // llvmStructType->print(llvm::outs(), true);
        // std::cout << "\n";
        // pointerToIndex->getValue()->print(llvm::outs(), true);
        // std::cout << "\n";

        std::string twine = pointerToIndex->getOriginVariable() + "." + this->nameToken->value + ".ptr";
        llvm::Value *fieldPointer = context->irBuilder->CreateGEP(pointerTypeToIndex->getLLVMPointedType(context), pointerToIndex->getValue(), indices, twine);

        generateDecrementReferenceIfPointer(context, pointerToIndex, false);

        return new TypedValue(fieldPointer, structField->type->getUnmanagedPointerToType(), twine);
    }
    else
    {
        std::cout << "ERROR: Member dereference only supports structs\n";
        exit(-1);

        return NULL;
    }
}

TypedValue *ASTIfStatement::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint, bool expectPointer)
{
#ifdef DEBUG
    std::cout << "debug: ASTIfStatement::generateLLVM\n";
#endif

    TypedValue *condition = this->condition->generateLLVM(context, scope, &BOOL_TYPE, false);
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
    auto thenScope = scope; // TODO: new FunctionScope(*scope); This does not work because finalizers will not free
    this->thenBody->generateLLVM(context, thenScope, NULL, true);

    llvm::BasicBlock *thenEndBlock = context->irBuilder->GetInsertBlock(); // Current block could have changed in generateLLVM calls above, update it here
    if (thenEndBlock->getTerminator() == NULL)
    {
        context->irBuilder->CreateBr(continueBlock);
    }

    context->irBuilder->SetInsertPoint(elseStartBlock);
    elseStartBlock->insertInto(parentFunction);
    auto elseScope = scope; // TODO: new FunctionScope(*scope); This does not work because finalizers will not free
    if (this->elseBody != NULL)
    {
        this->elseBody->generateLLVM(context, elseScope, NULL, true);
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

TypedValue *ASTInvocation::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint, bool expectPointer)
{
#ifdef DEBUG
    std::cout << "debug: ASTInvocation::generateLLVM\n";
#endif
    TypedValue *functionValue = this->functionPointerValue->generateLLVM(context, scope, NULL, true);
    if (functionValue == NULL)
    {
        std::cout << "ERROR: Function to call not found\n";
        exit(-1);

        return NULL;
    }

    if (functionValue->getTypeCode() != TypeCode::POINTER)
    {
        std::cout << "ERROR: Cannot invoke '" << functionValue->getOriginVariable() << "', it must be a pointer\n";
        exit(-1);

        return NULL;
    }
    PointerType *functionPointerType = static_cast<PointerType *>(functionValue->getType());
    if (functionPointerType->getPointedType()->getTypeCode() != TypeCode::FUNCTION)
    {
        std::cout << "ERROR: Cannot invoke '" << functionValue->getOriginVariable() << "', it must be a function pointer\n";
        exit(-1);

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
        exit(-1);

        return NULL;
    }

    std::vector<llvm::Value *> parameterValues;

    for (int p = 0; p < actualParameterCount; p++)
    {
        FunctionParameter &parameter = parameters[p];
        ASTNode *parameterNode = (*this->parameterValues)[p];
        TypedValue *parameterValue = parameterNode->generateLLVM(context, scope, parameter.type, false);
        if (parameterValue == NULL || parameterValue->getValue() == NULL)
        {
            return NULL;
        }

        TypedValue *convertedValue = generateTypeConversion(context, parameterValue, parameter.type, false);
        if (convertedValue == NULL)
        {
            std::cout << "ERROR: Cannot convert value '" << parameter.name << "' for invoke '" << functionValue->getOriginVariable() << "'\n";
            exit(-1);

            return NULL;
        }
        if (convertedValue->isType())
        {
            std::cout << "ERROR: Types cannot be passed as parameters\n";
            exit(-1);

            return NULL;
        }
        parameterValues.push_back(convertedValue->getValue());
    }

    auto callResult = context->irBuilder->CreateCall(function, parameterValues, functionType->getReturnType() == NULL ? "" : (function->getName() + ".call"));
#ifdef DEBUG
    std::cout << "debug: ASTInvocation::generateLLVM done\n";
#endif
    if (functionType->getReturnType() == NULL)
    {
        return NULL;
    }
    else
    {
        return new TypedValue(callResult, functionType->getReturnType());
    }
}

TypedValue *ASTBrackets::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint, bool expectPointer)
{
#ifdef DEBUG
    std::cout << "debug: ASTBrackets::generateLLVM\n";
#endif
    return this->inner->generateLLVM(context, scope, typeHint, expectPointer);
}

TypedValue *ASTFile::generateLLVM(GenerationContext *context, FunctionScope *scope, Type *typeHint, bool expectPointer)
{
#ifdef DEBUG
    std::cout << "debug: ASTFile::generateLLVM\n";
#endif
    FunctionScope *fileScope = new FunctionScope();
    for (ASTNode *statement : *this->statements)
    {
        statement->generateLLVM(context, fileScope, NULL, true);
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