#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Helper macros for checking token types
#define IS_REL_OPERATOR(token_type) \
((token_type) == TOKEN_EQUAL || (token_type) == TOKEN_NOT_EQUAL || \
(token_type) == TOKEN_LESS || (token_type) == TOKEN_GREATER || \
(token_type) == TOKEN_LESS_EQUAL || (token_type) == TOKEN_GREATER_EQUAL)

#define IS_TYPE_TOKEN(token_type) \
((token_type) == TOKEN_NUM || (token_type) == TOKEN_STRING_TYPE || \
(token_type) == TOKEN_NULL_TYPE)

// Expression stack implementation
static void expr_stack_init(Parser* parser, int capacity) {
    parser->expr_stack.items = malloc(capacity * sizeof(char*));
    parser->expr_stack.top = -1;
    parser->expr_stack.capacity = capacity;
}
/*
static void expr_stack_push(Parser* parser, const char* item) {
    if (parser->expr_stack.top < parser->expr_stack.capacity - 1) {
        parser->expr_stack.top++;
        parser->expr_stack.items[parser->expr_stack.top] = strdup(item);
    }
}

static char* expr_stack_pop(Parser* parser) {
    if (parser->expr_stack.top >= 0) {
        char* item = parser->expr_stack.items[parser->expr_stack.top];
        free(parser->expr_stack.items[parser->expr_stack.top]);
        parser->expr_stack.top--;
        return item;
    }
    return NULL;
}
*/
static void expr_stack_clear(Parser* parser) {
    while (parser->expr_stack.top >= 0) {
        free(parser->expr_stack.items[parser->expr_stack.top]);
        parser->expr_stack.top--;
    }
}

static void expr_stack_free(Parser* parser) {
    expr_stack_clear(parser);
    free(parser->expr_stack.items);
}

/**
 * Initialize parser
 */
Parser* parser_init(FILE* source, FILE* output) {
    Parser* parser = malloc(sizeof(Parser));
    if (!parser) return NULL;
    
    parser->scanner = scanner_init(source);
    if (!parser->scanner) {
        free(parser);
        return NULL;
    }
    
    parser->output = output;
    parser->global_table = symtable_init();
    parser->local_table = NULL;
    parser->had_error = false;
    parser->error_code = SUCCESS;
    parser->label_counter = 0;
    parser->temp_var_counter = 0;
    parser->current_function = NULL;
    parser->in_function = false;
    parser->function_param_count = 0;
    
    // Initialize expression stack
    expr_stack_init(parser, 100);
    
    // Get first token
    next_token(parser);
    
    return parser;
}

/**
 * Destroy parser and free resources
 */
void parser_destroy(Parser* parser) {
    if (!parser) return;
    
    if (parser->scanner) {
        scanner_destroy(parser->scanner);
    }
    
    if (parser->global_table) {
        symtable_free(parser->global_table);
    }
    
    if (parser->local_table) {
        symtable_free(parser->local_table);
    }
    
    if (parser->current_function) {
        free(parser->current_function);
    }
    
    expr_stack_free(parser);
    
    free(parser);
}

/**
 * Get next token from scanner
 */
void next_token(Parser* parser) {
    if (parser->current_token.value) {
        token_free(&parser->current_token);
    }
    parser->current_token = get_next_token(parser->scanner);
}

/**
 * Check if current token matches expected type
 */
bool accept(Parser* parser, TokenType type) {
    return parser->current_token.type == type;
}

/**
 * Expect a specific token type, error if not found
 */
bool expect(Parser* parser, TokenType type) {
    if (accept(parser, type)) {
        return true;
    }
    
    char error_msg[100];
    const char* expected = "unknown";
    const char* got = "unknown";
    
    // Simple mapping for error message 
    switch (type) {
        case TOKEN_IDENTIFIER: expected = "identifier"; break;
        case TOKEN_LEFT_PAREN: expected = "("; break;
        case TOKEN_RIGHT_PAREN: expected = ")"; break;
        case TOKEN_LEFT_BRACE: expected = "{"; break;
        case TOKEN_RIGHT_BRACE: expected = "}"; break;
        case TOKEN_ASSIGN: expected = "="; break;
        case TOKEN_EOL: expected = "end of line"; break;
        default: expected = "specific token"; break;
    }
    
    switch (parser->current_token.type) {
        case TOKEN_IDENTIFIER: got = "identifier"; break;
        case TOKEN_EOF: got = "end of file"; break;
        case TOKEN_EOL: got = "end of line"; break;
        default: got = "other token"; break;
    }
    
    snprintf(error_msg, sizeof(error_msg), "Expected %s, got %s", expected, got);
    error(parser, SYNTAX_ERROR, error_msg);
    return false;
}

