 #ifndef PYCLITE_PARSER_H
 #define PYCLITE_PARSER_H

#include "ast/ast.h"
#include "lexer/lexer.h"

 #include <stdbool.h>

 typedef struct {
     Lexer lexer;
     Token current;
     Token next;
     bool has_next;
 
     bool had_error;
     char error_message[256];
     Token error_token;
 } Parser;

 void parser_init(Parser *parser, const char *source, size_t length);
 ASTNode *parser_parse(Parser *parser);
 bool parser_has_error(const Parser *parser);
 const char *parser_error_message(const Parser *parser);
 Token parser_error_token(const Parser *parser);

 #endif // PYCLITE_PARSER_H

