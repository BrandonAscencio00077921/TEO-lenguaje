#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void parser_advance(Parser *parser);
static ASTNode *parse_instruction_list(Parser *parser, bool stop_on_rbrace);
static ASTNode *parse_instruction(Parser *parser);
static ASTNode *parse_declaration(Parser *parser);
static ASTNode *parse_array_declaration(Parser *parser);
static ASTNode *parse_assignment(Parser *parser);
static ASTNode *parse_if(Parser *parser);
static ASTNode *parse_for(Parser *parser);
static ASTNode *parse_while(Parser *parser);
static ASTNode *parse_function(Parser *parser);
static ASTNode *parse_return(Parser *parser);
static ASTNode *parse_expression(Parser *parser);
static ASTNode *parse_call(Parser *parser, ASTNode *callee);
static ASTNode *parse_call_statement(Parser *parser);
static ASTNode *parse_special_call(Parser *parser, Token keyword);
static ASTNode *parse_expression_statement(Parser *parser);
static ASTNode *parse_array_literal(Parser *parser);

static void parser_error(Parser *parser, Token token, const char *message) {
    if (parser->had_error) {
        return;
    }
    parser->had_error = true;
    parser->error_token = token;
    snprintf(parser->error_message, sizeof(parser->error_message), "%s", message);
}

void parser_init(Parser *parser, const char *source, size_t length) {
    lexer_init(&parser->lexer, source, length);
    parser->current = lexer_next_token(&parser->lexer);
    parser->next = lexer_next_token(&parser->lexer);
    parser->had_error = false;
    parser->error_message[0] = '\0';
    parser->error_token = parser->current;
}

static void parser_advance(Parser *parser) {
    parser->current = parser->next;
    parser->next = lexer_next_token(&parser->lexer);
}

static bool parser_check(const Parser *parser, TokenType type) {
    return parser->current.type == type;
}

static Token parser_consume(Parser *parser, TokenType type, const char *message) {
    Token token = parser->current;
    if (parser->current.type == type) {
        parser_advance(parser);
        return token;
    }
    parser_error(parser, parser->current, message);
    return token;
}

static bool parser_match(Parser *parser, TokenType type) {
    if (parser_check(parser, type)) {
        parser_advance(parser);
        return true;
    }
    return false;
}

static ASTNode *parse_identifier_node(Parser *parser) {
    Token token = parser_consume(parser, TOKEN_IDENTIFIER, "Se esperaba un identificador.");
    if (parser->had_error) {
        return NULL;
    }
    return ast_create(AST_IDENTIFIER, token);
}

ASTNode *parser_parse(Parser *parser) {
    ASTNode *program = ast_create(AST_PROGRAM, parser->current);
    ASTNode *instructions = parse_instruction_list(parser, false);
    if (!instructions) {
        ast_free(program);
        return NULL;
    }
    ast_add_child(program, instructions);
    if (!parser->had_error && parser->current.type != TOKEN_EOF) {
        parser_error(parser, parser->current, "Fin inesperado del programa.");
        ast_free(program);
        return NULL;
    }
    return parser->had_error ? NULL : program;
}

bool parser_has_error(const Parser *parser) {
    return parser->had_error;
}

const char *parser_error_message(const Parser *parser) {
    return parser->error_message;
}

Token parser_error_token(const Parser *parser) {
    return parser->error_token;
}

static ASTNode *parse_instruction_list(Parser *parser, bool stop_on_rbrace) {
    ASTNode *list = ast_create(AST_INSTRUCTION_LIST, parser->current);
    while (!parser_check(parser, TOKEN_EOF)) {
        if (stop_on_rbrace && parser_check(parser, TOKEN_RBRACE)) {
            break;
        }
        ASTNode *instr = parse_instruction(parser);
        if (!instr) {
            ast_free(list);
            return NULL;
        }
        ast_add_child(list, instr);
    }
    return list;
}