/**
 * Report error
 */
void error(Parser* parser, int code, const char* message) {
    if (!parser->had_error) {
        parser->had_error = true;
        parser->error_code = code;
    }
    
    fprintf(stderr, "Error %d at line %d, column %d: %s\n", code, parser->current_token.line, parser->current_token.column, message);
}

/**
 * Parse entire program
 */
int parse_program(Parser* parser) {
    // Generate prolog
    fprintf(parser->output, ".IFJcode25\n");
    generate_prolog(parser);
    
    // Parse prolog (import statement)
    parse_prolog(parser);
    if (parser->had_error) return parser->error_code;
    
    // Parse class definition
    parse_class(parser);
    if (parser->had_error) return parser->error_code;
    
    // Parse function definitions inside class
    parse_function_definitions(parser);
    if (parser->had_error) return parser->error_code;
    
    // Generate epilog
    generate_epilog(parser);
    
    return parser->error_code;
}

/**
 * Generate prolog code
 */
void generate_prolog(Parser* parser) {
    fprintf(parser->output, "CREATEFRAME\n");
    fprintf(parser->output, "PUSHFRAME\n");
}

/**
 * Generate epilog code
 */
void generate_epilog(Parser* parser) {
    SymbolData* main_data = NULL;
    if (!symtable_find(parser->global_table, "main_0", &main_data)) {
        error(parser, SEMANTIC_UNDEFINED, "main function not defined");
        return;
    }
    
    
    fprintf(parser->output, "CALL $main\n");
    fprintf(parser->output, "EXIT int@0\n");
}

/**
 * Parse prolog: import "ifj25" for Ifj
 */
void parse_prolog(Parser* parser) {
    // Expect import keyword
    if (!expect(parser, TOKEN_IMPORT)) {
        error(parser, SYNTAX_ERROR, "Missing import statement");
        return;
    }
    next_token(parser);
    
    // Expect string literal
    if (!expect(parser, TOKEN_STRING_LITERAL)) return;
    
    // Check if string is "ifj25"
    if (!parser->current_token.value || strcmp(parser->current_token.value, "ifj25") != 0) {
        error(parser, SYNTAX_ERROR, "Expected string \"ifj25\" in import");
        return;
    }
    next_token(parser);
    
    // Expect for keyword
    if (!expect(parser, TOKEN_FOR)) return;
    next_token(parser);
    
    // Expect Ifj namespace
    if (!expect(parser, TOKEN_IFJ_NAMESPACE)) return;
    next_token(parser);
    
    // Expect EOL
    if (!expect(parser, TOKEN_EOL)) return;
    next_token(parser);
}

/**
 * Parse class definition: class Program {
 */
void parse_class(Parser* parser) {
    // Expect class keyword
    if (!expect(parser, TOKEN_CLASS)) {
        error(parser, SYNTAX_ERROR, "Missing class definition");
        return;
    }
    next_token(parser);
    
    // Expect Program identifier
    if (!expect(parser, TOKEN_IDENTIFIER)) return;
    
    if (!parser->current_token.value || strcmp(parser->current_token.value, "Program") != 0) {
        error(parser, SYNTAX_ERROR, "Expected 'Program' as class name");
        return;
    }
    next_token(parser);
    
    // Expect {
    if (!expect(parser, TOKEN_LEFT_BRACE)) return;
    next_token(parser);
    
    // Expect EOL after {
    if (!expect(parser, TOKEN_EOL)) return;
    next_token(parser);
}

/**
 * Parse function definitions inside class
 */
void parse_function_definitions(Parser* parser) {
    while (!accept(parser, TOKEN_RIGHT_BRACE) && !parser->had_error) {
        if (accept(parser, TOKEN_STATIC)) {
            parse_function(parser);
        } else if (accept(parser, TOKEN_EOL)) {
            next_token(parser);
        } else {
            error(parser, SYNTAX_ERROR, "Expected function definition or end of class");
            break;
        }
    }
    
    if (!expect(parser, TOKEN_RIGHT_BRACE)) return;
    next_token(parser);
}

/**
 * Parse function definition
 */
