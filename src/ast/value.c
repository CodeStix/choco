#include "ast/value.h"
#include "ast.h"
#include "ast/modifiers.h"
#include "ast/operator.h"
#include "common/list.h"
#include "common/map.h"
#include <assert.h>
#include <stdio.h>

struct ASTBrackets {
    ASTNode* value;
};

struct ASTLiteralNumber {
    Token* value;
    ASTNode* modifiers;
};

struct ASTLiteralString {
    Token* value;
    ASTNode* modifiers;
};

struct ASTObjectField {
    Token* name;
    ASTNode* value_or_type;
};

struct ASTObject {
    List* fields;   // array of ASTObjectField*
    ASTNode* modifiers;
};

struct ASTArraySegment {
    ASTNode* value_or_type;
    ASTNode* times_value;   // can be null, then times == 1
};

struct ASTArray {
    List* segments;   // array of ASTArraySegment*
    ASTNode* modifiers;
};

struct ASTSymbol {
    Token* name;
    ASTNode* modifiers;
};

ASTBrackets* ast_brackets_malloc(ASTNode* value) {
    ASTBrackets* b = (ASTBrackets*)malloc(sizeof(ASTBrackets));
    b->value = value;
    return b;
}

ASTLiteralNumber* ast_literal_number_malloc(Token* value, ASTNode* modifiers) {
    ASTLiteralNumber* b = (ASTLiteralNumber*)malloc(sizeof(ASTLiteralNumber));
    b->value = value;
    b->modifiers = modifiers;
    return b;
}

ASTLiteralString* ast_literal_string_malloc(Token* value, ASTNode* modifiers) {
    ASTLiteralString* b = (ASTLiteralString*)malloc(sizeof(ASTLiteralString));
    b->value = value;
    b->modifiers = modifiers;
    return b;
}

ASTSymbol* ast_symbol_malloc(Token* name, ASTNode* modifiers) {
    ASTSymbol* b = (ASTSymbol*)malloc(sizeof(ASTSymbol));
    b->name = name;
    b->modifiers = modifiers;
    return b;
}

ASTObject* ast_object_malloc(List* fields, ASTNode* modifiers) {
    ASTObject* obj = (ASTObject*)malloc(sizeof(ASTObject));
    obj->fields = fields;
    obj->modifiers = modifiers;
    return obj;
}

ASTObjectField* ast_object_field_malloc(Token* name, ASTNode* value_or_type) {
    ASTObjectField* obj = (ASTObjectField*)malloc(sizeof(ASTObjectField));
    obj->name = name;
    obj->value_or_type = value_or_type;
    return obj;
}

ASTArray* ast_array_malloc(List* segments, ASTNode* modifiers) {
    ASTArray* obj = (ASTArray*)malloc(sizeof(ASTArray));
    obj->segments = segments;
    obj->modifiers = modifiers;
    return obj;
}

ASTArraySegment* ast_array_segment_malloc(ASTNode* times_value, ASTNode* value_or_type) {
    ASTArraySegment* obj = (ASTArraySegment*)malloc(sizeof(ASTArraySegment));
    obj->times_value = times_value;
    obj->value_or_type = value_or_type;
    return obj;
}

ASTNode* ast_parse_symbol(List* tokens, unsigned int* i, ASTNode* modifiers) {
    unsigned int start_token = *i;

    Token* tok = peek(tokens, i);
    assert(token_type(tok) == TOKEN_SYMBOL);

    next(tokens, i);
    consume(tokens, i, TOKEN_WHITESPACE);
    return ast_node_malloc(AST_SYMBOL, ast_symbol_malloc(tok, modifiers), start_token, *i);
}