static ASTNode *parse_instruction(Parser *parser) {
    switch (parser->current.type) {
        case TOKEN_KW_INT:
        case TOKEN_KW_FLOAT:
        case TOKEN_KW_CHAR:
        case TOKEN_KW_BOOL:
            return parse_declaration(parser);
        case TOKEN_KW_ARRAY:
            return parse_array_declaration(parser);
        case TOKEN_KW_IF:
            return parse_if(parser);
        case TOKEN_KW_FOR:
            return parse_for(parser);
        case TOKEN_KW_WHILE:
            return parse_while(parser);
        case TOKEN_KW_FUNC:
            return parse_function(parser);
        case TOKEN_KW_RETURN:
            return parse_return(parser);
        case TOKEN_KW_CSAY:
        case TOKEN_KW_CREAD:
            return parse_special_call(parser, parser->current);
        case TOKEN_IDENTIFIER:
            if (parser->next.type == TOKEN_LPAREN) {
                return parse_call_statement(parser);
            }
            return parse_assignment(parser);
        default:
            return parse_expression_statement(parser);
    }
}

static ASTNode *parse_declaration(Parser *parser) {
    Token type_token = parser->current;
    parser_advance(parser);
    ASTNode *node = ast_create(AST_DECLARATION, type_token);
    ASTNode *identifier = parse_identifier_node(parser);
    if (!identifier) {
        ast_free(node);
        return NULL;
    }
    parser_consume(parser, TOKEN_EQ, "Se esperaba '=' en la declaración.");
    if (parser->had_error) {
        ast_free(node);
        ast_free(identifier);
        return NULL;
    }
    ASTNode *expr = parse_expression(parser);
    parser_consume(parser, TOKEN_SEMICOLON, "Se esperaba ';' al final de la declaración.");
    if (parser->had_error || !expr) {
        ast_free(node);
        ast_free(identifier);
        ast_free(expr);
        return NULL;
    }
    ast_add_child(node, identifier);
    ast_add_child(node, expr);
    return node;
}

static ASTNode *parse_array_declaration(Parser *parser) {
    Token array_token = parser->current;
    parser_advance(parser);
    ASTNode *node = ast_create(AST_DECLARATION, array_token);
    ASTNode *identifier = parse_identifier_node(parser);
    parser_consume(parser, TOKEN_EQ, "Se esperaba '=' en la declaración de arreglo.");
    ASTNode *array_literal = parse_array_literal(parser);
    parser_consume(parser, TOKEN_SEMICOLON, "Se esperaba ';' tras la declaración de arreglo.");
    if (parser->had_error || !identifier || !array_literal) {
        ast_free(node);
        ast_free(identifier);
        ast_free(array_literal);
        return NULL;
    }
    ast_add_child(node, identifier);
    ast_add_child(node, array_literal);
    return node;
}

static ASTNode *parse_assignment(Parser *parser) {
    ASTNode *identifier = parse_identifier_node(parser);
    parser_consume(parser, TOKEN_EQ, "Se esperaba '=' en la asignación.");
    ASTNode *expr = parse_expression(parser);
    parser_consume(parser, TOKEN_SEMICOLON, "Se esperaba ';' tras la asignación.");
    if (parser->had_error || !identifier || !expr) {
        ast_free(identifier);
        ast_free(expr);
        return NULL;
    }
    ASTNode *node = ast_create(AST_ASSIGNMENT, identifier->token);
    ast_add_child(node, identifier);
    ast_add_child(node, expr);
    return node;
}

static ASTNode *parse_if(Parser *parser) {
    Token if_token = parser->current;
    parser_advance(parser);
    parser_consume(parser, TOKEN_LPAREN, "Se esperaba '(' en la condición del if.");
    ASTNode *condition = parse_expression(parser);
    parser_consume(parser, TOKEN_RPAREN, "Se esperaba ')' tras la condición del if.");
    parser_consume(parser, TOKEN_LBRACE, "Se esperaba '{' para iniciar el bloque del if.");
    ASTNode *body = parse_instruction_list(parser, true);
    parser_consume(parser, TOKEN_RBRACE, "Se esperaba '}' al cerrar el bloque del if.");
    if (parser->had_error || !condition || !body) {
        ast_free(condition);
        ast_free(body);
        return NULL;
    }
    ASTNode *node = ast_create(AST_IF, if_token);
    ast_add_child(node, condition);
    ast_add_child(node, body);
    return node;
}

