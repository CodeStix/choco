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
            statement = parseStruct(tokens);
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

// ASTTypeNode *parseStructType(TokenStream* tokens, bool isInline)
// {
//     const Token *tok = tokens->peek();
//     if (!isInline)
//     {
//         if (tok->type != TokenType::STRUCT_KEYWORD)
//         {
//             return NULL;
//         }
//         tokens->next();
//     }

//     bool managed = true, packed = false, value = false;
//     bool readingModifiers = true;
//     while (readingModifiers)
//     {
//         tok = tokens->peek();
//         switch (tok->type)
//         {
//         case TokenType::VALUE_KEYWORD:
//             value = true;
//             tokens->next();
//             break;
//         case TokenType::PACKED_KEYWORD:
//             packed = true;
//             tokens->next();
//             break;
//         case TokenType::UNMANAGED_KEYWORD:
//             managed = false;
//             tokens->next();
//             break;
//         default:
//             readingModifiers = false;
//             break;
//         }
//     }

//     const Token *nameToken;
//     if (tok->type == TokenType::SYMBOL)
//     {
//         nameToken = tok;
//         tokens->next();
//         tok = tokens->peek();
//     }
//     else
//     {
//         // Unnamed struct
//         nameToken = NULL;
//         if (!isInline)
//         {
//             std::cout << "ERROR: Global structs must have a name {\n";
//             return NULL;
//         }
//     }

//     if (tok->type != TokenType::CURLY_BRACKET_OPEN)
//     {
//         std::cout << "ERROR: Struct type must start with {\n";
//         return NULL;
//     }
//     tokens->next();

//     std::vector<ASTStructTypeField *> fields;
//     while (1)
//     {
//         const Token *tok = tokens->peek();
//         if (tok->type == TokenType::CURLY_BRACKET_CLOSE)
//         {
//             tokens->next();
//             break;
//         }

//         const Token *fieldNameToken = NULL;
//         if (tok->type == TokenType::SYMBOL)
//         {
//             // Parse field name
//             fieldNameToken = tok;
//             tokens->next();
//             tok = tokens->peek();
//         }

//         if (tok->type == TokenType::COLON)
//         {
//             // Parse field type
//             tokens->next();

//             ASTTypeNode *fieldType = parseInlineType(tokens);
//             bool hidden = false;
//             fields.push_back(new ASTStructTypeField(fieldNameToken, fieldType, hidden));

//             tok = tokens->peek();
//         }
//         else
//         {
//             std::cout << "ERROR: Unexpected token while parsing struct\n";
//             tokens->next();
//             continue;
//         }

//         if (tok->type == TokenType::COMMA)
//         {
//             tokens->next();
//         }
//     }

