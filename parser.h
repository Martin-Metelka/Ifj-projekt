/**
 * Project: Implementace překladače jazyka IFJ25.
 *
 * parser.c
 * syntactical analysis and code generation
 *
 * @author Jakub Gono - xgonoja00
 * @author Vojtěch Kabelka - xkabelv00
 * @author Filip Kachyňa - xkachyf00
 * @author Martin Metelka - xmetelm00
 */

#ifndef PARSER_H
#define PARSER_H

#include "scanner.h"
#include "symtable.h"
#include <stdio.h>
#include <stdbool.h>

// Error codes (from assignment)
#define SUCCESS 0
#define LEXICAL_ERROR 1
#define SYNTAX_ERROR 2
#define SEMANTIC_UNDEFINED 3
#define SEMANTIC_REDEFINITION 4
#define SEMANTIC_ARG_COUNT 5
#define SEMANTIC_TYPE_COMPAT 6
#define SEMANTIC_OTHER 10
#define INTERNAL_ERROR 99

// Parser state structure
typedef struct {
    Scanner* scanner;
    Token current_token;
    SymTable* global_table;      // For global variables and functions
    SymTable* local_table;       // For local variables (current scope)
    FILE* output;                // For generated IFJcode25 code
    bool had_error;
    int error_code;
    
    // Code generation state
    int label_counter;
    int temp_var_counter;
    char* current_function;
    bool in_function;
    int function_param_count;
    
    // Stack for expression evaluation
    struct {
        char** items;
        int top;
        int capacity;
    } expr_stack;
    
} Parser;

// allocates space for string and copieas value of str1 to adress
char * strdup( const char *str1 );

// Function prototypes
Parser* parser_init(FILE* source, FILE* output);
void parser_destroy(Parser* parser);
int parse_program(Parser* parser);

// Token handling
void next_token(Parser* parser);
bool accept(Parser* parser, TokenType type);
bool expect(Parser* parser, TokenType type);
void error(Parser* parser, int code, const char* message);

// Grammar parsing functions
void parse_prolog(Parser* parser);
void parse_class(Parser* parser);
void parse_function_definitions(Parser* parser);
void parse_function(Parser* parser);
void parse_parameters(Parser* parser, SymbolData* func_data);
void parse_block(Parser* parser);
void parse_statement(Parser* parser);
void parse_var_declaration(Parser* parser);
void parse_assignment(Parser* parser);
void parse_if_statement(Parser* parser);
void parse_while_statement(Parser* parser);
void parse_function_call_statement(Parser* parser);
void parse_return(Parser* parser);

// Expression parsing (precedence-based)
void parse_expression(Parser* parser);
void parse_simple_expression(Parser* parser);
void parse_term(Parser* parser);
void parse_factor(Parser* parser);
void parse_relation(Parser* parser);
void parse_is_expression(Parser* parser);

// Function call parsing
void parse_function_call(Parser* parser, const char* func_name);
int parse_argument_list(Parser* parser);

// Getters and setters
void parse_getter(Parser* parser, const char* name);
void parse_setter(Parser* parser, const char* name);

// Semantic analysis
SymbolData* lookup_symbol(Parser* parser, const char* name);
bool check_redefinition(Parser* parser, const char* name, bool is_global);
bool check_type_compatibility(ifj25_type_t t1, ifj25_type_t t2, TokenType op);
ifj25_type_t get_expression_type(Parser* parser);

// Code generation
void generate_prolog(Parser* parser);
void generate_epilog(Parser* parser);
void generate_function_prolog(Parser* parser, const char* name, int param_count);
void generate_function_epilog(Parser* parser);
void generate_var_declaration(Parser* parser, const char* name, bool is_global);
void generate_assignment(Parser* parser, const char* name, bool is_global);
void generate_if(Parser* parser, const char* else_label, const char* end_label);
void generate_while(Parser* parser, const char* start_label, const char* end_label);
void generate_function_call(Parser* parser, const char* func_name, int arg_count, bool is_builtin);
void generate_return(Parser* parser);
void generate_expression_start(Parser* parser);
void generate_expression_end(Parser* parser);
void generate_binary_op(Parser* parser, TokenType op);
void generate_relational_op(Parser* parser, TokenType op);
void generate_is_op(Parser* parser, TokenType type_token);

// Helper functions
char* generate_label(Parser* parser);
char* generate_temp_var(Parser* parser);
void push_expr_stack(Parser* parser, const char* item);
char* pop_expr_stack(Parser* parser);

// Built-in function handling
bool is_builtin_function(const char* name);
int get_builtin_arity(const char* name);

#endif // PARSER_H