static ASTNode *parse_for(Parser *parser) {
    Token for_token = parser->current;
    parser_advance(parser);
    parser_consume(parser, TOKEN_LPAREN, "Se esperaba '(' en el for.");
    ASTNode *iterator = parse_identifier_node(parser);
    parser_consume(parser, TOKEN_KW_IN, "Se esperaba la palabra clave 'in'.");
    ASTNode *iterable = parse_identifier_node(parser);
    parser_consume(parser, TOKEN_RPAREN, "Se esperaba ')' tras la cabecera del for.");
    parser_consume(parser, TOKEN_LBRACE, "Se esperaba '{' para el cuerpo del for.");
    ASTNode *body = parse_instruction_list(parser, true);
    parser_consume(parser, TOKEN_RBRACE, "Se esperaba '}' al cerrar el for.");
    if (parser->had_error || !iterator || !iterable || !body) {
        ast_free(iterator);
        ast_free(iterable);
        ast_free(body);
        return NULL;
    }
    ASTNode *node = ast_create(AST_FOR, for_token);
    ast_add_child(node, iterator);
    ast_add_child(node, iterable);
    ast_add_child(node, body);
    return node;
}

static ASTNode *parse_while(Parser *parser) {
    Token while_token = parser->current;
    parser_advance(parser);
    parser_consume(parser, TOKEN_LPAREN, "Se esperaba '(' en el while.");
    ASTNode *condition = parse_expression(parser);
    parser_consume(parser, TOKEN_RPAREN, "Se esperaba ')' tras la condición del while.");
    parser_consume(parser, TOKEN_LBRACE, "Se esperaba '{' para el cuerpo del while.");
    ASTNode *body = parse_instruction_list(parser, true);
    parser_consume(parser, TOKEN_RBRACE, "Se esperaba '}' al cerrar el while.");
    if (parser->had_error || !condition || !body) {
        ast_free(condition);
        ast_free(body);
        return NULL;
    }
    ASTNode *node = ast_create(AST_WHILE, while_token);
    ast_add_child(node, condition);
    ast_add_child(node, body);
    return node;
}

static ASTNode *parse_parameter_list(Parser *parser) {
    ASTNode *params = ast_create(AST_PARAM_LIST, parser->current);
    if (parser->current.type == TOKEN_RPAREN) {
        return params;
    }
    while (true) {
        ASTNode *param = parse_identifier_node(parser);
        if (!param) {
            ast_free(params);
            return NULL;
        }
        ast_add_child(params, param);
        if (!parser_match(parser, TOKEN_COMMA)) {
            break;
        }
    }
    return params;
}

static ASTNode *parse_function(Parser *parser) {
    Token func_token = parser->current;
    parser_advance(parser);
    ASTNode *name = parse_identifier_node(parser);
    parser_consume(parser, TOKEN_LPAREN, "Se esperaba '(' tras el nombre de la función.");
    ASTNode *params = parse_parameter_list(parser);
    parser_consume(parser, TOKEN_RPAREN, "Se esperaba ')' tras los parámetros.");
    parser_consume(parser, TOKEN_LBRACE, "Se esperaba '{' para el cuerpo de la función.");
    ASTNode *body = parse_instruction_list(parser, true);
    ASTNode *maybe_return = NULL;
    if (parser_check(parser, TOKEN_KW_RETURN)) {
        maybe_return = parse_return(parser);
    }
    parser_consume(parser, TOKEN_RBRACE, "Se esperaba '}' al cerrar la función.");
    if (parser->had_error || !name || !params || !body) {
        ast_free(name);
        ast_free(params);
        ast_free(body);
        ast_free(maybe_return);
        return NULL;
    }
    ASTNode *node = ast_create(AST_FUNCTION, func_token);
    ast_add_child(node, name);
    ast_add_child(node, params);
    ast_add_child(node, body);
    if (maybe_return) {
        ast_add_child(node, maybe_return);
    }
    return node;
}

static ASTNode *parse_return(Parser *parser) {
    Token return_token = parser->current;
    parser_advance(parser);
    ASTNode *expr = parse_expression(parser);
    parser_consume(parser, TOKEN_SEMICOLON, "Se esperaba ';' tras return.");
    if (parser->had_error || !expr) {
        ast_free(expr);
        return NULL;
    }
    ASTNode *node = ast_create(AST_RETURN, return_token);
    ast_add_child(node, expr);
    return node;
}