ASTNode* ast_parse_object_value(List* tokens, unsigned int* i, ASTNode* modifiers) {
    unsigned int start_token = *i;

    Token* tok = peek(tokens, i);
    assert(token_type(tok) == TOKEN_CURLY_BRACKET_OPEN);

    next(tokens, i);
    consume(tokens, i, TOKEN_WHITESPACE);

    List* fields = list_malloc(4);

    while (1) {
        tok = peek(tokens, i);

        if (token_type(tok) == TOKEN_CURLY_BRACKET_CLOSE) {
            next(tokens, i);
            consume(tokens, i, TOKEN_WHITESPACE);
            break;
        }

        unsigned int field_start_token = *i;

        Token* field_name_tok = NULL;
        if (token_type(tok) == TOKEN_SYMBOL) {
            field_name_tok = tok;
        } else {
            printf("Expected object field name after {\n");
            token_highlight(tok, tok);
            assert(false);
            return NULL;
        }

        next(tokens, i);
        consume(tokens, i, TOKEN_WHITESPACE);
        tok = peek(tokens, i);

        if (token_type(tok) != TOKEN_COLON) {
            printf("Expected : after object field name\n");
            token_highlight(tok, tok);
            assert(false);
            return NULL;
        }

        next(tokens, i);
        consume(tokens, i, TOKEN_WHITESPACE);

        ASTNode* field_value_or_type = ast_parse_value(tokens, i);
        assert(field_value_or_type != NULL);

        list_add(fields, ast_node_malloc(AST_OBJECT_FIELD, ast_object_field_malloc(field_name_tok, field_value_or_type),
                                         field_start_token, *i));

        tok = peek(tokens, i);
        if (token_type(tok) != TOKEN_COMMA) {
            if (token_type(tok) != TOKEN_CURLY_BRACKET_CLOSE) {
                printf("Expected , or } after object field\n");
                token_highlight(tok, tok);
                assert(false);
                return NULL;
            }
        } else {
            next(tokens, i);
            consume(tokens, i, TOKEN_WHITESPACE);
        }
    }

    return ast_node_malloc(AST_OBJECT, ast_object_malloc(fields, modifiers), start_token, *i);
}

ASTNode* ast_parse_array_value(List* tokens, unsigned int* i, ASTNode* modifiers) {
    unsigned int start_token = *i;

    Token* tok = peek(tokens, i);
    assert(token_type(tok) == TOKEN_SQUARE_BRACKET_OPEN);

    next(tokens, i);
    consume(tokens, i, TOKEN_WHITESPACE);

    List* segments = list_malloc(4);

    while (1) {
        tok = peek(tokens, i);

        if (token_type(tok) == TOKEN_SQUARE_BRACKET_CLOSE) {
            next(tokens, i);
            consume(tokens, i, TOKEN_WHITESPACE);
            break;
        }

        unsigned int segment_start_token = *i;

        ASTNode* field_value_or_type = ast_parse_value(tokens, i);
        assert(field_value_or_type != NULL);

        tok = peek(tokens, i);
        ASTNode* times_value = NULL;
        if (token_type(tok) == TOKEN_HASHTAG) {
            next(tokens, i);
            consume(tokens, i, TOKEN_WHITESPACE);

            times_value = ast_parse_value(tokens, i);
            assert(times_value != NULL);
        }

        list_add(segments,
                 ast_node_malloc(AST_ARRAY_SEGMENT, ast_array_segment_malloc(times_value, field_value_or_type),
                                 segment_start_token, *i));

        tok = peek(tokens, i);
        if (token_type(tok) != TOKEN_COMMA) {
            if (token_type(tok) != TOKEN_SQUARE_BRACKET_CLOSE) {
                printf("Expected , or ] after array item\n");
                token_highlight(tok, tok);
                assert(false);
                return NULL;
            }
        } else {
            next(tokens, i);
            consume(tokens, i, TOKEN_WHITESPACE);
        }
    }

    return ast_node_malloc(AST_ARRAY, ast_array_malloc(segments, modifiers), start_token, *i);
}

ASTNode* ast_parse_value(List* tokens, unsigned int* i) {
    unsigned int start_token = *i;

    ASTNode* modifiers = ast_parse_modifiers(tokens, i);

    Token* tok = peek(tokens, i);

    switch (token_type(tok)) {
    case TOKEN_LITERAL_STRING:
        next(tokens, i);
        consume(tokens, i, TOKEN_WHITESPACE);
        return ast_node_malloc(AST_LITERAL_STRING, ast_literal_string_malloc(tok, modifiers), start_token, *i);
    case TOKEN_LITERAL_NUMBER:
        next(tokens, i);
        consume(tokens, i, TOKEN_WHITESPACE);
        return ast_node_malloc(AST_LITERAL_NUMBER, ast_literal_number_malloc(tok, modifiers), start_token, *i);
    case TOKEN_SYMBOL:
        return ast_parse_symbol(tokens, i, modifiers);
    case TOKEN_BRACKET_OPEN:
        return ast_parse_brackets(tokens, i);
    case TOKEN_SQUARE_BRACKET_OPEN:
        return ast_parse_array_value(tokens, i, modifiers);
    case TOKEN_CURLY_BRACKET_OPEN:
        return ast_parse_object_value(tokens, i, modifiers);

    default:
        // This isn't a value
        *i = start_token;
        return NULL;
    }
}