void parse_function(Parser* parser) {
    // Consume static
    next_token(parser);
    
    // Get function name
    if (!expect(parser, TOKEN_IDENTIFIER)) return;
    char* func_name = strdup(parser->current_token.value);
    if (!func_name) {
        error(parser, INTERNAL_ERROR, "Memory allocation failed");
        return;
    }
    next_token(parser);
    
    // Check if this is a getter (no parentheses)
    if (accept(parser, TOKEN_LEFT_BRACE)) {
        parse_getter(parser, func_name);
        free(func_name);
        return;
    }
    
    // Check if this is a setter (has = (param) before block)
    if (accept(parser, TOKEN_ASSIGN)) {
        parse_setter(parser, func_name);
        free(func_name);
        return;
    }
    
    // Regular function - expect (
    if (!expect(parser, TOKEN_LEFT_PAREN)) {
        free(func_name);
        return;
    }
    next_token(parser);
    
    // Parse parameters
    int param_count = 0;
    if (!accept(parser, TOKEN_RIGHT_PAREN)) {
        // Parse first parameter
        if (!expect(parser, TOKEN_IDENTIFIER)) {
            free(func_name);
            return;
        }
        param_count++;
        next_token(parser);
        
        // Parse additional parameters
        while (accept(parser, TOKEN_COMMA)) {
            next_token(parser);
            if (!expect(parser, TOKEN_IDENTIFIER)) {
                free(func_name);
                return;
            }
            param_count++;
            next_token(parser);
        }
    }
    
    // Expect )
    if (!expect(parser, TOKEN_RIGHT_PAREN)) {
        free(func_name);
        return;
    }
    next_token(parser);
    
    // Store function information
    SymbolData* func_data = symdata_create_func(IFJ_SYMBOL_FUNC, param_count);
    if (!func_data) {
        error(parser, INTERNAL_ERROR, "Failed to create function data");
        free(func_name);
        return;
    }
    
    // Create unique key: name_arity
    char key[256];
    snprintf(key, sizeof(key), "%s_%d", func_name, param_count);
    
    // Check for redefinition
    SymbolData* existing = NULL;
    if (symtable_find(parser->global_table, key, &existing)) {
        error(parser, SEMANTIC_REDEFINITION, "Function redefined");
        symdata_free(func_data);
        free(func_name);
        return;
    }
    
    // Insert into symbol table
    if (!symtable_insert(parser->global_table, key, func_data)) {
        error(parser, INTERNAL_ERROR, "Failed to insert function");
        symdata_free(func_data);
        free(func_name);
        return;
    }
    
    // Generate function prolog
    generate_function_prolog(parser, func_name, param_count);
    
    // Set current function context
    parser->current_function = strdup(func_name);
    parser->in_function = true;
    parser->function_param_count = param_count;
    
    // Create local symbol table for this function
    if (parser->local_table) {
        symtable_free(parser->local_table);
    }
    parser->local_table = symtable_init();
    
    // Parse function body
    parse_block(parser);
    
    // Generate function epilog
    generate_function_epilog(parser);
    
    // Clean up function context
    free(parser->current_function);
    parser->current_function = NULL;
    parser->in_function = false;
    parser->function_param_count = 0;
    
    free(func_name);
}

/**
 * Generate function prolog
 */
void generate_function_prolog(Parser* parser, const char* name, int param_count) {
    fprintf(parser->output, "LABEL $%s\n", name);
    
    // Create new frame
    fprintf(parser->output, "CREATEFRAME\n");
    fprintf(parser->output, "PUSHFRAME\n");
    
    // Initialize parameters (they will be on stack in reverse order)
    for (int i = param_count - 1; i >= 0; i--) {
        char param_name[32];
        snprintf(param_name, sizeof(param_name), "param%d", i);
        fprintf(parser->output, "DEFVAR LF@%s\n", param_name);
        fprintf(parser->output, "POPS LF@%s\n", param_name);
    }
}

/**
 * Generate function epilog
 */
void generate_function_epilog(Parser* parser) {
    // If no explicit return, push nil
    if (parser->current_function && strcmp(parser->current_function, "main") == 0) {
        fprintf(parser->output, "PUSHS nil@nil\n");
    }
    
    fprintf(parser->output, "POPFRAME\n");
    fprintf(parser->output, "RETURN\n");
}

/**
 * Parse block: { EOL sequence_of_statements }
 */
void parse_block(Parser* parser) {
    // Expect {
    if (!expect(parser, TOKEN_LEFT_BRACE)) return;
    next_token(parser);
    
    // Expect EOL after {
    if (!expect(parser, TOKEN_EOL)) return;
    next_token(parser);
    
    // Parse statements until }
    while (!accept(parser, TOKEN_RIGHT_BRACE) && !parser->had_error) {
        parse_statement(parser);
        
        // Expect EOL after statement (except before })
        if (!accept(parser, TOKEN_RIGHT_BRACE)) {
            if (!expect(parser, TOKEN_EOL)) return;
            next_token(parser);
        }
    }
    
    // Expect }
    if (!expect(parser, TOKEN_RIGHT_BRACE)) return;
    next_token(parser);
}