static ASTNode *parse_argument_list(Parser *parser) {
    ASTNode *args = ast_create(AST_ARG_LIST, parser->current);
    if (parser_check(parser, TOKEN_RPAREN)) {
        return args;
    }
    while (true) {
        ASTNode *expr = parse_expression(parser);
        if (!expr) {
            ast_free(args);
            return NULL;
        }
        ast_add_child(args, expr);
        if (!parser_match(parser, TOKEN_COMMA)) {
            break;
        }
    }
    return args;
}

static ASTNode *parse_call(Parser *parser, ASTNode *callee) {
    Token call_token = parser->current;
    parser_consume(parser, TOKEN_LPAREN, "Se esperaba '(' en la llamada.");
    ASTNode *args = parse_argument_list(parser);
    parser_consume(parser, TOKEN_RPAREN, "Se esperaba ')' al cerrar la llamada.");
    if (parser->had_error || !callee || !args) {
        ast_free(callee);
        ast_free(args);
        return NULL;
    }
    ASTNode *call = ast_create(AST_CALL, call_token);
    ast_add_child(call, callee);
    ast_add_child(call, args);
    return call;
}

static ASTNode *parse_call_statement(Parser *parser) {
    ASTNode *callee = parse_identifier_node(parser);
    ASTNode *call = parse_call(parser, callee);
    parser_consume(parser, TOKEN_SEMICOLON, "Se esperaba ';' tras la llamada.");
    return call;
}

static ASTNode *parse_special_call(Parser *parser, Token keyword) {
    parser_advance(parser);
    parser_consume(parser, TOKEN_LPAREN, "Se esperaba '(' tras llamada especial.");
    ASTNode *args = parse_argument_list(parser);
    parser_consume(parser, TOKEN_RPAREN, "Se esperaba ')' en la llamada especial.");
    ASTNode *call = ast_create(AST_CALL, keyword);
    ast_add_child(call, args);
    if (keyword.type == TOKEN_KW_CREAD) {
        ASTNode *destination = parse_identifier_node(parser);
        ast_add_child(call, destination);
    }
    parser_consume(parser, TOKEN_SEMICOLON, "Se esperaba ';' tras la llamada especial.");
    return call;
}

static ASTNode *parse_expression_statement(Parser *parser) {
    ASTNode *expr = parse_expression(parser);
    parser_consume(parser, TOKEN_SEMICOLON, "Se esperaba ';' tras la expresión.");
    if (parser->had_error || !expr) {
        ast_free(expr);
        return NULL;
    }
    ASTNode *wrapper = ast_create(AST_EXPRESSION, expr->token);
    ast_add_child(wrapper, expr);
    return wrapper;
}

static ASTNode *parse_primary(Parser *parser);
static ASTNode *parse_unary(Parser *parser);
static ASTNode *parse_mul(Parser *parser);
static ASTNode *parse_add(Parser *parser);
static ASTNode *parse_rel(Parser *parser);
static ASTNode *parse_eq(Parser *parser);
static ASTNode *parse_and(Parser *parser);
static ASTNode *parse_or(Parser *parser);

static ASTNode *parse_expression(Parser *parser) {
    return parse_or(parser);
}

static ASTNode *parse_or(Parser *parser) {
    ASTNode *left = parse_and(parser);
    while (parser_check(parser, TOKEN_OROR)) {
        Token op = parser->current;
        parser_advance(parser);
        ASTNode *right = parse_and(parser);
        ASTNode *node = ast_create(AST_EXPRESSION, op);
        ast_add_child(node, left);
        ast_add_child(node, right);
        left = node;
    }
    return left;
}

static ASTNode *parse_and(Parser *parser) {
    ASTNode *left = parse_eq(parser);
    while (parser_check(parser, TOKEN_ANDAND)) {
        Token op = parser->current;
        parser_advance(parser);
        ASTNode *right = parse_eq(parser);
        ASTNode *node = ast_create(AST_EXPRESSION, op);
        ast_add_child(node, left);
        ast_add_child(node, right);
        left = node;
    }
    return left;
}

