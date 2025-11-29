#include "scanner.h"


typedef struct {
    const char* keyword;
    TokenType token_type;
} Keyword;

static const Keyword keywords[] = {
    {"class", TOKEN_CLASS},
    {"if", TOKEN_IF},
    {"else", TOKEN_ELSE},
    {"is", TOKEN_IS},
    {"null", TOKEN_NULL},
    {"return", TOKEN_RETURN},
    {"var", TOKEN_VAR},
    {"while", TOKEN_WHILE},
    {"Ifj", TOKEN_IFJ_NAMESPACE},
    {"static", TOKEN_STATIC},
    {"import", TOKEN_IMPORT},
    {"for", TOKEN_FOR},
    {"Num", TOKEN_NUM},
    {"String", TOKEN_STRING_TYPE},
    {"Null", TOKEN_NULL_TYPE},
    {NULL, TOKEN_IDENTIFIER}  
};

/**
 *  Initiliazes the scanner
 * @param source file with scanned code
 * @return initialized scanner
 */
Scanner* scanner_init(FILE* source) {
    Scanner* scanner = malloc(sizeof(Scanner));
    if (scanner == NULL) return NULL;
    
    scanner->source = source;
    scanner->line = 1;
    scanner->column = 0;
    scanner->is_eof_reached = false;
    scanner->current_char = '\0';
    
    // Read first character
    advance(scanner);
    return scanner;
}

/**
 *  Frees the scanner
 * @param scanner scanner to be freed
 */
void scanner_destroy(Scanner* scanner) {
    if (scanner != NULL) {
        free(scanner);
    }
}

/**
 *  Advances the scanner to the next character
 * @param scanner 
 * @return new character
 */
char advance(Scanner* scanner) {
    if (scanner->is_eof_reached) {
        return '\0';
    }
    
    scanner->current_char = fgetc(scanner->source);
    
    if (scanner->current_char == EOF) {
        scanner->is_eof_reached = true;
        scanner->current_char = '\0';
    } else if (scanner->current_char == '\n') {
        scanner->line++;
        scanner->column = 0;
    } else {
        scanner->column++;
    }
    
    return scanner->current_char;
}

/**
 *  Peeks at next char without advancing scanner
 * @param scanner 
 * @return next char in line
 */
char peek(Scanner* scanner) {
    if (scanner->is_eof_reached) {
        return '\0';
    }
    
    int next_char = fgetc(scanner->source);
    if (next_char == EOF) {
        return '\0';
    }
    
    ungetc(next_char, scanner->source);
    return (char)next_char;
}

/**
 * Skips whitespaces and comments
 * @param scanner 
 */
void skip_whitespace(Scanner* scanner) {
    while (true) {
        while (isspace(scanner->current_char) && scanner->current_char != '\n') {
            advance(scanner);
        }
        
        if (scanner->current_char == '/') {
            char next = peek(scanner);
            if (next == '/') {
                // Line comment
                while (scanner->current_char != '\n' && !scanner->is_eof_reached) {
                    advance(scanner);
                }
                if (scanner->current_char == '\n') {
                    advance(scanner);
                }
            } else if (next == '*') {
                // Block comment
                skip_comment(scanner);
            } else {
                break;
            }
        } else {
            break;
        }
    }
}

/**
 *  Skips block comments (and nested block comments)
 * @param scanner
 */
void skip_comment(Scanner* scanner) {
    int nesting = 1;
    advance(scanner); // Skip '/'
    advance(scanner); // Skip '*'
    
    while (nesting > 0 && !scanner->is_eof_reached) {
        if (scanner->current_char == '/' && peek(scanner) == '*') {
            nesting++;
            advance(scanner); // Skip '/'
            advance(scanner); // Skip '*'
        } else if (scanner->current_char == '*' && peek(scanner) == '/') {
            nesting--;
            advance(scanner); // Skip '*'
            advance(scanner); // Skip '/'
        } else {
            advance(scanner);
        }
    }
}

/**
 *  Checks if a string is one of the keywords
 * @param str 
 * @return true if str is a keyword
 */
bool is_keyword(const char* str) {
    for (int i = 0; keywords[i].keyword != NULL; i++) {
        if (strcmp(keywords[i].keyword, str) == 0) {
            return true;
        }
    }
    return false;
}

/**
 *  Assigns token type to a keyword
 * @param str keyword
 * @return token type of the keyword or token type TOKEN_IDENTIFIER if not a keyword
 */
