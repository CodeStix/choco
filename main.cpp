#include <iostream>
#include <string>
#include <fstream>
#include <list>

enum class TokenType
{
    SYMBOL,
    FUNC_KEYWORD,
    RETURN_KEYWORD,
    LET_KEYWORD,
    CONST_KEYWORD,
    LITERAL_STRING,
    LITERAL_CHAR,
    LITERAL_NUMBER,
    BRACKET_OPEN,
    BRACKET_CLOSE,
    SQUARE_BRACKET_OPEN,
    SQUARE_BRACKET_CLOSE,
    CURLY_BRACKET_OPEN,
    CURLY_BRACKET_CLOSE,
    ASSIGNMENT_OPERATOR,
    ADDITION_OPERATOR,
    SUBSTRACTION_OPERATOR,
    DIVISION_OPERATOR,
    MULTIPLICATION_OPERATOR,
};

const char *getTokenTypeName(TokenType type)
{
    switch (type)
    {
    case TokenType::SYMBOL:
        return "SYMBOL";
    case TokenType::LITERAL_STRING:
        return "LITERAL_STRING";
    case TokenType::LITERAL_CHAR:
        return "LITERAL_CHAR";
    case TokenType::LITERAL_NUMBER:
        return "LITERAL_NUMBER";
    case TokenType::BRACKET_OPEN:
        return "BRACKET_OPEN";
    case TokenType::BRACKET_CLOSE:
        return "BRACKET_CLOSE";
    case TokenType::SQUARE_BRACKET_OPEN:
        return "SQUARE_BRACKET_OPEN";
    case TokenType::SQUARE_BRACKET_CLOSE:
        return "SQUARE_BRACKET_CLOSE";
    case TokenType::CURLY_BRACKET_OPEN:
        return "CURLY_BRACKET_OPEN";
    case TokenType::CURLY_BRACKET_CLOSE:
        return "CURLY_BRACKET_CLOSE";
    case TokenType::LET_KEYWORD:
        return "LET_KEYWORD";
    case TokenType::FUNC_KEYWORD:
        return "FUNC_KEYWORD";
    case TokenType::RETURN_KEYWORD:
        return "RETURN_KEYWORD";
    case TokenType::CONST_KEYWORD:
        return "CONST_KEYWORD";
    case TokenType::ASSIGNMENT_OPERATOR:
        return "ASSIGNMENT_OPERATOR";
    case TokenType::ADDITION_OPERATOR:
        return "ADDITION_OPERATOR";
    case TokenType::SUBSTRACTION_OPERATOR:
        return "SUBSTRACTION_OPERATOR";
    case TokenType::DIVISION_OPERATOR:
        return "DIVISION_OPERATOR";
    case TokenType::MULTIPLICATION_OPERATOR:
        return "MULTIPLICATION_OPERATOR";
    default:
        return "Unknown";
    }
}

class Token
{
public:
    Token(int position, TokenType type, std::string value) : position(position), type(type), value(value){};

    int position;
    TokenType type;
    std::string value;
};

int getTokenOperatorImporance(const Token *tok)
{
    switch (tok->type)
    {
    case TokenType::ADDITION_OPERATOR:
        return 1;
    case TokenType::SUBSTRACTION_OPERATOR:
        return 1;
    case TokenType::MULTIPLICATION_OPERATOR:
        return 2;
    case TokenType::DIVISION_OPERATOR:
        return 2;
    default:
        return -1;
    }
}

enum class TokenizeState
{
    NONE,
    PARSING_LITERAL_STRING,
    PARSING_LITERAL_CHAR,
    PARSING_LITERAL_NUMBER,
    PARSING_OPERATOR,
    PARSING_SYMBOL,
};