//     return new ASTStructType(nameToken, fields, managed, packed);
// }

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

    ASTBlock *thenBody = parseBlock(tokens);
    if (thenBody == NULL)
    {
        return NULL;
    }

    ASTNode *elseBody = NULL;
    tok = tokens->peek();
    if (tok->type == TokenType::ELSE_KEYWORD)
    {
        tokens->next();
        tokens->consume(TokenType::WHITESPACE);

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

    ASTBlock *loopBody = parseBlock(tokens);
    if (loopBody == NULL)
    {
        return NULL;
    }

    ASTNode *elseBody = NULL;
    tok = tokens->peek();
    if (tok->type == TokenType::ELSE_KEYWORD)
    {
        tokens->next();
        tokens->consume(TokenType::WHITESPACE);

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
        std::cout << "ERROR: Invalid unary operand at " << tok->position << " type " << getTokenTypeName(tok->type) << "\n";
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

    ASTStruct *structType = parseStruct(tokens);
    if (structType == NULL)
    {
        return NULL;
    }

    return new ASTStructDeclaration(nameToken, structType);
}

ASTStruct *parseStruct(TokenStream *tokens)
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

    return new ASTStruct(fields);
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
        value = parseStruct(tokens);
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
        value = parseStruct(tokens);
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

// ASTNode *parseValueOrTypeCast(TokenStream* tokens)
// {
//     ASTTypeNode *leftType = parseInlineType(tokens);
//     if (leftType == NULL)
//     {
//         // This can't be a type cast, return a value
//         return parseValueOrOperator(tokens);
//     }
//     else
//     {
//         const Token *tok = tokens->peek();
//         switch (tok->type)
//         {
//         case TokenType::AS_KEYWORD:
//         {
//             // This is a cast
//             const Token *operatorToken = tok;
//             tokens->next();
//             ASTNode *rightValue = parseValueOrTypeCast(tokens);
//             if (rightValue == NULL)
//             {
//                 std::cout << "ERROR: could not parse type cast\n";
//                 return NULL;
//             }
//             return new ASTCast(operatorToken, leftType, rightValue);
//         }

//         default:
//             std::cout << "ERROR: found type definition at invalid location\n";
//             return NULL;
//         }
//     }
// }

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
            statement = parseStruct(tokens);
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
            // case TokenType::SYMBOL:
            //     statement = parseValueOrOperator(tokens);
            //     break;
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

llvm::AllocaInst *createAllocaInCurrentFunction(GenerationContext *context, llvm::Type *type, llvm::StringRef twine = "")
{
    llvm::Function *function = context->irBuilder->GetInsertBlock()->getParent();
    llvm::IRBuilder<> insertBuilder(&function->getEntryBlock(), function->getEntryBlock().begin());
    return insertBuilder.CreateAlloca(type, NULL, twine);
}

TypedValue *ASTSymbol::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    std::cout << "DEBUG: ASTSymbol::generateLLVM\n";
    auto valuePointer = scope->getValue(this->nameToken->value);
    if (!valuePointer)
    {
        valuePointer = context->globalModule.getValue(this->nameToken->value, context, scope);
        if (!valuePointer)
        {
            std::cout << "ERROR: Could not find variable '" << this->nameToken->value << "'\n";
            return NULL;
        }
    }
    valuePointer->setOriginVariable(this->nameToken->value);
    return valuePointer;
}

TypedValue *ASTLiteralNumber::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    std::cout << "DEBUG: ASTLiteralNumber::generateLLVM\n";
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
        std::cout << "DEBUG: ASTLiteralNumber::generateLLVM floating\n";
        double floatingValue = strtod(cleaned.c_str(), NULL);
        Type *type = new FloatType(64);
        auto value = llvm::ConstantFP::get(type->getLLVMType(*context->context), floatingValue);
        std::cout << "DEBUG: ASTLiteralNumber::generateLLVM floating 2\n";
        return new TypedValue(value, type);
    }
    else
    {
        std::cout << "DEBUG: ASTLiteralNumber::generateLLVM int\n";
        unsigned long long integerValue = strtoull(cleaned.c_str(), NULL, integerBase);
        bool isSigned = integerValue <= INT64_MAX;
        Type *type = new IntegerType(64, isSigned);
        auto value = llvm::ConstantInt::get(type->getLLVMType(*context->context), integerValue, isSigned);
        std::cout << "DEBUG: ASTLiteralNumber::generateLLVM int 2\n";
        return new TypedValue(value, type);
    }
}

// Generates code that converts a deeply nested pointer to a single-deep pointer
// T*** -> T*
// T* -> T*
// T -> (ERROR)
// T***** -> T*
TypedValue *generateDereferenceToPointer(GenerationContext *context, TypedValue *currentValue)
{
    if (currentValue->getType()->getTypeCode() != TypeCode::POINTER)
    {
        std::cout << "ERROR: cannot generateDereferenceToPointer(...) to pointer when its not a pointer\n";
        return NULL;
    }

    while (1)
    {
        PointerType *currentPointerType = static_cast<PointerType *>(currentValue->getType());
        if (currentPointerType->getPointedType()->getTypeCode() != TypeCode::POINTER)
        {
            // Stop when a value pointer has been reached
            break;
        }

        llvm::Value *derefValue = context->irBuilder->CreateLoad(currentPointerType->getPointedType()->getLLVMType(*context->context), currentValue->getValue(), "deref");
        currentValue = new TypedValue(derefValue, currentPointerType->getPointedType());
    }

    return currentValue;
}