TokenType get_keyword_type(const char* str) {
    for (int i = 0; keywords[i].keyword != NULL; i++) {
        if (strcmp(keywords[i].keyword, str) == 0) {
            return keywords[i].token_type;
        }
    }
    return TOKEN_IDENTIFIER;
}

/**
 *  Creates a token and dynamically allocates it value
 * @param type type of the token
 * @param value value of the token
 * @param line number of the line where the lexem represented by the token is located
 * @param colum number of the columnwhere the lexem represented by the token is located
 * @return created token
 */Token create_token(TokenType type, const char* value, int line, int column) {
    Token token;
    token.type = type;
    token.line = line;
    token.column = column;
    
    if (value) {
        token.value = malloc(strlen(value) + 1);
        if (token.value) {
            strcpy(token.value, value);
        }
    } else {
        token.value = NULL;
    }
    
    return token;
}

/**
 *  Reads one whole identifier, determines if it is a local or global identifier
 *  and creates a token
 * @param scanner
 * @return token of read identifier
 */
Token read_identifier(Scanner* scanner) {
    int start_line = scanner->line;
    int start_column = scanner->column;
    
    char buffer[256];
    int pos = 0;
    
    // Check for global identifier (starts with __)
    if (scanner->current_char == '_' && peek(scanner) == '_') {
        buffer[pos++] = scanner->current_char;
        advance(scanner);
        buffer[pos++] = scanner->current_char;
        advance(scanner);
        
        while (isalnum(scanner->current_char) || scanner->current_char == '_') {
            if (pos < 255) {
                buffer[pos++] = scanner->current_char;
            }
            advance(scanner);
        }
        
        buffer[pos] = '\0';
        return create_token(TOKEN_GLOBAL_IDENTIFIER, buffer, start_line, start_column);
    }
    
    // Regular identifier
    while (isalnum(scanner->current_char) || scanner->current_char == '_') {
        if (pos < 255) {
            buffer[pos++] = scanner->current_char;
        }
        advance(scanner);
    }
    buffer[pos] = '\0';
    
   
    if (is_keyword(buffer)) {
        return create_token(get_keyword_type(buffer), buffer, start_line, start_column);
    }
    
    return create_token(TOKEN_IDENTIFIER, buffer, start_line, start_column);
}

/**
 *  Reads one whole number and creates a token
 * @param scanner
 * @return token of read number
 */
Token read_number(Scanner* scanner) {
    int start_line = scanner->line;
    int start_column = scanner->column;
    
    char buffer[256];
    int pos = 0;
    bool is_float = false;
    bool is_hex = false;
    
    // Check for hexadecimal
    if (scanner->current_char == '0' && (peek(scanner) == 'x' || peek(scanner) == 'X')) {
        buffer[pos++] = scanner->current_char;
        advance(scanner);
        buffer[pos++] = scanner->current_char;
        advance(scanner);
        is_hex = true;
        
        while (isxdigit(scanner->current_char)) {
            if (pos < 255) {
                buffer[pos++] = scanner->current_char;
            }
            advance(scanner);
        }
    } else {
        // Regular decimal number
        while (isdigit(scanner->current_char)) {
            if (pos < 255) {
                buffer[pos++] = scanner->current_char;
            }
            advance(scanner);
        }
        
        // Check for decimal point
        if (scanner->current_char == '.') {
            is_float = true;
            if (pos < 255) {
                buffer[pos++] = scanner->current_char;
            }
            advance(scanner);
            
            while (isdigit(scanner->current_char)) {
                if (pos < 255) {
                    buffer[pos++] = scanner->current_char;
                }
                advance(scanner);
            }
        }
        
        // Check for exponent
        if (scanner->current_char == 'e' || scanner->current_char == 'E') {
            is_float = true;
            if (pos < 255) {
                buffer[pos++] = scanner->current_char;
            }
            advance(scanner);
            
            // Optional sign
            if (scanner->current_char == '+' || scanner->current_char == '-') {
                if (pos < 255) {
                    buffer[pos++] = scanner->current_char;
                }
                advance(scanner);
            }
            
            while (isdigit(scanner->current_char)) {
                if (pos < 255) {
                    buffer[pos++] = scanner->current_char;
                }
                advance(scanner);
            }
        }
    }
    
    buffer[pos] = '\0';
    
    if (is_float) {
        return create_token(TOKEN_FLOAT_LITERAL, buffer, start_line, start_column);
    } else {
        return create_token(TOKEN_INT_LITERAL, buffer, start_line, start_column);
    }
}

