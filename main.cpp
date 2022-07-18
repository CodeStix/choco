#include <iostream>
#include <string>
#include <fstream>
#include <list>

// Keywords, operators, string literal, number literal, comments

enum class TokenType
{
    SYMBOL,
    LITERAL_STRING,
    LITERAL_CHAR,
    LITERAL_NUMBER,
    BRACKET_OPEN,
    BRACKET_CLOSE,
    SQUARE_BRACKET_OPEN,
    SQUARE_BRACKET_CLOSE,
    CURLY_BRACKET_OPEN,
    CURLY_BRACKET_CLOSE,
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
                tokenList.push_back(new Token(i, TokenType::SYMBOL, currentString));
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

                default:
                    std::cout << "WARNING: Unknown char " << currentChar << "\n";
                    break;
                }
            }
        }
    }
}

int main()
{
    std::string fileContent;
    std::getline(std::ifstream("input.cho"), fileContent, '\0');

    std::cout << "Parsing...\n";

    std::list<Token *> tokens;
    parseString(fileContent, tokens);

    for (const auto &token : tokens)
    {
        std::cout << getTokenTypeName(token->type) << " token at " << token->position << ", value = " << token->value << "\n";
    }

    return 0;
}