// Generates code that converts a deeply nested pointer to a value
// T*** -> T
// T* -> T
// T -> T
// T***** -> T
TypedValue *generateDereferenceToValue(GenerationContext *context, TypedValue *currentValue)
{
    while (currentValue->getTypeCode() == TypeCode::POINTER)
    {
        PointerType *currentPointerType = static_cast<PointerType *>(currentValue->getType());
        llvm::Value *newValue = context->irBuilder->CreateLoad(currentPointerType->getPointedType()->getLLVMType(*context->context), currentValue->getValue(), "deref");
        currentValue = new TypedValue(newValue, currentPointerType->getPointedType());
    }

    return currentValue;
}

// Converts the left or right value to match the other one's type without losing precision
bool generateTypeJugging(GenerationContext *context, TypedValue **leftInOut, TypedValue **rightInOut)
{
    if (*(*leftInOut)->getType() == *(*rightInOut)->getType())
    {
        return true;
    }

    *leftInOut = generateDereferenceToValue(context, *leftInOut);
    *rightInOut = generateDereferenceToValue(context, *rightInOut);

    auto leftType = (*leftInOut)->getType();
    auto leftValue = (*leftInOut)->getValue();
    auto rightType = (*rightInOut)->getType();
    auto rightValue = (*rightInOut)->getValue();

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
    if (*valueToConvert->getType() == *targetType)
    {
        return valueToConvert;
    }

    if (targetType->getTypeCode() == TypeCode::POINTER)
    {
        std::cout << "ERROR: cannot generateTypeConversion(...) to pointer type\n";
        return NULL;
    }

    valueToConvert = generateDereferenceToValue(context, valueToConvert);
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

TypedValue *ASTStruct::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    std::cout << "DEBUG: ASTStruct::generateLLVM\n";

    bool isType = false;
    std::vector<TypedValue *> fieldValues;
    std::vector<StructTypeField> fieldTypes;
    bool first = true;
    for (auto &field : this->fields)
    {
        TypedValue *fieldValue = field->generateLLVM(context, scope);
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

        fieldValues.push_back(fieldValue);
        fieldTypes.push_back(StructTypeField(fieldValue->getType(), field->getName()));
    }

    if (this->value)
    {
        std::cout << "ERROR: value structs not yet supported\n";
        return NULL;
    }

    StructType *structType = new StructType(fieldTypes, this->managed, this->packed);

    if (!isType)
    {
        // This struct is not a type definition
        llvm::Type *llvmStructType = structType->getLLVMType(*context->context);

        auto structPointer = createAllocaInCurrentFunction(context, llvmStructType, "allocstruct");

        int fieldIndex = 0;
        for (auto &field : fieldValues)
        {
            auto structFieldPointer = context->irBuilder->CreateStructGEP(llvmStructType, structPointer, fieldIndex, "structgep");
            context->irBuilder->CreateStore(field->getValue(), structFieldPointer, false);
            fieldIndex++;
        }

        return new TypedValue(structPointer, structType->getPointerToType());
    }
    else
    {
        return new TypedValue(NULL, structType->getPointerToType());
    }
}

TypedValue *ASTStructField::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    std::cout << "DEBUG: ASTStructField::generateLLVM\n";
    return this->value->generateLLVM(context, scope);
}

TypedValue *ASTUnaryOperator::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    std::cout << "DEBUG: ASTUnaryOperator::generateLLVM\n";
    auto *operand = this->operand->generateLLVM(context, scope);

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

TypedValue *ASTCast::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    std::cout << "DEBUG: ASTCast::generateLLVM\n";

    auto targetType = this->targetType->generateLLVM(context, scope);
    if (targetType == NULL || !targetType->isType())
    {
        std::cout << "ERROR: left-hand side of cast must be a type (got a value)\n";
        return NULL;
    }

    auto value = this->value->generateLLVM(context, scope);
    if (value == NULL || value->isType())
    {
        std::cout << "ERROR: right-hand side of cast must be a value (got a type)\n";
        return NULL;
    }

    return generateTypeConversion(context, value, targetType->getType(), true);
}