/**
 *  Reads scanner escape sequence
 * @param scanner
 * @return read escape sequence or read character when invalid escape sequence
 */
char read_escape_sequence(Scanner* scanner) {
    advance(scanner); // Skip backslash
    
    switch (scanner->current_char) {
        case 'n': advance(scanner); return '\n';
        case 'r': advance(scanner); return '\r';
        case 't': advance(scanner); return '\t';
        case '\\': advance(scanner); return '\\';
        case '"': advance(scanner); return '"';
        case 'x': {
            // Hexadecimal escape sequence
            advance(scanner);
            char hex[3] = {0};
            if (isxdigit(scanner->current_char)) {
                hex[0] = scanner->current_char;
                advance(scanner);
                if (isxdigit(scanner->current_char)) {
                    hex[1] = scanner->current_char;
                    advance(scanner);
                }
            }
            return (char)strtol(hex, NULL, 16);
        }
        default:
            char c = scanner->current_char;
            advance(scanner);
            return c;
    }
}

/**
 *  Reads one whole string, checks for multiline streng and creates a toke
 * @param scanner
 * @return token of read string
 */
Token read_string(Scanner* scanner) {
    int start_line = scanner->line;
    int start_column = scanner->column;
    
    char buffer[1024];
    int pos = 0;
    char quote_char = scanner->current_char;
    
    // Check for multiline string (triple quotes)
    bool is_multiline = false;
    if (scanner->current_char == '"' && peek(scanner) == '"' && peek2(scanner) == '"') {
        is_multiline = true;
        advance(scanner); // Skip second quote
        advance(scanner); // Skip third quote
    } else {
        advance(scanner); // Skip opening quote
    }
    
    if (is_multiline) {
        // Multiline string - no escape sequences
        while (!scanner->is_eof_reached) {
            if (scanner->current_char == '"' && peek(scanner) == '"' && peek2(scanner) == '"') {
                advance(scanner); // Skip first quote
                advance(scanner); // Skip second quote
                advance(scanner); // Skip third quote
                break;
            }
            
            if (pos < 1023) {
                buffer[pos++] = scanner->current_char;
            }
            advance(scanner);
        }
    } else {
        // Regular string with escape sequences
        while (scanner->current_char != quote_char && !scanner->is_eof_reached) {
            if (scanner->current_char == '\\') {
                char escaped = read_escape_sequence(scanner);
                if (pos < 1023) {
                    buffer[pos++] = escaped;
                }
            } else if (scanner->current_char == '\n') {
                // Strings cannot span multiple lines (except multiline)
                break;
            } else {
                if (pos < 1023) {
                    buffer[pos++] = scanner->current_char;
                }
                advance(scanner);
            }
        }
        
        if (scanner->current_char == quote_char) {
            advance(scanner); // Skip closing quote
        }
    }
    
    buffer[pos] = '\0';
    
    if (is_multiline) {
        return create_token(TOKEN_MULTILINE_STRING_LITERAL, buffer, start_line, start_column);
    } else {
        return create_token(TOKEN_STRING_LITERAL, buffer, start_line, start_column);
    }
}

/**
 *  Peeks 2 characters ahead without advancing scanner
 * @param scanner 
 * @return char 2 characters ahead
 */
char peek2(Scanner* scanner) {
    if (scanner->is_eof_reached) {
        return '\0';
    }
    
    int c1 = fgetc(scanner->source);
    if (c1 == EOF) {
        return '\0';
    }
    
    int c2 = fgetc(scanner->source);
    if (c2 == EOF) {
        ungetc(c1, scanner->source);
        return '\0';
    }
    
    ungetc(c2, scanner->source);
    ungetc(c1, scanner->source);
    
    return (char)c2;
}

/**
 *  Main token scanning function
 * @param scanner 
 * @return scanned token
 */
