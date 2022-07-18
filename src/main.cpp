#include <iostream>
#include <string>
#include <fstream>
#include <list>
#include "token.hpp"
#include "ast.hpp"

void parseFile(std::list<const Token *> &tokens)
{
    std::list<ASTNode *> rootNodes;

    while (!tokens.empty())
    {
        const Token *tok = tokens.front();

        ASTNode *statement = NULL;
        switch (tok->type)
        {
        case TokenType::FUNC_KEYWORD:
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
            std::cout << "ERROR: Invalid statement, unexpected " << getTokenTypeName(tok->type) << " at " << tok->position << "\n";
        }
        else
        {
            rootNodes.push_back(statement);
        }
    }

    for (ASTNode *node : rootNodes)
    {
        std::cout << node->toString() << "\n";
    }
}

int main()
{
    std::string fileContent;
    std::getline(std::ifstream("test.cho"), fileContent, '\0');

    std::cout << "Tokenizing...\n";

    std::list<const Token *> tokens;
    parseString(fileContent, tokens);

    // for (const auto &token : tokens)
    // {
    //     std::cout << getTokenTypeName(token->type) << " token at " << token->position << ", value = " << token->value << "\n";
    // }

    std::cout << "Parsing...\n";
    parseFile(tokens);

    std::cout << "Done\n";
    return 0;
}