void parseString(std::string &input, std::list<Token *> &tokenList)
{
    std::string currentString;
    TokenizeState state = TokenizeState::NONE;

    for (int i = 0; i < input.length(); i++)
    {
        char currentChar = input[i];

        if (state == TokenizeState::PARSING_LITERAL_STRING)
        {
            if (currentChar == '"')
            {
                tokenList.push_back(new Token(i, TokenType::LITERAL_STRING, currentString));
                state = TokenizeState::NONE;
                continue;
            }
            else
            {
                currentString += currentChar;
            }
        }
        else if (state == TokenizeState::PARSING_LITERAL_CHAR)
        {
            if (currentChar == '\'')
            {
                tokenList.push_back(new Token(i, TokenType::LITERAL_CHAR, currentString));
                state = TokenizeState::NONE;
                continue;
            }
            else
            {
                currentString += currentChar;
            }
        }
        else if (state == TokenizeState::PARSING_LITERAL_NUMBER)
        {
            if (isdigit(currentChar))
            {
                currentString += currentChar;
            }
            else
            {
                tokenList.push_back(new Token(i, TokenType::LITERAL_NUMBER, currentString));
                state = TokenizeState::NONE;
            }
        }
        else if (state == TokenizeState::PARSING_SYMBOL)
        {
            if (isalnum(currentChar))
            {
                currentString += currentChar;
            }
            else
            {
                TokenType type;
                if (currentString == "func")
                {
                    type = TokenType::FUNC_KEYWORD;
                }
                else if (currentString == "return")
                {
                    type = TokenType::RETURN_KEYWORD;
                }
                else if (currentString == "let")
                {
                    type = TokenType::LET_KEYWORD;
                }
                else if (currentString == "const")
                {
                    type = TokenType::CONST_KEYWORD;
                }
                else
                {
                    type = TokenType::SYMBOL;
                }

                tokenList.push_back(new Token(i, type, currentString));
                state = TokenizeState::NONE;
            }
        }

        if (state == TokenizeState::NONE)
        {
            if (isalpha(currentChar))
            {
                state = TokenizeState::PARSING_SYMBOL;
                currentString = std::string(1, currentChar);
            }
            else if (isdigit(currentChar))
            {
                state = TokenizeState::PARSING_LITERAL_NUMBER;
                currentString = std::string(1, currentChar);
            }
            else if (isspace(currentChar))
            {
                continue;
            }
            else
            {
                switch (currentChar)
                {
                case '"':
                    state = TokenizeState::PARSING_LITERAL_STRING;
                    currentString = "";
                    break;
                case '\'':
                    state = TokenizeState::PARSING_LITERAL_CHAR;
                    currentString = "";
                    break;

                case '(':
                    tokenList.push_back(new Token(i, TokenType::BRACKET_OPEN, std::string(1, currentChar)));
                    break;
                case ')':
                    tokenList.push_back(new Token(i, TokenType::BRACKET_CLOSE, std::string(1, currentChar)));
                    break;
                case '[':
                    tokenList.push_back(new Token(i, TokenType::SQUARE_BRACKET_OPEN, std::string(1, currentChar)));
                    break;
                case ']':
                    tokenList.push_back(new Token(i, TokenType::SQUARE_BRACKET_CLOSE, std::string(1, currentChar)));
                    break;
                case '{':
                    tokenList.push_back(new Token(i, TokenType::CURLY_BRACKET_OPEN, std::string(1, currentChar)));
                    break;
                case '}':
                    tokenList.push_back(new Token(i, TokenType::CURLY_BRACKET_CLOSE, std::string(1, currentChar)));
                    break;
                case '=':
                    tokenList.push_back(new Token(i, TokenType::ASSIGNMENT_OPERATOR, std::string(1, currentChar)));
                    break;
                case '+':
                    tokenList.push_back(new Token(i, TokenType::ADDITION_OPERATOR, std::string(1, currentChar)));
                    break;
                case '-':
                    tokenList.push_back(new Token(i, TokenType::SUBSTRACTION_OPERATOR, std::string(1, currentChar)));
                    break;
                case '*':
                    tokenList.push_back(new Token(i, TokenType::MULTIPLICATION_OPERATOR, std::string(1, currentChar)));
                    break;
                case '/':
                    tokenList.push_back(new Token(i, TokenType::DIVISION_OPERATOR, std::string(1, currentChar)));
                    break;

                default:
                    std::cout << "WARNING: Unknown char " << currentChar << "\n";
                    break;
                }
            }
        }
    }

    if (state != TokenizeState::NONE)
    {
        std::cout << "WARNING: Found end of file too early, was still parsing " << static_cast<int>(state) << "\n";
    }
}

enum class ASTNodeType
{
    OPERATOR,
    LITERAL_NUMBER,
    LITERAL_STRING,
    FUNCTION,
    DECLARATION,
    ASSIGNMENT,
};

class ASTNode
{
public:
    ASTNode(ASTNodeType type) : type(type) {}
    ASTNodeType type;

    virtual std::string toString() = 0;
};

class ASTOperator : public ASTNode
{
public:
    ASTOperator(const Token *operatorToken, ASTNode *left, ASTNode *right) : ASTNode(ASTNodeType::OPERATOR), operatorToken(operatorToken), left(left), right(right) {}
    const Token *operatorToken;
    ASTNode *left;
    ASTNode *right;

    virtual std::string toString()
    {
        std::string str = "(";
        str += this->left->toString();
        str += " ";
        str += this->operatorToken->value;
        str += " ";
        str += this->right->toString();
        str += ")";
        return str;
    }
};

class ASTLiteralString : public ASTNode
{
public:
    ASTLiteralString(const Token *value) : ASTNode(ASTNodeType::LITERAL_STRING), valueToken(value) {}
    const Token *valueToken;

    virtual std::string toString()
    {
        std::string str = "\"";
        str += this->valueToken->value;
        str += "\"";
        return str;
    }
};