TypedValue *ASTOperator::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    TokenType operatorType = this->operatorToken->type;

    std::cout << "DEBUG: ASTOperator::generateLLVM left\n";
    auto *left = this->left->generateLLVM(context, scope);
    std::cout << "DEBUG: ASTOperator::generateLLVM right\n";
    auto *right = this->right->generateLLVM(context, scope);

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

TypedValue *ASTLiteralString::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    std::cout << "DEBUG: ASTLiteralString::generateLLVM\n";
    auto value = context->irBuilder->CreateGlobalString(this->valueToken->value, "str");
    Type *type = new ArrayType(&CHAR_TYPE, this->valueToken->value.length() + 1);
    return new TypedValue(value, type);
}

TypedValue *ASTDeclaration::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    std::cout << "DEBUG: ASTDeclaration::generateLLVM\n";

    if (scope->hasValue(this->nameToken->value))
    {
        std::cout << "ERROR: Cannot redeclare '" << this->nameToken->value << "', it has already been declared\n";
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
        TypedValue *specifiedTypeValue = this->typeSpecifier->generateLLVM(context, scope);
        if (!specifiedTypeValue->isType())
        {
            std::cout << "ERROR: declaration type specifier may not have value\n";
            return NULL;
        }
        specifiedType = specifiedTypeValue->getType();
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
    TypedValue *pointer = new TypedValue(pointerValue, pointerType->getPointerToType());
    scope->addValue(this->nameToken->value, pointer);

    if (initialValue != NULL)
    {
        TypedValue *convertedValue;
        if (specifiedType != NULL)
        {
            convertedValue = generateTypeConversion(context, initialValue, specifiedType, true); // TODO: remove allowLosePrecision when casts are supported
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
    std::cout << "DEBUG: ASTAssignment::generateLLVM\n";

    TypedValue *valuePointer = this->pointerValue->generateLLVM(context, scope);
    if (valuePointer == NULL)
    {
        std::cout << "ERROR: Cannot set variable '" << valuePointer->getOriginVariable() << "', it is not found\n";
        return NULL;
    }
    if (valuePointer->getTypeCode() != TypeCode::POINTER)
    {
        std::cout << "ERROR: assert failed: valuePointer->getTypeCode() != TypeCode::POINTER\n";
        return NULL;
    }
    PointerType *valuePointerType = static_cast<PointerType *>(valuePointer->getType());

    TypedValue *newValue = this->value->generateLLVM(context, scope);

    TypedValue *convertedValue;
    if (*valuePointerType != *newValue->getType()->getPointerToType())
    {
        convertedValue = generateTypeConversion(context, newValue, valuePointerType->getPointedType(), true); // TODO: remove allowLosePrecision when casts are supported
        if (convertedValue == NULL)
        {
            std::cout << "ERROR: Cannot set variable '" << valuePointer->getOriginVariable() << "', right-hand side has invalid type\n";
            return NULL;
        }
    }
    else
    {
        convertedValue = newValue;
    }

    bool isVolatile = false;
    context->irBuilder->CreateStore(convertedValue->getValue(), valuePointer->getValue(), isVolatile);
    return newValue;
}

TypedValue *ASTReturn::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    std::cout << "DEBUG: ASTReturn::generateLLVM\n";
    if (this->value != NULL)
    {
        auto value = this->value->generateLLVM(context, scope);

        Type *returnType = context->currentFunction->getReturnType();
        if (returnType == NULL)
        {
            std::cout << "ERROR: function does not return value\n";
            return NULL;
        }

        TypedValue *newValue = generateTypeConversion(context, value, returnType, true); // TODO: remove allowLosePrecision when casts are supported
        if (newValue == NULL)
        {
            std::cout << "ERROR: cannot convert return value in function\n";
            return NULL;
        }

        context->irBuilder->CreateRet(newValue->getValue());
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
    std::cout << "DEBUG: ASTBlock::generateLLVM\n";
    TypedValue *lastValue = NULL;
    for (ASTNode *statement : *this->statements)
    {
        std::cout << "DEBUG: ASTBlock::generateLLVM generate " << astNodeTypeToString(statement->type) << "\n";
        lastValue = statement->generateLLVM(context, scope);
    }
    return lastValue;
}

TypedValue *ASTParameter::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    std::cout << "DEBUG: ASTParameter::generateLLVM\n";
    return this->typeSpecifier->generateLLVM(context, scope);
}

TypedValue *ASTFunction::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    std::cout << "DEBUG: ASTFunction::generateLLVM\n";

    if (context->globalModule.hasValue(this->nameToken->value))
    {
        std::cout << "ERROR: Duplicate name '" << this->nameToken->value << "' cannot be used again for function.\n";
        return NULL;
    }

    std::vector<FunctionParameter> parameters;
    for (ASTParameter *parameter : *this->parameters)
    {
        TypedValue *parameterTypeValue = parameter->generateLLVM(context, scope);
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
        TypedValue *returnTypeValue = this->returnType->generateLLVM(context, scope);
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

    int i = 0;
    for (auto &arg : function->args())
    {
        ASTParameter *parameter = (*this->parameters)[i++];
        arg.setName(parameter->getParameterName());
    }

    TypedValue *existingFunctionValue = new TypedValue(function, newFunctionType->getPointerToType());

    if (!context->globalModule.addValue(this->nameToken->value, existingFunctionValue))
    {
        std::cout << "ERROR: The function '" << this->nameToken->value << "' has already been declared";
        return NULL;
    }

    if (this->body != NULL)
    {
        llvm::Function *function = static_cast<llvm::Function *>(existingFunctionValue->getValue());
        if (!function->empty())
        {
            std::cout << "ERROR: Cannot implement the '" << this->nameToken->value << "' function a second time\n";
            return NULL;
        }

        FunctionScope *functionScope = new FunctionScope(*scope);
        llvm::BasicBlock *functionStartBlock = llvm::BasicBlock::Create(*context->context, "block", function);
        context->irBuilder->SetInsertPoint(functionStartBlock);

        int i = 0;
        for (auto &parameterValue : function->args())
        {
            ASTParameter *parameter = (*this->parameters)[i++];

            TypedValue *parameterTypeValue = parameter->generateLLVM(context, scope);
            if (!parameterTypeValue->isType())
            {
                std::cout << "ERROR: parameter type specifier may not have value\n";
                return NULL;
            }
            Type *parameterType = parameterTypeValue->getType();

            auto parameterPointer = context->irBuilder->CreateAlloca(parameterType->getLLVMType(*context->context), NULL, "loadarg");
            context->irBuilder->CreateStore(&parameterValue, parameterPointer, false);
            functionScope->addValue(parameter->getParameterName(), new TypedValue(parameterPointer, parameterType->getPointerToType()));
        }

        PointerType *functionPointerType = static_cast<PointerType *>(existingFunctionValue->getType());
        context->currentFunction = static_cast<FunctionType *>(functionPointerType->getPointedType());
        this->body->generateLLVM(context, functionScope);

        // Check if the function was propery terminated
        llvm::BasicBlock *functionEndBlock = context->irBuilder->GetInsertBlock();
        if (functionEndBlock->getTerminator() == NULL)
        {
            if (this->returnType == NULL)
            {
                context->irBuilder->CreateRetVoid();
            }
            else
            {
                std::cout << "ERROR: function '" << this->nameToken->value << "' must return a value";
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

    return existingFunctionValue;
}

TypedValue *ASTWhileStatement::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    std::cout << "DEBUG: ASTWhileStatement::generateLLVM\n";
    TypedValue *preConditionValue = this->condition->generateLLVM(context, scope);
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
    this->loopBody->generateLLVM(context, loopScope);

    TypedValue *conditionValue = this->condition->generateLLVM(context, loopScope);
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
        this->elseBody->generateLLVM(context, elseScope);
    }
    llvm::BasicBlock *elseEndBlock = context->irBuilder->GetInsertBlock(); // Current block could have changed in generateLLVM calls above, update it here
    if (elseEndBlock->getTerminator() == NULL)
    {
        context->irBuilder->CreateBr(continueBlock);
    }

    context->irBuilder->SetInsertPoint(continueBlock);
    continueBlock->insertInto(parentFunction);
    return NULL;
}

TypedValue *ASTIndexDereference::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    std::cout << "DEBUG: ASTIndexDereference::generateLLVM\n";

    TypedValue *valueToIndex = this->toIndex->generateLLVM(context, scope);
    TypedValue *pointerToIndex = generateDereferenceToPointer(context, valueToIndex);
    if (pointerToIndex == NULL)
    {
        std::cout << "ERROR: index dereference only supports pointers\n";
        return NULL;
    }

    TypedValue *indexValue = this->index->generateLLVM(context, scope);

    std::cout << "ERROR: index dereference not implemented\n";
    return NULL;
}

TypedValue *ASTMemberDereference::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    std::cout << "DEBUG: ASTMemberDereference::generateLLVM\n";

    TypedValue *valueToIndex = this->toIndex->generateLLVM(context, scope);
    TypedValue *pointerToIndex = generateDereferenceToPointer(context, valueToIndex);
    if (pointerToIndex == NULL)
    {
        std::cout << "ERROR: member dereference only supports pointers\n";
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
        return new TypedValue(fieldPointer, structField->type->getPointerToType());
    }
    else
    {
        std::cout << "ERROR: member dereference only supports structs\n";
        return NULL;
    }
}