ASTNode* ast_parse_brackets(List* tokens, unsigned int* i) {
    unsigned int start_token = *i;

    Token* open_tok = peek(tokens, i);
    assert(token_type(open_tok) == TOKEN_BRACKET_OPEN);
    next(tokens, i);
    consume(tokens, i, TOKEN_WHITESPACE);

    ASTNode* value = ast_parse_value_or_operator(tokens, i);

    Token* close_tok = peek(tokens, i);
    if (token_type(close_tok) != TOKEN_BRACKET_CLOSE) {
        printf("Expected closing ) for brackets\n");
        token_highlight(open_tok, close_tok);
        assert(false);
        return NULL;
    }

    return ast_node_malloc(AST_BRACKETS, ast_brackets_malloc(value), start_token, *i);
}

void ast_symbol_print(ASTNode* node, bool verbose, unsigned int indent) {
    ASTSymbol* br = (ASTSymbol*)ast_node_data(node);

    unsigned int tok_len = 0;
    char* tok_value = token_value(br->name, &tok_len);

    printf("%*sSymbol { name=%.*s }\n", indent, "", tok_len, tok_value);

    if (br->modifiers != NULL) {
        ast_node_print(br->modifiers, true, indent + 2);
    }
}

void ast_brackets_print(ASTNode* node, bool verbose, unsigned int indent) {
    ASTBrackets* br = (ASTBrackets*)ast_node_data(node);

    printf("%*sBrackets { }\n", indent, "");

    if (verbose) {
        ast_node_print(br->value, true, indent + 2);
    }
}

void ast_literal_number_print(ASTNode* node, bool verbose, unsigned int indent) {
    ASTLiteralNumber* num = (ASTLiteralNumber*)ast_node_data(node);

    unsigned int tok_len = 0;
    char* tok_value = token_value(num->value, &tok_len);

    printf("%*sLiteralNumber { value=%.*s }\n", indent, "", tok_len, tok_value);

    if (num->modifiers != NULL) {
        ast_node_print(num->modifiers, true, indent + 2);
    }
}

void ast_literal_string_print(ASTNode* node, bool verbose, unsigned int indent) {
    ASTLiteralString* num = (ASTLiteralString*)ast_node_data(node);

    unsigned int tok_len = 0;
    char* tok_value = token_value(num->value, &tok_len);

    printf("%*sASTLiteralString { value=\"%.*s\" }\n", indent, "", tok_len, tok_value);

    if (num->modifiers != NULL) {
        ast_node_print(num->modifiers, true, indent + 2);
    }
}

void ast_object_print(ASTNode* node, bool verbose, unsigned int indent) {
    ASTObject* br = (ASTObject*)ast_node_data(node);

    printf("%*sObject { fields=%u }\n", indent, "", list_length(br->fields));

    if (br->modifiers != NULL) {
        ast_node_print(br->modifiers, true, indent + 2);
    }

    for (int i = 0; i < list_length(br->fields); i++) {
        ast_node_print(list_get(br->fields, i), true, indent + 2);
    }
}

void ast_object_field_print(ASTNode* node, bool verbose, unsigned int indent) {
    ASTObjectField* br = (ASTObjectField*)ast_node_data(node);

    unsigned int tok_len = 0;
    char* tok_value = token_value(br->name, &tok_len);

    printf("%*sObjectField { name=%.*s }\n", indent, "", tok_len, tok_value);

    if (verbose) {
        ast_node_print(br->value_or_type, true, indent + 2);
    }
}

void ast_array_print(ASTNode* node, bool verbose, unsigned int indent) {
    ASTArray* br = (ASTArray*)ast_node_data(node);

    printf("%*sArray { segments=%u }\n", indent, "", list_length(br->segments));

    if (br->modifiers != NULL) {
        ast_node_print(br->modifiers, true, indent + 2);
    }

    for (int i = 0; i < list_length(br->segments); i++) {
        ast_node_print(list_get(br->segments, i), true, indent + 2);
    }
}

void ast_array_segment_print(ASTNode* node, bool verbose, unsigned int indent) {
    ASTArraySegment* br = (ASTArraySegment*)ast_node_data(node);

    printf("%*sArraySegment { has_times=%s }\n", indent, "", br->times_value != NULL ? "yes" : "no");

    if (verbose) {
        if (br->times_value == NULL) {
            ast_node_print(br->value_or_type, true, indent + 2);
        } else {
            printf("%*sValue:\n", indent + 2, "");
            ast_node_print(br->value_or_type, true, indent + 4);
            printf("%*sTimes:\n", indent + 2, "");
            ast_node_print(br->times_value, true, indent + 4);
        }
    }
}