static ASTNode *parse_eq(Parser *parser) {
    ASTNode *left = parse_rel(parser);
    while (parser_check(parser, TOKEN_EQEQ) || parser_check(parser, TOKEN_BANGEQ)) {
        Token op = parser->current;
        parser_advance(parser);
        ASTNode *right = parse_rel(parser);
        ASTNode *node = ast_create(AST_EXPRESSION, op);
        ast_add_child(node, left);
        ast_add_child(node, right);
        left = node;
    }
    return left;
}

static ASTNode *parse_rel(Parser *parser) {
    ASTNode *left = parse_add(parser);
    while (parser_check(parser, TOKEN_LT) || parser_check(parser, TOKEN_LTE) ||
           parser_check(parser, TOKEN_GT) || parser_check(parser, TOKEN_GTE)) {
        Token op = parser->current;
        parser_advance(parser);
        ASTNode *right = parse_add(parser);
        ASTNode *node = ast_create(AST_EXPRESSION, op);
        ast_add_child(node, left);
        ast_add_child(node, right);
        left = node;
    }
    return left;
}

static ASTNode *parse_add(Parser *parser) {
    ASTNode *left = parse_mul(parser);
    while (parser_check(parser, TOKEN_PLUS) || parser_check(parser, TOKEN_MINUS)) {
        Token op = parser->current;
        parser_advance(parser);
        ASTNode *right = parse_mul(parser);
        ASTNode *node = ast_create(AST_EXPRESSION, op);
        ast_add_child(node, left);
        ast_add_child(node, right);
        left = node;
    }
    return left;
}

static ASTNode *parse_mul(Parser *parser) {
    ASTNode *left = parse_unary(parser);
    while (parser_check(parser, TOKEN_STAR) || parser_check(parser, TOKEN_SLASH) || parser_check(parser, TOKEN_PERCENT)) {
        Token op = parser->current;
        parser_advance(parser);
        ASTNode *right = parse_unary(parser);
        ASTNode *node = ast_create(AST_EXPRESSION, op);
        ast_add_child(node, left);
        ast_add_child(node, right);
        left = node;
    }
    return left;
}

static ASTNode *parse_unary(Parser *parser) {
    if (parser_check(parser, TOKEN_MINUS) || parser_check(parser, TOKEN_BANG) ||
        parser_check(parser, TOKEN_PLUSPLUS) || parser_check(parser, TOKEN_MINUSMINUS)) {
        Token op = parser->current;
        parser_advance(parser);
        ASTNode *expr = parse_unary(parser);
        ASTNode *node = ast_create(AST_EXPRESSION, op);
        ast_add_child(node, expr);
        return node;
    }
    return parse_primary(parser);
}

static ASTNode *parse_primary(Parser *parser) {
    Token token = parser->current;
    switch (token.type) {
        case TOKEN_NUMBER:
        case TOKEN_STRING:
        case TOKEN_CHAR:
        case TOKEN_TRUE:
        case TOKEN_FALSE: {
            parser_advance(parser);
            ASTNode *literal = ast_create(AST_LITERAL, token);
            return literal;
        }
        case TOKEN_IDENTIFIER: {
            ASTNode *identifier = parse_identifier_node(parser);
            if (parser->current.type == TOKEN_LPAREN) {
                return parse_call(parser, identifier);
            }
            return identifier;
        }
        case TOKEN_LPAREN: {
            parser_advance(parser);
            ASTNode *expr = parse_expression(parser);
            parser_consume(parser, TOKEN_RPAREN, "Se esperaba ')' tras la expresión.");
            return expr;
        }
        case TOKEN_LBRACKET:
            return parse_array_literal(parser);
        default:
            parser_error(parser, token, "Expresión primaria inválida.");
            return NULL;
    }
}

static ASTNode *parse_array_literal(Parser *parser) {
    Token bracket = parser_consume(parser, TOKEN_LBRACKET, "Se esperaba '['.");
    ASTNode *array = ast_create(AST_ARRAY_LITERAL, bracket);
    if (!parser_check(parser, TOKEN_RBRACKET)) {
        while (true) {
            ASTNode *value = parse_expression(parser);
            if (!value) {
                ast_free(array);
                return NULL;
            }
            ast_add_child(array, value);
            if (!parser_match(parser, TOKEN_COMMA)) {
                break;
            }
        }
    }
    parser_consume(parser, TOKEN_RBRACKET, "Se esperaba ']' tras el arreglo.");
    return array;
}

