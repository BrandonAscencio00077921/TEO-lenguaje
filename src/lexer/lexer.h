 #ifndef PYCLITE_LEXER_H
 #define PYCLITE_LEXER_H

 #include <stdbool.h>
 #include <stddef.h>
 
 typedef enum {
     TOKEN_EOF = 0,
     TOKEN_IDENTIFIER,
     TOKEN_NUMBER,
     TOKEN_STRING,
     TOKEN_CHAR,
     TOKEN_TRUE,
     TOKEN_FALSE,
 
     TOKEN_KW_INT,
     TOKEN_KW_FLOAT,
     TOKEN_KW_CHAR,
     TOKEN_KW_BOOL,
     TOKEN_KW_ARRAY,
     TOKEN_KW_IF,
     TOKEN_KW_FOR,
     TOKEN_KW_IN,
     TOKEN_KW_WHILE,
     TOKEN_KW_FUNC,
     TOKEN_KW_RETURN,
     TOKEN_KW_CSAY,
     TOKEN_KW_CREAD,
 
     TOKEN_PLUS,
     TOKEN_MINUS,
     TOKEN_STAR,
     TOKEN_SLASH,
     TOKEN_PERCENT,
     TOKEN_EQ,
     TOKEN_EQEQ,
     TOKEN_BANGEQ,
     TOKEN_LT,
     TOKEN_LTE,
     TOKEN_GT,
     TOKEN_GTE,
     TOKEN_ANDAND,
     TOKEN_OROR,
     TOKEN_BANG,
     TOKEN_PLUSPLUS,
     TOKEN_MINUSMINUS,
 
     TOKEN_LPAREN,
     TOKEN_RPAREN,
     TOKEN_LBRACE,
     TOKEN_RBRACE,
     TOKEN_LBRACKET,
     TOKEN_RBRACKET,
     TOKEN_COMMA,
     TOKEN_SEMICOLON,
     TOKEN_DOT,
 
     TOKEN_COMMENT,
     TOKEN_UNKNOWN
 } TokenType;
 
 typedef struct {
     TokenType type;
     const char *lexeme;
     size_t length;
     size_t line;
     size_t column;
 } Token;
 
 typedef struct {
     const char *source;
     size_t length;
     size_t position;
     size_t line;
     size_t column;
 } Lexer;
 
 void lexer_init(Lexer *lexer, const char *source, size_t length);
 Token lexer_next_token(Lexer *lexer);
 const char *token_type_str(TokenType type);
 
 #endif // PYCLITE_LEXER_H