/**
 * Parse statement
 */
void parse_statement(Parser* parser) {
    if (accept(parser, TOKEN_VAR)) {
        parse_var_declaration(parser);
    } else if (accept(parser, TOKEN_IF)) {
        parse_if_statement(parser);
    } else if (accept(parser, TOKEN_WHILE)) {
        parse_while_statement(parser);
    } else if (accept(parser, TOKEN_RETURN)) {
        parse_return(parser);
    } else if (accept(parser, TOKEN_IDENTIFIER) || accept(parser, TOKEN_GLOBAL_IDENTIFIER)) {
        // Could be assignment or function call
        Token saved_token = parser->current_token;
        next_token(parser);
        
        if (accept(parser, TOKEN_ASSIGN)) {
            // It's an assignment - put token back and parse assignment
            parser->current_token = saved_token;
            parse_assignment(parser);
        } else {
            // It's a function call without assignment (only for builtins in basic version)
            // For now, treat as error unless it's EXTFUN extension
            parser->current_token = saved_token;
            error(parser, SEMANTIC_OTHER, "Function call without assignment not supported in basic version");
        }
    } else {
        error(parser, SYNTAX_ERROR, "Invalid statement");
    }
}

/**
 * Generate variable declaration code
 */
void generate_var_declaration(Parser* parser, const char* name, bool is_global) {
    if (is_global) {
        fprintf(parser->output, "DEFVAR GF@%s\n", name);
        fprintf(parser->output, "MOVE GF@%s nil@nil\n", name);
    } else {
        fprintf(parser->output, "DEFVAR LF@%s\n", name);
        fprintf(parser->output, "MOVE LF@%s nil@nil\n", name);
    }
}

/**
 * Parse variable declaration: var id
 */
void parse_var_declaration(Parser* parser) {
    // Consume var
    next_token(parser);
    
    // Expect identifier
    if (!expect(parser, TOKEN_IDENTIFIER)) return;
    
    char* var_name = strdup(parser->current_token.value);
    if (!var_name) {
        error(parser, INTERNAL_ERROR, "Memory allocation failed");
        return;
    }
    
    // Check for redefinition in current scope
    SymbolData* existing = NULL;
    if (symtable_find(parser->local_table, var_name, &existing)) {
        error(parser, SEMANTIC_REDEFINITION, "Variable redefined");
        free(var_name);
        return;
    }
    
    // Create variable data
    SymbolData* var_data = symdata_create_var(IFJ_TYPE_NULL);
    if (!var_data) {
        error(parser, INTERNAL_ERROR, "Failed to create variable data");
        free(var_name);
        return;
    }
    
    // Insert into local table
    if (!symtable_insert(parser->local_table, var_name, var_data)) {
        error(parser, INTERNAL_ERROR, "Failed to insert variable");
        symdata_free(var_data);
        free(var_name);
        return;
    }
    
    // Generate code for variable declaration
    generate_var_declaration(parser, var_name, false);
    
    free(var_name);
    next_token(parser);
}

/**
 * Generate assignment code
 */
void generate_assignment(Parser* parser, const char* name, bool is_global) {
    // Value should be on stack from expression evaluation
    if (is_global) {
        fprintf(parser->output, "POPS GF@%s\n", name);
    } else {
        fprintf(parser->output, "POPS LF@%s\n", name);
    }
}

/**
 * Parse assignment: id = expression
 */
