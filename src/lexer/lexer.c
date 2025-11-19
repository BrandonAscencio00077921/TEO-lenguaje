 #include "lexer.h"
 
 #include <ctype.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 
 typedef struct {
     const char *keyword;
     TokenType type;
 } KeywordEntry;
 
 static const KeywordEntry KEYWORDS[] = {
     {"array", TOKEN_KW_ARRAY},
     {"bool", TOKEN_KW_BOOL},
     {"char", TOKEN_KW_CHAR},
     {"cread", TOKEN_KW_CREAD},
     {"csay", TOKEN_KW_CSAY},
     {"false", TOKEN_FALSE},
     {"float", TOKEN_KW_FLOAT},
     {"for", TOKEN_KW_FOR},
     {"func", TOKEN_KW_FUNC},
     {"if", TOKEN_KW_IF},
     {"in", TOKEN_KW_IN},
     {"int", TOKEN_KW_INT},
     {"return", TOKEN_KW_RETURN},
     {"true", TOKEN_TRUE},
     {"while", TOKEN_KW_WHILE},
 };
 
 static char lexer_peek(const Lexer *lexer) {
     if (lexer->position >= lexer->length) {
         return '\0';
     }
     return lexer->source[lexer->position];
 }
 
 static char lexer_peek_next(const Lexer *lexer) {
     if (lexer->position + 1 >= lexer->length) {
         return '\0';
     }
     return lexer->source[lexer->position + 1];
 }
 
 static char lexer_advance(Lexer *lexer) {
     if (lexer->position >= lexer->length) {
         return '\0';
     }
     char c = lexer->source[lexer->position++];
     if (c == '\n') {
         lexer->line++;
         lexer->column = 1;
     } else {
         lexer->column++;
     }
     return c;
 }
 
 static bool is_identifier_start(char c) {
     return isalpha((unsigned char)c) || c == '_';
 }
 
 static bool is_identifier_part(char c) {
     return isalnum((unsigned char)c) || c == '_';
 }
 
 static void skip_whitespace(Lexer *lexer) {
     while (true) {
         char c = lexer_peek(lexer);
         if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
             lexer_advance(lexer);
             continue;
         }
         break;
     }
 }
 
 static void skip_comment(Lexer *lexer) {
     char c = lexer_peek(lexer);
     char next = lexer_peek_next(lexer);
 
     if (c == '/' && next == '/') {
         while (c != '\n' && c != '\0') {
             c = lexer_advance(lexer);
         }
         return;
     }
 
     if (c == '$') {
         while (c != '\n' && c != '\0') {
             c = lexer_advance(lexer);
         }
         return;
     }
 
     if (c == '/' && next == '*') {
         lexer_advance(lexer);
         lexer_advance(lexer);
         while (true) {
             if (lexer_peek(lexer) == '\0') {
                 break;
             }
             if (lexer_peek(lexer) == '*' && lexer_peek_next(lexer) == '/') {
                 lexer_advance(lexer);
                 lexer_advance(lexer);
                 break;
             }
             lexer_advance(lexer);
         }
         return;
     }
 
     if (c == '%' && next == '%') {
         lexer_advance(lexer);
         lexer_advance(lexer);
         while (true) {
             if (lexer_peek(lexer) == '\0') {
                 break;
             }
             if (lexer_peek(lexer) == '%' && lexer_peek_next(lexer) == '%') {
                 lexer_advance(lexer);
                 lexer_advance(lexer);
                 break;
             }
             lexer_advance(lexer);
         }
     }
 }
 
 static void skip_ignorable(Lexer *lexer) {
     bool progress = true;
     while (progress) {
         progress = false;
         size_t before = lexer->position;
         skip_whitespace(lexer);
         if (lexer->position != before) {
             progress = true;
             continue;
         }
         char c = lexer_peek(lexer);
         char next = lexer_peek_next(lexer);
         if ((c == '/' && (next == '/' || next == '*')) || c == '$' || (c == '%' && next == '%')) {
             skip_comment(lexer);
             progress = true;
         }
     }
 }
 
 void lexer_init(Lexer *lexer, const char *source, size_t length) {
     lexer->source = source;
     lexer->length = length;
     lexer->position = 0;
     lexer->line = 1;
     lexer->column = 1;
 }
 
 static Token make_token(Lexer *lexer, TokenType type, size_t start, size_t length) {
     Token token;
     token.type = type;
     token.lexeme = lexer->source + start;
     token.length = length;
     token.line = lexer->line;
     token.column = lexer->column - (length ? length - 1 : 0);
     return token;
 }
 
 static Token token_from_identifier(Lexer *lexer, size_t start) {
     size_t length = 0;
     while (is_identifier_part(lexer_peek(lexer))) {
         lexer_advance(lexer);
     }
     size_t end = lexer->position;
     length = end - start;
 
     for (size_t i = 0; i < sizeof(KEYWORDS) / sizeof(KEYWORDS[0]); ++i) {
         const KeywordEntry *entry = &KEYWORDS[i];
         if (length == strlen(entry->keyword) && strncmp(lexer->source + start, entry->keyword, length) == 0) {
             return make_token(lexer, entry->type, start, length);
         }
     }
     return make_token(lexer, TOKEN_IDENTIFIER, start, length);
 }
 
 static Token token_from_number(Lexer *lexer, size_t start) {
     bool has_dot = false;
     while (isdigit((unsigned char)lexer_peek(lexer)) || lexer_peek(lexer) == '.') {
         if (lexer_peek(lexer) == '.') {
             if (has_dot) {
                 break;
             }
             has_dot = true;
         }
         lexer_advance(lexer);
     }
     size_t length = lexer->position - start;
     return make_token(lexer, TOKEN_NUMBER, start, length);
 }
 
 static Token token_from_string(Lexer *lexer, size_t start, char quote) {
     while (true) {
         char c = lexer_peek(lexer);
         if (c == '\0') {
             break;
         }
         if (c == quote) {
             lexer_advance(lexer);
             break;
         }
         if (c == '\\' && lexer_peek_next(lexer) != '\0') {
             lexer_advance(lexer);
         }
         lexer_advance(lexer);
     }
     size_t length = lexer->position - start;
     return make_token(lexer, quote == '"' ? TOKEN_STRING : TOKEN_CHAR, start, length);
 }
 
 Token lexer_next_token(Lexer *lexer) {
     skip_ignorable(lexer);
     size_t start = lexer->position;
 
     if (start >= lexer->length) {
         Token token = {TOKEN_EOF, "", 0, lexer->line, lexer->column};
         return token;
     }
 
     char c = lexer_advance(lexer);
 
     if (is_identifier_start(c)) {
         return token_from_identifier(lexer, start);
     }
 
     if (isdigit((unsigned char)c)) {
         return token_from_number(lexer, start);
     }
 
     switch (c) {
         case '"':
             return token_from_string(lexer, start, '"');
         case '\'':
             return token_from_string(lexer, start, '\'');
         case '+':
             if (lexer_peek(lexer) == '+') {
                 lexer_advance(lexer);
                 return make_token(lexer, TOKEN_PLUSPLUS, start, 2);
             }
             return make_token(lexer, TOKEN_PLUS, start, 1);
         case '-':
             if (lexer_peek(lexer) == '-') {
                 lexer_advance(lexer);
                 return make_token(lexer, TOKEN_MINUSMINUS, start, 2);
             }
             return make_token(lexer, TOKEN_MINUS, start, 1);
         case '*':
             return make_token(lexer, TOKEN_STAR, start, 1);
         case '/':
             return make_token(lexer, TOKEN_SLASH, start, 1);
         case '%':
             return make_token(lexer, TOKEN_PERCENT, start, 1);
         case '=':
             if (lexer_peek(lexer) == '=') {
                 lexer_advance(lexer);
                 return make_token(lexer, TOKEN_EQEQ, start, 2);
             }
             return make_token(lexer, TOKEN_EQ, start, 1);
         case '!':
             if (lexer_peek(lexer) == '=') {
                 lexer_advance(lexer);
                 return make_token(lexer, TOKEN_BANGEQ, start, 2);
             }
             return make_token(lexer, TOKEN_BANG, start, 1);
         case '<':
             if (lexer_peek(lexer) == '=') {
                 lexer_advance(lexer);
                 return make_token(lexer, TOKEN_LTE, start, 2);
             }
             return make_token(lexer, TOKEN_LT, start, 1);
         case '>':
             if (lexer_peek(lexer) == '=') {
                 lexer_advance(lexer);
                 return make_token(lexer, TOKEN_GTE, start, 2);
             }
             return make_token(lexer, TOKEN_GT, start, 1);
         case '&':
             if (lexer_peek(lexer) == '&') {
                 lexer_advance(lexer);
                 return make_token(lexer, TOKEN_ANDAND, start, 2);
             }
             break;
         case '|':
             if (lexer_peek(lexer) == '|') {
                 lexer_advance(lexer);
                 return make_token(lexer, TOKEN_OROR, start, 2);
             }
             break;
         case '(':
             return make_token(lexer, TOKEN_LPAREN, start, 1);
         case ')':
             return make_token(lexer, TOKEN_RPAREN, start, 1);
         case '{':
             return make_token(lexer, TOKEN_LBRACE, start, 1);
         case '}':
             return make_token(lexer, TOKEN_RBRACE, start, 1);
         case '[':
             return make_token(lexer, TOKEN_LBRACKET, start, 1);
         case ']':
             return make_token(lexer, TOKEN_RBRACKET, start, 1);
         case ',':
             return make_token(lexer, TOKEN_COMMA, start, 1);
         case ';':
             return make_token(lexer, TOKEN_SEMICOLON, start, 1);
         case '.':
             return make_token(lexer, TOKEN_DOT, start, 1);
     }
 
     return make_token(lexer, TOKEN_UNKNOWN, start, 1);
 }
 
 const char *token_type_str(TokenType type) {
     switch (type) {
 #define TOKEN_NAME(t) case t: return #t;
         TOKEN_NAME(TOKEN_EOF)
         TOKEN_NAME(TOKEN_IDENTIFIER)
         TOKEN_NAME(TOKEN_NUMBER)
         TOKEN_NAME(TOKEN_STRING)
         TOKEN_NAME(TOKEN_CHAR)
         TOKEN_NAME(TOKEN_TRUE)
         TOKEN_NAME(TOKEN_FALSE)
         TOKEN_NAME(TOKEN_KW_INT)
         TOKEN_NAME(TOKEN_KW_FLOAT)
         TOKEN_NAME(TOKEN_KW_CHAR)
         TOKEN_NAME(TOKEN_KW_BOOL)
         TOKEN_NAME(TOKEN_KW_ARRAY)
         TOKEN_NAME(TOKEN_KW_IF)
         TOKEN_NAME(TOKEN_KW_FOR)
         TOKEN_NAME(TOKEN_KW_IN)
         TOKEN_NAME(TOKEN_KW_WHILE)
         TOKEN_NAME(TOKEN_KW_FUNC)
         TOKEN_NAME(TOKEN_KW_RETURN)
         TOKEN_NAME(TOKEN_KW_CSAY)
         TOKEN_NAME(TOKEN_KW_CREAD)
         TOKEN_NAME(TOKEN_PLUS)
         TOKEN_NAME(TOKEN_MINUS)
         TOKEN_NAME(TOKEN_STAR)
         TOKEN_NAME(TOKEN_SLASH)
         TOKEN_NAME(TOKEN_PERCENT)
         TOKEN_NAME(TOKEN_EQ)
         TOKEN_NAME(TOKEN_EQEQ)
         TOKEN_NAME(TOKEN_BANGEQ)
         TOKEN_NAME(TOKEN_LT)
         TOKEN_NAME(TOKEN_LTE)
         TOKEN_NAME(TOKEN_GT)
         TOKEN_NAME(TOKEN_GTE)
         TOKEN_NAME(TOKEN_ANDAND)
         TOKEN_NAME(TOKEN_OROR)
         TOKEN_NAME(TOKEN_BANG)
         TOKEN_NAME(TOKEN_PLUSPLUS)
         TOKEN_NAME(TOKEN_MINUSMINUS)
         TOKEN_NAME(TOKEN_LPAREN)
         TOKEN_NAME(TOKEN_RPAREN)
         TOKEN_NAME(TOKEN_LBRACE)
         TOKEN_NAME(TOKEN_RBRACE)
         TOKEN_NAME(TOKEN_LBRACKET)
         TOKEN_NAME(TOKEN_RBRACKET)
         TOKEN_NAME(TOKEN_COMMA)
         TOKEN_NAME(TOKEN_SEMICOLON)
         TOKEN_NAME(TOKEN_DOT)
         TOKEN_NAME(TOKEN_COMMENT)
         TOKEN_NAME(TOKEN_UNKNOWN)
 #undef TOKEN_NAME
         default:
             return "TOKEN_UNDEFINED";
     }
 }

