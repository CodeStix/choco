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

int getTokenOperatorImportance(TokenType type)
{
    switch (type)
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