void parse_assignment(Parser* parser) {
    // Get identifier
    char* var_name = NULL;
    bool is_global = false;
    
    if (accept(parser, TOKEN_IDENTIFIER)) {
        var_name = strdup(parser->current_token.value);
        is_global = false;
    } else if (accept(parser, TOKEN_GLOBAL_IDENTIFIER)) {
        var_name = strdup(parser->current_token.value);
        is_global = true;
    } else {
        error(parser, SYNTAX_ERROR, "Expected identifier in assignment");
        return;
    }
    
    if (!var_name) {
        error(parser, INTERNAL_ERROR, "Memory allocation failed");
        return;
    }
    
    // For local variables, check if they exist
    if (!is_global) {
        SymbolData* var_data = NULL;
        if (!symtable_find(parser->local_table, var_name, &var_data)) {
            error(parser, SEMANTIC_UNDEFINED, "Undefined local variable");
            free(var_name);
            return;
        }
    }
    
    
    next_token(parser);
    
    // Check if this is a function call
    if (accept(parser, TOKEN_IDENTIFIER)) {
        // Could be function call or getter
        char* func_name = strdup(parser->current_token.value);
        next_token(parser);
        
        if (accept(parser, TOKEN_LEFT_PAREN)) {
            // Function call
            parse_function_call(parser, func_name);
        } else {
            // Simple expression
            // Put the token back and parse expression
            Token saved_token = parser->current_token;
            parser->current_token.type = TOKEN_IDENTIFIER;
            parser->current_token.value = func_name;
            parser->current_token.line = parser->current_token.line;
            parser->current_token.column = parser->current_token.column - 1;
            
            parse_expression(parser);
            
            // Restore token
            parser->current_token = saved_token;
        }
        
        free(func_name);
    } else {
        // Parse expression (result will be on stack)
        parse_expression(parser);
    }
    
    // Generate code for assignment
    generate_assignment(parser, var_name, is_global);
    
    free(var_name);
}

/**
 * Parse if statement: if (expression) block else block
 */
void parse_if_statement(Parser* parser) {
    // Generate labels
    char* else_label = generate_label(parser);
    char* end_label = generate_label(parser);
    
    // Consume if
    next_token(parser);
    
    // Expect (
    if (!expect(parser, TOKEN_LEFT_PAREN)) {
        free(else_label);
        free(end_label);
        return;
    }
    next_token(parser);
    
    // Parse condition expression
    parse_expression(parser);
    
    // Condition result is on stack
    fprintf(parser->output, "POPS LF@cond\n");
    fprintf(parser->output, "JUMPIFEQ %s LF@cond bool@false\n", else_label);
    
    // Expect )
    if (!expect(parser, TOKEN_RIGHT_PAREN)) {
        free(else_label);
        free(end_label);
        return;
    }
    next_token(parser);
    
    // Parse then block
    parse_block(parser);
    
    // Jump to end after then block
    fprintf(parser->output, "JUMP %s\n", end_label);
    
    // Generate else label
    fprintf(parser->output, "LABEL %s\n", else_label);
    
    // Expect else
    if (!expect(parser, TOKEN_ELSE)) {
        error(parser, SYNTAX_ERROR, "Expected else in if statement");
        free(else_label);
        free(end_label);
        return;
    }
    next_token(parser);
    
    // Parse else block
    parse_block(parser);
    
    // Generate end label
    fprintf(parser->output, "LABEL %s\n", end_label);
    
    free(else_label);
    free(end_label);
}

/**
 * Parse while statement: while (expression) block
 */
void parse_while_statement(Parser* parser) {
    // Generate labels
    char* start_label = generate_label(parser);
    char* end_label = generate_label(parser);
    
    // Generate start label
    fprintf(parser->output, "LABEL %s\n", start_label);
    
    // Consume while
    next_token(parser);
    
    // Expect (
    if (!expect(parser, TOKEN_LEFT_PAREN)) {
        free(start_label);
        free(end_label);
        return;
    }
    next_token(parser);
    
    // Parse condition expression
    parse_expression(parser);
    
    // Condition result is on stack
    fprintf(parser->output, "POPS LF@cond\n");
    fprintf(parser->output, "JUMPIFEQ %s LF@cond bool@false\n", end_label);
    
    // Expect )
    if (!expect(parser, TOKEN_RIGHT_PAREN)) {
        free(start_label);
        free(end_label);
        return;
    }
    next_token(parser);
    
    // Parse loop body
    parse_block(parser);
    
    // Jump back to start
    fprintf(parser->output, "JUMP %s\n", start_label);
    
    // Generate end label
    fprintf(parser->output, "LABEL %s\n", end_label);
    
    free(start_label);
    free(end_label);
}

/**
 * Generate return code
 */
/*
void generate_return(Parser* parser) {
}
*/
/**
 * Parse return statement: return expression
 */
void parse_return(Parser* parser) {
    // Consume return
    next_token(parser);
    
    // Parse return expression (result will be on stack)
    parse_expression(parser);
    
    // Generate return code
    //generate_return(parser);
}

/**
 * Parse function call
 */