Token get_next_token(Scanner* scanner) {
    if (scanner->is_eof_reached) {
        return create_token(TOKEN_EOF, NULL, scanner->line, scanner->column);
    }
    
    skip_whitespace(scanner);
    
    if (scanner->is_eof_reached) {
        return create_token(TOKEN_EOF, NULL, scanner->line, scanner->column);
    }
    
    int start_line = scanner->line;
    int start_column = scanner->column;
    
    char current = scanner->current_char;
    
    // Identifiers and keywords
    if (isalpha(current) || current == '_') {
        return read_identifier(scanner);
    }
    
    // Numbers
    if (isdigit(current)) {
        return read_number(scanner);
    }
    
    // Strings
    if (current == '"') {
        return read_string(scanner);
    }
    
    // Single character tokens
    switch (current) {
        case '\n':
            advance(scanner);
            return create_token(TOKEN_EOL, NULL, start_line, start_column);
            
        case '+':
            advance(scanner);
            return create_token(TOKEN_PLUS, NULL, start_line, start_column);
            
        case '-':
            advance(scanner);
            return create_token(TOKEN_MINUS, NULL, start_line, start_column);
            
        case '*':
            advance(scanner);
            return create_token(TOKEN_MULTIPLY, NULL, start_line, start_column);
            
        case '/':
            advance(scanner);
            return create_token(TOKEN_DIVIDE, NULL, start_line, start_column);
            
        case '=':
            advance(scanner);
            if (scanner->current_char == '=') {
                advance(scanner);
                return create_token(TOKEN_EQUAL, NULL, start_line, start_column);
            }
            return create_token(TOKEN_ASSIGN, NULL, start_line, start_column);
            
        case '<':
            advance(scanner);
            if (scanner->current_char == '=') {
                advance(scanner);
                return create_token(TOKEN_LESS_EQUAL, NULL, start_line, start_column);
            }
            return create_token(TOKEN_LESS, NULL, start_line, start_column);
            
        case '>':
            advance(scanner);
            if (scanner->current_char == '=') {
                advance(scanner);
                return create_token(TOKEN_GREATER_EQUAL, NULL, start_line, start_column);
            }
            return create_token(TOKEN_GREATER, NULL, start_line, start_column);
            
        case '!':
            advance(scanner);
            if (scanner->current_char == '=') {
                advance(scanner);
                return create_token(TOKEN_NOT_EQUAL, NULL, start_line, start_column);
            }
            return create_token(TOKEN_NOT, NULL, start_line, start_column);
            
        case '(':
            advance(scanner);
            return create_token(TOKEN_LEFT_PAREN, NULL, start_line, start_column);
            
        case ')':
            advance(scanner);
            return create_token(TOKEN_RIGHT_PAREN, NULL, start_line, start_column);
            
        case '{':
            advance(scanner);
            return create_token(TOKEN_LEFT_BRACE, NULL, start_line, start_column);
            
        case '}':
            advance(scanner);
            return create_token(TOKEN_RIGHT_BRACE, NULL, start_line, start_column);
            
        case ',':
            advance(scanner);
            return create_token(TOKEN_COMMA, NULL, start_line, start_column);
            
        case '.':
            advance(scanner);
            if (scanner->current_char == '.' && peek(scanner) == '.') {
                advance(scanner); // Skip second dot
                advance(scanner); // Skip third dot
                return create_token(TOKEN_RANGE_INCLUSIVE, NULL, start_line, start_column);
            } else if (scanner->current_char == '.') {
                advance(scanner);
                return create_token(TOKEN_RANGE_EXCLUSIVE, NULL, start_line, start_column);
            }
            return create_token(TOKEN_DOT, NULL, start_line, start_column);
            
        case ':':
            advance(scanner);
            return create_token(TOKEN_COLON, NULL, start_line, start_column);
            
        case '?':
            advance(scanner);
            return create_token(TOKEN_QUESTION, NULL, start_line, start_column);
            
        case '&':
            if (peek(scanner) == '&') {
                advance(scanner);
                advance(scanner);
                return create_token(TOKEN_AND, NULL, start_line, start_column);
            }
            break;
            
        case '|':
            if (peek(scanner) == '|') {
                advance(scanner);
                advance(scanner);
                return create_token(TOKEN_OR, NULL, start_line, start_column);
            }
            break;
    }
    
    // Unknown character
    char unknown[2] = {current, '\0'};
    Token error_token = create_token(TOKEN_ERROR, unknown, start_line, start_column);
    advance(scanner);
    return error_token;
}

/**
 *  Frees token from memmory
 * @param token 
 */
void token_free(Token* token) {
    if (token && token->value) {
        free(token->value);
        token->value = NULL;
    }
}
