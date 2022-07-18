#include <iostream>
#include <string>
#include <fstream>
#include <list>
#include "token.hpp"
#include "ast.hpp"

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