void parse_function_call(Parser* parser, const char* func_name) {
    // Consume (
    if (!expect(parser, TOKEN_LEFT_PAREN)) return;
    next_token(parser);
    
    // Parse arguments
    int arg_count = 0;
    if (!accept(parser, TOKEN_RIGHT_PAREN)) {
        // Parse first argument
        parse_expression(parser);
        arg_count++;
        
        // Parse additional arguments
        while (accept(parser, TOKEN_COMMA)) {
            next_token(parser);
            parse_expression(parser);
            arg_count++;
        }
    }
    
    // Expect )
    if (!expect(parser, TOKEN_RIGHT_PAREN)) return;
    next_token(parser);
    
    // Check if function exists
    char key[256];
    snprintf(key, sizeof(key), "%s_%d", func_name, arg_count);
    
    SymbolData* func_data = NULL;
    bool is_builtin = false;
    
    if (strncmp(func_name, "Ifj.", 4) == 0) {
        // Built-in function
        is_builtin = true;
    } else if (!symtable_find(parser->global_table, key, &func_data)) {
        error(parser, SEMANTIC_UNDEFINED, "Function not defined");
        return;
    }
    
    // Generate function call
    generate_function_call(parser, func_name, arg_count, is_builtin);
}

/**
 * Generate function call code
 */
void generate_function_call(Parser* parser, const char* func_name, int arg_count, bool is_builtin) {
    if (is_builtin) {
        // Built-in functions need special handling
        // For now, just push arguments in reverse order
        for (int i = 0; i < arg_count; i++) {
            // Arguments are on stack in correct order
            // No special generation for builtins in basic version
        }
        
        fprintf(parser->output, "# Call to built-in function %s\n", func_name);
        
        // For basic version, just leave values on stack
        // Real implementation would generate specific IFJcode25 instructions
    } else {
        // User-defined function
        // Arguments should already be on stack in correct order
        fprintf(parser->output, "CALL $%s\n", func_name);
    }
}

/**
 * Parse expression using precedence
 */
void parse_expression(Parser* parser) {
    parse_is_expression(parser);
}

/**
 * Parse is expression (lowest priority)
 */
void parse_is_expression(Parser* parser) {
    parse_relation(parser);
    
    if (accept(parser, TOKEN_IS)) {
        next_token(parser);
        
        // Expect type token
        if (!IS_TYPE_TOKEN(parser->current_token.type)) {
            error(parser, SYNTAX_ERROR, "Expected type after is operator");
            return;
        }
        
        TokenType type_token = parser->current_token.type;
        next_token(parser);
        
        // Generate is operation
        generate_is_op(parser, type_token);
    }
}

/**
 * Parse relation expressions (==, !=, <, >, <=, >=)
 */
void parse_relation(Parser* parser) {
    parse_simple_expression(parser);
    
    while (IS_REL_OPERATOR(parser->current_token.type)) {
        TokenType op = parser->current_token.type;
        next_token(parser);
        
        parse_simple_expression(parser);
        
        // Generate relational operation
        generate_relational_op(parser, op);
    }
}

/**
 * Parse simple expression (+, -)
 */
void parse_simple_expression(Parser* parser) {
    parse_term(parser);
    
    while (accept(parser, TOKEN_PLUS) || accept(parser, TOKEN_MINUS)) {
        TokenType op = parser->current_token.type;
        next_token(parser);
        
        parse_term(parser);
        
        // Generate binary operation
        generate_binary_op(parser, op);
    }
}

/**
 * Parse term (*, /)
 */
void parse_term(Parser* parser) {
    parse_factor(parser);
    
    while (accept(parser, TOKEN_MULTIPLY) || accept(parser, TOKEN_DIVIDE)) {
        TokenType op = parser->current_token.type;
        next_token(parser);
        
        parse_factor(parser);
        
        // Generate binary operation
        generate_binary_op(parser, op);
    }
}

/**
 * Parse factor (basic elements)
 */