class ASTLiteralNumber : public ASTNode
{
public:
    ASTLiteralNumber(const Token *value) : ASTNode(ASTNodeType::LITERAL_NUMBER), valueToken(value) {}
    const Token *valueToken;

    virtual std::string toString()
    {
        std::string str = this->valueToken->value;
        return str;
    }
};

class ASTDeclaration : public ASTNode
{
public:
    ASTDeclaration(const Token *nameToken, ASTNode *value) : ASTNode(ASTNodeType::DECLARATION), nameToken(nameToken), value(value) {}
    const Token *nameToken;
    ASTNode *value;

    virtual std::string toString()
    {
        std::string str = "const ";
        str += this->nameToken->value;
        str += " = ";
        str += this->value->toString();
        return str;
    }
};

class ASTFunction : public ASTNode
{
public:
    ASTFunction(const Token *nameToken, std::list<ASTNode *> *statements) : ASTNode(ASTNodeType::FUNCTION), nameToken(nameToken), statements(statements) {}
    const Token *nameToken;
    std::list<ASTNode *> *statements;

    virtual std::string toString()
    {
        std::string str = "func ";
        str += this->nameToken->value;
        str += " {\n";
        for (ASTNode *statement : *this->statements)
        {
            str += statement->toString();
            str += "\n";
        }
        str += "} ";
        return str;
    }
};

ASTDeclaration *parseConstDeclaration(std::list<Token *> &tokens);

ASTFunction *parseFunction(std::list<Token *> &tokens)
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

    tok = tokens.front();
    if (tok == NULL)
    {
        std::cout << "ERROR: Unexpected end of file\n";
        return NULL;
    }
    if (tok->type != TokenType::BRACKET_CLOSE)
    {
        std::cout << "ERROR: Function must close parameter list\n";
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
    if (tok->type != TokenType::CURLY_BRACKET_OPEN)
    {
        std::cout << "ERROR: Function must have body\n";
    }
    else
    {
        tokens.pop_front();
    }

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

        switch (tok->type)
        {
        case TokenType::FUNC_KEYWORD:
            statements->push_front(parseFunction(tokens));
            break;
        case TokenType::CONST_KEYWORD:
            statements->push_front(parseConstDeclaration(tokens));
            break;

        default:
            std::cout << "ERROR: Invalid function statement\n";
            break;
        }
    }

    return new ASTFunction(nameToken, statements);
}

ASTNode *parseValue(std::list<Token *> &tokens)
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
        break;
    case TokenType::LITERAL_NUMBER:
        value = new ASTLiteralNumber(tok);
        break;
    default:
        std::cout << "ERROR: Not a value: " << getTokenTypeName(tok->type) << "\n";
        return NULL;
    }

    tokens.pop_front();

    return value;
}

ASTNode *parseValueOrOperator(std::list<Token *> &tokens)
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

        int operatorImportance = getTokenOperatorImporance(tok);
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
            int topImportance = getTokenOperatorImporance(topOperator->operatorToken);
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

ASTDeclaration *parseConstDeclaration(std::list<Token *> &tokens)
{
    const Token *tok = tokens.front();
    if (tok == NULL)
    {
        std::cout << "ERROR: Unexpected end of file\n";
        return NULL;
    }
    if (tok->type != TokenType::CONST_KEYWORD)
    {
        std::cout << "ERROR: Constant must start with 'const'\n";
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

void parseFile(std::list<Token *> &tokens)
{
    std::list<ASTNode *> rootNodes;

    while (!tokens.empty())
    {
        const Token *tok = tokens.front();

        switch (tok->type)
        {
        case TokenType::FUNC_KEYWORD:
            rootNodes.push_front(parseFunction(tokens));
            break;
        case TokenType::CONST_KEYWORD:
            rootNodes.push_front(parseConstDeclaration(tokens));
            break;

        default:
            std::cout << "ERROR: Unexpected " << getTokenTypeName(tok->type) << " at " << tok->position << "\n";
            break;
        }
    }

    for (ASTNode *node : rootNodes)
    {
        if (node != NULL)
        {
            std::cout << "Node: " << node->toString() << "\n";
        }
        else
        {
            std::cout << "Node: NULL\n";
        }
    }
}

int main()
{
    std::string fileContent;
    std::getline(std::ifstream("test.cho"), fileContent, '\0');

    std::cout << "Tokenizing...\n";

    std::list<Token *> tokens;
    parseString(fileContent, tokens);

    for (const auto &token : tokens)
    {
        std::cout << getTokenTypeName(token->type) << " token at " << token->position << ", value = " << token->value << "\n";
    }

    std::cout << "Parsing...\n";
    parseFile(tokens);

    std::cout << "Done\n";
    return 0;
}
