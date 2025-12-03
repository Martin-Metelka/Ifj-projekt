#ifndef SYMTABLE_H
#define SYMTABLE_H

#include <stdbool.h>
#include <stddef.h>

// Data types in IFJ25
typedef enum ifj25_type_t {
    IFJ_TYPE_NULL,
    IFJ_TYPE_NUM,
    IFJ_TYPE_STRING,
    IFJ_TYPE_BOOL,
    IFJ_TYPE_UNDEF // for uninitialized variables possibly just error
} ifj25_type_t;

// Symbol kinds
typedef enum ifj25_symbol_kind_t {
    IFJ_SYMBOL_VAR,
    IFJ_SYMBOL_FUNC,
    IFJ_SYMBOL_GETTER,
    IFJ_SYMBOL_SETTER
} ifj25_symbol_kind_t;

// Function parameter list
typedef struct Param {
    char *name;
    struct Param *next;
} Param;

// Function signature
typedef struct FuncData {
    int arity;
    Param *params;
} FuncData;

// Variable data
typedef struct VarData {
    ifj25_type_t type;
    union {
        double num;
        char *str;
        bool boolean;
    } value;
} VarData;

// Symbol data union
typedef struct SymbolData {
    ifj25_symbol_kind_t kind;
    VarData *var;        // for variables
    FuncData *func;      // for functions/getters/setters
} SymbolData;

// BST Node
typedef struct BSTNode {
    char *key; // identifier
    SymbolData *data;
    int height; // for balancing
    struct BSTNode *left;
    struct BSTNode *right;
} BSTNode;

// Symbol table (root of BST)
typedef struct SymTable {
    BSTNode *root;
} SymTable;

// Initialize a symbol table
SymTable* symtable_init(void);

// Insert a symbol
bool symtable_insert(SymTable *table, const char *key, SymbolData *data);

// Find a symbol
bool symtable_find(SymTable *table, const char *key, SymbolData **return_data);

// Delete a symbol
bool symtable_delete(SymTable *table, const char *key);

// Free the entire table
void symtable_free(SymTable *table);

// Helper to create symbol data for variable
SymbolData* symdata_create_var(ifj25_type_t type);

// Helper to create symbol data for function/getter/setter
SymbolData* symdata_create_func(ifj25_symbol_kind_t kind, int arity);

// Free symbol data
void symdata_free(SymbolData *data);

#endif // SYMTABLE_H

/*
SymTable
  |
  └── BSTNode* root
          |
          ├── char* key        <-- identifier
          ├── SymbolData* data
          |       |
          |       ├── ifj25_symbol_kind_t kind
          |       ├── VarData* var       <-- if kind == IFJ_SYMBOL_VAR
          |       |       |
          |       |       ├── ifj25_type_t type
          |       |       └── union value
          |       |               ├── double num    <-- if type == IFJ_TYPE_NUM
          |       |               ├── char* str     <-- if type == IFJ_TYPE_STRING
          |       |               ├── bool boolean <-- if type == IFJ_TYPE_BOOL
          |       |               └── NULL         <-- if type == IFJ_TYPE_NULL; IFJ_TYPE_UNDEF
          |       |
          |       └── FuncData* func  <-- if kind == IFJ_SYMBOL_FUNC/GETTER/SETTER
          |               |
          |               ├── int arity
          |               └── Param* params
          |                       |
          |                       ├── char* name
          |                       └── Param* next
          |
          ├── BSTNode* left
          └── BSTNode* right
*/