void parse_factor(Parser* parser) {
    switch (parser->current_token.type) {
        case TOKEN_IDENTIFIER: {
            // Local variable
            char* name = parser->current_token.value;
            
            // Check if variable exists
            SymbolData* var_data = NULL;
            if (!symtable_find(parser->local_table, name, &var_data)) {
                error(parser, SEMANTIC_UNDEFINED, "Undefined variable");
                return;
            }
            
            // Push variable value onto stack
            fprintf(parser->output, "PUSHS LF@%s\n", name);
            
            next_token(parser);
            break;
        }
            
        case TOKEN_GLOBAL_IDENTIFIER: {
            // Global variable
            char* name = parser->current_token.value;
            
            // Global variables always exist (value is null if not initialized)
            fprintf(parser->output, "PUSHS GF@%s\n", name);
            
            next_token(parser);
            break;
        }
            
        case TOKEN_INT_LITERAL:
            fprintf(parser->output, "PUSHS int@%s\n", parser->current_token.value);
            next_token(parser);
            break;
            
        case TOKEN_FLOAT_LITERAL:
            fprintf(parser->output, "PUSHS float@%s\n", parser->current_token.value);
            next_token(parser);
            break;
            
        case TOKEN_STRING_LITERAL:
            fprintf(parser->output, "PUSHS string@%s\n", parser->current_token.value);
            next_token(parser);
            break;
            
        case TOKEN_NULL:
            fprintf(parser->output, "PUSHS nil@nil\n");
            next_token(parser);
            break;
            
        case TOKEN_LEFT_PAREN:
            next_token(parser);
            parse_expression(parser);
            if (!expect(parser, TOKEN_RIGHT_PAREN)) return;
            next_token(parser);
            break;
            
        default:
            error(parser, SYNTAX_ERROR, "Invalid factor in expression");
    }
    return;
}

/**
 * Generate binary operation code
 */
void generate_binary_op(Parser* parser, TokenType op) {
    char* temp = generate_temp_var(parser);
    
    switch (op) {
        case TOKEN_PLUS:
            fprintf(parser->output, "ADDS\n");
            break;
        case TOKEN_MINUS:
            fprintf(parser->output, "SUBS\n");
            break;
        case TOKEN_MULTIPLY:
            fprintf(parser->output, "MULS\n");
            break;
        case TOKEN_DIVIDE:
            fprintf(parser->output, "DIVS\n");
            break;
        default:
            error(parser, INTERNAL_ERROR, "Unknown binary operator");
            free(temp);
            return;
    }
    
    free(temp);
}

/**
 * Generate relational operation code
 */
void generate_relational_op(Parser* parser, TokenType op) {
    char* temp = generate_temp_var(parser);
    
    switch (op) {
        case TOKEN_EQUAL:
            fprintf(parser->output, "EQS\n");
            break;
        case TOKEN_NOT_EQUAL:
            // For !=, we need to negate the result of EQ
            fprintf(parser->output, "EQS\n");
            fprintf(parser->output, "NOTS\n");
            break;
        case TOKEN_LESS:
            fprintf(parser->output, "LTS\n");
            break;
        case TOKEN_GREATER:
            // For >, we can use LT with swapped operands
            fprintf(parser->output, "POPS LF@temp2\n");
            fprintf(parser->output, "POPS LF@temp1\n");
            fprintf(parser->output, "PUSHS LF@temp2\n");
            fprintf(parser->output, "PUSHS LF@temp1\n");
            fprintf(parser->output, "LTS\n");
            break;
        case TOKEN_LESS_EQUAL:
            // For <=, we can use !(b < a)
            fprintf(parser->output, "POPS LF@temp2\n");
            fprintf(parser->output, "POPS LF@temp1\n");
            fprintf(parser->output, "PUSHS LF@temp1\n");
            fprintf(parser->output, "PUSHS LF@temp2\n");
            fprintf(parser->output, "LTS\n");
            fprintf(parser->output, "NOTS\n");
            break;
        case TOKEN_GREATER_EQUAL:
            // For >=, we can use !(a < b)
            fprintf(parser->output, "LTS\n");
            fprintf(parser->output, "NOTS\n");
            break;
        default:
            error(parser, INTERNAL_ERROR, "Unknown relational operator");
            free(temp);
            return;
    }
    
    free(temp);
}

/**
 * Generate is operation code
 */
void generate_is_op(Parser* parser, TokenType type_token) {
    char* temp = generate_temp_var(parser);
    
    // Get type of value on stack
    fprintf(parser->output, "POPS LF@value\n");
    fprintf(parser->output, "TYPE LF@type LF@value\n");
    
    // Compare with expected type
    const char* expected_type = "";
    switch (type_token) {
        case TOKEN_NUM:
            expected_type = "float";
            break;
        case TOKEN_STRING_TYPE:
            expected_type = "string";
            break;
        case TOKEN_NULL_TYPE:
            expected_type = "nil";
            break;
        default:
            error(parser, SYNTAX_ERROR, "Invalid type in is expression");
            free(temp);
            return;
    }
    
    fprintf(parser->output, "PUSHS string@%s\n", expected_type);
    fprintf(parser->output, "PUSHS LF@type\n");
    fprintf(parser->output, "EQS\n");
    
    free(temp);
}

/**
 * Parse getter function
 */
