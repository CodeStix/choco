#include "token.hpp"

enum class TokenizeState
{
    NONE,
    PARSING_LITERAL_STRING,
    PARSING_LITERAL_CHAR,
    PARSING_LITERAL_NUMBER,
    PARSING_OPERATOR,
    PARSING_SYMBOL,
    PARSING_COMMENT,
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
            std::cout << "DEBUG: parse " << currentChar << " " << currentString.size() << " " << currentString << "\n";
            if (isalnum(currentChar) || currentChar == '.' || currentChar == '_')
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
        else if (state == TokenizeState::PARSING_OPERATOR)
        {
            char firstChar = currentString[0];
            if (firstChar == '=')
            {
                if (currentChar == '=')
                {
                    currentString += currentChar;
                    tokenList.push_back(new Token(i, TokenType::OPERATOR_EQUALS, currentString));
                    state = TokenizeState::NONE;
                    continue;
                }
                else
                {
                    tokenList.push_back(new Token(i, TokenType::OPERATOR_ASSIGNMENT, currentString));
                    state = TokenizeState::NONE;
                }
            }
            else if (firstChar == '/')
            {
                if (currentChar == '/')
                {
                    // Begin reading comment
                    currentString = "";
                    state = TokenizeState::PARSING_COMMENT;
                    continue;
                }
                else
                {
                    tokenList.push_back(new Token(i, TokenType::OPERATOR_DIVISION, currentString));
                    state = TokenizeState::NONE;
                }
            }
            else if (firstChar == '!')
            {
                if (currentChar == '=')
                {
                    currentString += currentChar;
                    tokenList.push_back(new Token(i, TokenType::OPERATOR_NOT_EQUALS, currentString));
                    state = TokenizeState::NONE;
                    continue;
                }
                else
                {
                    tokenList.push_back(new Token(i, TokenType::OPERATOR_NOT, currentString));
                    state = TokenizeState::NONE;
                }
            }
            else if (firstChar == '*')
            {
                tokenList.push_back(new Token(i, TokenType::OPERATOR_MULTIPLICATION, currentString));
                state = TokenizeState::NONE;
            }
            else if (firstChar == '+')
            {
                tokenList.push_back(new Token(i, TokenType::OPERATOR_ADDITION, currentString));
                state = TokenizeState::NONE;
            }
            else if (firstChar == '-')
            {
                tokenList.push_back(new Token(i, TokenType::OPERATOR_SUBSTRACTION, currentString));
                state = TokenizeState::NONE;
            }
            else if (firstChar == '>')
            {
                if (currentChar == '=')
                {
                    currentString += currentChar;
                    tokenList.push_back(new Token(i, TokenType::OPERATOR_GTE, currentString));
                    state = TokenizeState::NONE;
                    continue;
                }
                else if (currentChar == '>')
                {
                    currentString += currentChar;
                    tokenList.push_back(new Token(i, TokenType::OPERATOR_DOUBLE_GT, currentString));
                    state = TokenizeState::NONE;
                    continue;
                }
                else
                {
                    tokenList.push_back(new Token(i, TokenType::OPERATOR_GT, currentString));
                    state = TokenizeState::NONE;
                }
            }
            else if (firstChar == '<')
            {
                if (currentChar == '=')
                {
                    currentString += currentChar;
                    tokenList.push_back(new Token(i, TokenType::OPERATOR_LTE, currentString));
                    state = TokenizeState::NONE;
                    continue;
                }
                else if (currentChar == '<')
                {
                    currentString += currentChar;
                    tokenList.push_back(new Token(i, TokenType::OPERATOR_DOUBLE_LT, currentString));
                    state = TokenizeState::NONE;
                    continue;
                }
                else
                {
                    tokenList.push_back(new Token(i, TokenType::OPERATOR_LT, currentString));
                    state = TokenizeState::NONE;
                }
            }
            else if (firstChar == '|')
            {
                tokenList.push_back(new Token(i, TokenType::OPERATOR_OR, currentString));
                state = TokenizeState::NONE;
            }
            else if (firstChar == '&')
            {
                tokenList.push_back(new Token(i, TokenType::OPERATOR_AND, currentString));
                state = TokenizeState::NONE;
            }
            else if (firstChar == '~')
            {
                tokenList.push_back(new Token(i, TokenType::OPERATOR_TILDE, currentString));
                state = TokenizeState::NONE;
            }
            else if (firstChar == '^')
            {
                tokenList.push_back(new Token(i, TokenType::OPERATOR_XOR, currentString));
                state = TokenizeState::NONE;
            }
            else if (firstChar == '%')
            {
                tokenList.push_back(new Token(i, TokenType::OPERATOR_PERCENT, currentString));
                state = TokenizeState::NONE;
            }
            else
            {
                state = TokenizeState::NONE;
            }
        }
        else if (state == TokenizeState::PARSING_COMMENT)
        {
            if (currentChar == '\n')
            {
                std::cout << "DEBUG: parsed comment '" << currentString << "'\n";
                state = TokenizeState::NONE;
            }
            else
            {
                currentString += currentChar;
                continue;
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
                case '+':
                case '-':
                case '*':
                case '/':
                case '>':
                case '<':
                case '!':
                case '|':
                case '&':
                case '^':
                case '~':
                case '%':
                    state = TokenizeState::PARSING_OPERATOR;
                    currentString = std::string(1, currentChar);
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
    case TokenType::OPERATOR_ASSIGNMENT:
        return "OPERATOR_ASSIGNMENT";
    case TokenType::OPERATOR_ADDITION:
        return "OPERATOR_ADDITION";
    case TokenType::OPERATOR_SUBSTRACTION:
        return "OPERATOR_SUBSTRACTION";
    case TokenType::OPERATOR_DIVISION:
        return "OPERATOR_DIVISION";
    case TokenType::OPERATOR_MULTIPLICATION:
        return "OPERATOR_MULTIPLICATION";
    case TokenType::OPERATOR_LT:
        return "OPERATOR_LT";
    case TokenType::OPERATOR_GT:
        return "OPERATOR_GT";
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
    case TokenType::OPERATOR_NOT:
        return "OPERATOR_NOT";
    case TokenType::OPERATOR_AND:
        return "OPERATOR_AND";
    case TokenType::OPERATOR_OR:
        return "OPERATOR_OR";
    case TokenType::OPERATOR_XOR:
        return "OPERATOR_XOR";
    case TokenType::OPERATOR_TILDE:
        return "OPERATOR_XOR";
    case TokenType::OPERATOR_LTE:
        return "OPERATOR_LTE";
    case TokenType::OPERATOR_GTE:
        return "OPERATOR_GTE";
    case TokenType::OPERATOR_EQUALS:
        return "OPERATOR_EQUALS";
    case TokenType::OPERATOR_NOT_EQUALS:
        return "OPERATOR_NOT_EQUALS";
    case TokenType::OPERATOR_PERCENT:
        return "OPERATOR_PERCENT";
    case TokenType::OPERATOR_DOUBLE_LT:
        return "OPERATOR_DOUBLE_LT";
    case TokenType::OPERATOR_DOUBLE_GT:
        return "OPERATOR_DOUBLE_GT";
    default:
        return "Unknown";
    }
}

int getTokenOperatorImportance(TokenType type)
{
    // TODO add assignment as operator
    switch (type)
    {
    case TokenType::OPERATOR_AND:
    case TokenType::OPERATOR_OR:
    case TokenType::OPERATOR_XOR:
        return 1;
    case TokenType::OPERATOR_EQUALS:
    case TokenType::OPERATOR_NOT_EQUALS:
    case TokenType::OPERATOR_LTE:
    case TokenType::OPERATOR_GTE:
    case TokenType::OPERATOR_LT:
    case TokenType::OPERATOR_GT:
        return 2;
    case TokenType::OPERATOR_DOUBLE_LT:
    case TokenType::OPERATOR_DOUBLE_GT:
        return 3;
    case TokenType::OPERATOR_ADDITION:
    case TokenType::OPERATOR_SUBSTRACTION:
        return 4;
    case TokenType::OPERATOR_MULTIPLICATION:
    case TokenType::OPERATOR_DIVISION:
        return 5;
    case TokenType::COLON:
        return 6;

    default:
        return -1;
    }
}