TypedValue *ASTIfStatement::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    std::cout << "DEBUG: ASTIfStatement::generateLLVM\n";

    TypedValue *condition = this->condition->generateLLVM(context, scope);
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
    this->thenBody->generateLLVM(context, thenScope);

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
        this->elseBody->generateLLVM(context, elseScope);
    }

    llvm::BasicBlock *elseEndBlock = context->irBuilder->GetInsertBlock(); // Current block could have changed in generateLLVM calls above, update it here
    if (elseEndBlock->getTerminator() == NULL)
    {
        context->irBuilder->CreateBr(continueBlock);
    }

    context->irBuilder->SetInsertPoint(continueBlock);
    continueBlock->insertInto(parentFunction);

    return NULL;
}

TypedValue *ASTInvocation::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    std::cout << "DEBUG: ASTInvocation::generateLLVM\n";
    TypedValue *functionValue = this->functionPointerValue->generateLLVM(context, scope);
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
    for (int p = 0; p < actualParameterCount; p++)
    {
        FunctionParameter &parameter = parameters[p];
        ASTNode *parameterNode = (*this->parameterValues)[p];
        TypedValue *parameterValue = parameterNode->generateLLVM(context, scope);
        if (parameterValue == NULL || parameterValue->getValue() == NULL)
        {
            return NULL;
        }

        TypedValue *convertedValue = generateTypeConversion(context, parameterValue, parameter.type, true); // TODO: remove allowLosePrecision when casts are supported
        if (convertedValue == NULL || convertedValue->getValue() == NULL)
        {
            std::cout << "ERROR: cannot convert value '" << parameter.name << "' for invoke '" << functionValue->getOriginVariable() << "'\n";
            return NULL;
        }
        parameterValues.push_back(convertedValue->getValue());
    }

    llvm::Value *callResult = context->irBuilder->CreateCall(function, parameterValues, functionType->getReturnType() == NULL ? "" : "call");
    return new TypedValue(callResult, functionType->getReturnType());
}

TypedValue *ASTBrackets::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    std::cout << "DEBUG: ASTBrackets::generateLLVM\n";
    return this->inner->generateLLVM(context, scope);
}

TypedValue *ASTFile::generateLLVM(GenerationContext *context, FunctionScope *scope)
{
    std::cout << "DEBUG: ASTFile::generateLLVM\n";
    FunctionScope *fileScope = new FunctionScope(*scope);
    for (ASTNode *statement : *this->statements)
    {
        statement->generateLLVM(context, fileScope);
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
    default:
        return "Unknown";
    }
}