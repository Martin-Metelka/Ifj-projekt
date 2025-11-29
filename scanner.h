#ifndef SCANNER_H
#define SCANNER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

typedef enum {

    // Special tokens
    TOKEN_EOF,
    TOKEN_EOL,
    TOKEN_ERROR,
    
    // Identifiers
    TOKEN_IDENTIFIER,
    TOKEN_GLOBAL_IDENTIFIER,

    // Literals
    TOKEN_INT_LITERAL,
    TOKEN_FLOAT_LITERAL,
    TOKEN_STRING_LITERAL,
    TOKEN_MULTILINE_STRING_LITERAL,
    TOKEN_NULL,
    
    // Keywords
    TOKEN_CLASS,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_IS,
    TOKEN_RETURN,
    TOKEN_VAR,
    TOKEN_WHILE,
    TOKEN_STATIC,
    TOKEN_IMPORT,
    TOKEN_FOR,
    TOKEN_NUM,
    TOKEN_STRING_TYPE,
    TOKEN_NULL_TYPE,
    
    // Built-in namespace
    TOKEN_IFJ_NAMESPACE,
    
    // Operators and punctuation
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MULTIPLY,
    TOKEN_DIVIDE,
    TOKEN_ASSIGN,
    TOKEN_LESS,
    TOKEN_GREATER,
    TOKEN_LESS_EQUAL,
    TOKEN_GREATER_EQUAL,
    TOKEN_EQUAL,
    TOKEN_NOT_EQUAL,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE,
    TOKEN_RIGHT_BRACE,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_COLON,
    TOKEN_QUESTION,
    
    // Range operators (není nutný)
    TOKEN_RANGE_EXCLUSIVE,
    TOKEN_RANGE_INCLUSIVE,
    
    // Boolean operators (není nutný)
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT
} TokenType;

typedef struct {
    TokenType type;
    char* value;        
    int line;       
    int column;        
} Token;

typedef struct {
    FILE* source;           
    char current_char;     
    int line;               
    int column;             
    bool is_eof_reached;    
} Scanner;

Scanner* scanner_init(FILE* source);
void scanner_destroy(Scanner* scanner);
Token get_next_token(Scanner* scanner);
void token_free(Token* token);
const char* token_type_to_string(TokenType type);

char advance(Scanner* scanner);
char peek(Scanner* scanner);
void skip_whitespace(Scanner* scanner);
void skip_comment(Scanner* scanner);
bool is_keyword(const char* str);

#endif