void parse_getter(Parser* parser, const char* name) {
    // Create getter data
    SymbolData* getter_data = symdata_create_func(IFJ_SYMBOL_GETTER, 0);
    if (!getter_data) {
        error(parser, INTERNAL_ERROR, "Failed to create getter data");
        return;
    }
    
    // Create unique key: name_0 (arity 0 for getter)
    char key[256];
    snprintf(key, sizeof(key), "%s_0", name);
    
    // Check for redefinition
    SymbolData* existing = NULL;
    if (symtable_find(parser->global_table, key, &existing)) {
        error(parser, SEMANTIC_REDEFINITION, "Getter redefined");
        symdata_free(getter_data);
        return;
    }
    
    // Insert into symbol table
    if (!symtable_insert(parser->global_table, key, getter_data)) {
        error(parser, INTERNAL_ERROR, "Failed to insert getter");
        symdata_free(getter_data);
        return;
    }
    
    // Generate function prolog for getter
    generate_function_prolog(parser, name, 0);
    
    // Set current function context
    parser->current_function = strdup(name);
    parser->in_function = true;
    parser->function_param_count = 0;
    
    // Create local symbol table for this getter
    if (parser->local_table) {
        symtable_free(parser->local_table);
    }
    parser->local_table = symtable_init();
    
    // Parse getter body
    parse_block(parser);
    
    // Generate function epilog
    generate_function_epilog(parser);
    
    // Clean up function context
    free(parser->current_function);
    parser->current_function = NULL;
    parser->in_function = false;
    parser->function_param_count = 0;
}

/**
 * Parse setter function
 */
void parse_setter(Parser* parser, const char* name) {
    // Consume =
    next_token(parser);
    
    // Expect (
    if (!expect(parser, TOKEN_LEFT_PAREN)) return;
    next_token(parser);
    
    // Expect parameter identifier
    if (!expect(parser, TOKEN_IDENTIFIER)) return;
    
    char* param_name = strdup(parser->current_token.value);
    if (!param_name) {
        error(parser, INTERNAL_ERROR, "Memory allocation failed");
        return;
    }
    next_token(parser);
    
    // Expect )
    if (!expect(parser, TOKEN_RIGHT_PAREN)) {
        free(param_name);
        return;
    }
    next_token(parser);
    
    // Create setter data (arity 1)
    SymbolData* setter_data = symdata_create_func(IFJ_SYMBOL_SETTER, 1);
    if (!setter_data) {
        error(parser, INTERNAL_ERROR, "Failed to create setter data");
        free(param_name);
        return;
    }
    
    // Create unique key: name_1 (arity 1 for setter)
    char key[256];
    snprintf(key, sizeof(key), "%s_1", name);
    
    // Check for redefinition
    SymbolData* existing = NULL;
    if (symtable_find(parser->global_table, key, &existing)) {
        error(parser, SEMANTIC_REDEFINITION, "Setter redefined");
        symdata_free(setter_data);
        free(param_name);
        return;
    }
    
    // Insert into symbol table
    if (!symtable_insert(parser->global_table, key, setter_data)) {
        error(parser, INTERNAL_ERROR, "Failed to insert setter");
        symdata_free(setter_data);
        free(param_name);
        return;
    }
    
    // Generate function prolog for setter
    generate_function_prolog(parser, name, 1);
    
    // Set current function context
    parser->current_function = strdup(name);
    parser->in_function = true;
    parser->function_param_count = 1;
    
    // Create local symbol table for this setter
    if (parser->local_table) {
        symtable_free(parser->local_table);
    }
    parser->local_table = symtable_init();
    
    // Add parameter to local table
    SymbolData* param_data = symdata_create_var(IFJ_TYPE_NULL);
    if (param_data) {
        symtable_insert(parser->local_table, param_name, param_data);
    }
    
    // Parse setter body
    parse_block(parser);
    
    // Generate function epilog
    generate_function_epilog(parser);
    
    // Clean up function context
    free(parser->current_function);
    parser->current_function = NULL;
    parser->in_function = false;
    parser->function_param_count = 0;
    
    free(param_name);
}

/**
 * Generate a unique label
 */
char* generate_label(Parser* parser) {
    char* label = malloc(32);
    if (label) {
        snprintf(label, 32, "label_%d", parser->label_counter++);
    }
    return label;
}

/**
 * Generate a unique temporary variable
 */
char* generate_temp_var(Parser* parser) {
    char* temp = malloc(32);
    if (temp) {
        snprintf(temp, 32, "temp_%d", parser->temp_var_counter++);
    }
    return temp;
}


                