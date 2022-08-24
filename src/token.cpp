#include "token.hpp"

enum class TokenizeState
{
    NONE,
    PARSING_LITERAL_STRING,
    PARSING_LITERAL_CHAR,
    PARSING_LITERAL_NUMBER,
    PARSING_OPERATOR,
    PARSING_SYMBOL,
};

void parseString(std::string &input, std::list<const Token *> &tokenList)
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
            if (isalnum(currentChar) || currentChar == '_')
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
                else if (currentString == "extern")
                {
                    type = TokenType::EXTERN_KEYWORD;
                }
                else if (currentString == "if")
                {
                    type = TokenType::IF_KEYWORD;
                }
                else if (currentString == "else")
                {
                    type = TokenType::ELSE_KEYWORD;
                }
                else if (currentString == "for")
                {
                    type = TokenType::FOR_KEYWORD;
                }
                else if (currentString == "goto")
                {
                    type = TokenType::GOTO_KEYWORD;
                }
                else if (currentString == "while")
                {
                    type = TokenType::WHILE_KEYWORD;
                }
                else if (currentString == "export")
                {
                    type = TokenType::EXPORT_KEYWORD;
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
                case '>':
                    tokenList.push_back(new Token(i, TokenType::GT_OPERATOR, std::string(1, currentChar)));
                    break;
                case '<':
                    tokenList.push_back(new Token(i, TokenType::LT_OPERATOR, std::string(1, currentChar)));
                    break;
                case ',':
                    tokenList.push_back(new Token(i, TokenType::COMMA, std::string(1, currentChar)));
                    break;
                case ':':
                    tokenList.push_back(new Token(i, TokenType::COLON, std::string(1, currentChar)));
                    break;
                case ';':
                    tokenList.push_back(new Token(i, TokenType::SEMICOLON, std::string(1, currentChar)));
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
    case TokenType::EXTERN_KEYWORD:
        return "EXTERN_KEYWORD";
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
    case TokenType::LT_OPERATOR:
        return "LT_OPERATOR";
    case TokenType::GT_OPERATOR:
        return "GT_OPERATOR";
    case TokenType::COMMA:
        return "COMMA";
    case TokenType::IF_KEYWORD:
        return "IF_KEYWORD";
    case TokenType::ELSE_KEYWORD:
        return "ELSE_KEYWORD";
    case TokenType::FOR_KEYWORD:
        return "FOR_KEYWORD";
    case TokenType::GOTO_KEYWORD:
        return "GOTO_KEYWORD";
    case TokenType::WHILE_KEYWORD:
        return "WHILE_KEYWORD";
    case TokenType::EXPORT_KEYWORD:
        return "EXPORT_KEYWORD";
    case TokenType::COLON:
        return "COLON";
    case TokenType::SEMICOLON:
        return "SEMICOLON";
    default:
        return "Unknown";
    }
}

int getTokenOperatorImportance(TokenType type)
{
    // TODO add assignment as operator
    switch (type)
    {
    case TokenType::LT_OPERATOR:
    case TokenType::GT_OPERATOR:
        return 1;
    case TokenType::ADDITION_OPERATOR:
    case TokenType::SUBSTRACTION_OPERATOR:
        return 2;
    case TokenType::MULTIPLICATION_OPERATOR:
    case TokenType::DIVISION_OPERATOR:
        return 3;

    default:
        return -1;
    }
}
