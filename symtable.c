#include "symtable.h"

/**
 * Initializes the symbol table
 * @param  table Pointer to the symbol table
 */
SymTable* symtable_init(void){
    SymTable *table = malloc(sizeof(SymTable));
    if (table == NULL) {
        return NULL; // Memory allocation failure
    }
    table->root = NULL;
    return table;
}

/**
 * Inserts a symbol into the symbol table
 * @param table symbol table to insert into
 * @param key key of the symbol
 * @param data data associated with the symbol
 * @return true on success, false on failure
 */
bool symtable_insert(SymTable *table, const char *key, SymbolData *data){
    if (table == NULL) {
        return false;
    }
    // Insert into BST
    return bst_insert(&table->root, key, data);
}

/**
 * Insert helper function
 * @param tree current BST node
 * @param key  key to insert
 * @param data data associated with the symbol
 * @return true on success, false on memory allocation failure
 */
static bool bst_insert(BSTNode** tree, const char* key, SymbolData* data){
    if (*tree == NULL) {
        // Create new node
        BSTNode* new_node = malloc(sizeof(BSTNode));
        if (new_node == NULL) {
            return false; // Memory allocation failure
        }
        // Allocate a copy of the key
        new_node->key = strdup(key);
        if (new_node->key == NULL) {
            free(new_node);
            return false; // Memory allocation failure
        }
        // Allocate a copy of SymbolData
        new_node->data = data;
        new_node->left = NULL;
        new_node->right = NULL;
        *tree = new_node;
        return true;
    }

    // Traverse left or right based on key comparison
    if (strcmp(key, (*tree)->key) < 0) {
        return bst_insert(&(*tree)->left, key, data);
    }
    if (strcmp(key, (*tree)->key) > 0) {
        return bst_insert(&(*tree)->right, key, data);
    }
    
    // Node found, rewrite existing data
    symdata_free((*tree)->data); // Free old data
    (*tree)->data = data;
    return true;
}

/**
 * Finds a symbol in the symbol table
 * @param table symbol table to search in
 * @param key key of the symbol to find
 * @param return_data pointer to found data, NULL if not found
 * @return true if symbol found, false otherwise
 */
bool symtable_find(SymTable *table, const char *key, SymbolData **return_data){
    if (return_data == NULL) {
        return false;
    }

    if (table == NULL || table->root == NULL) {
        *return_data = NULL;
        return false;
    }

  return bst_find(table->root, key, return_data);
}

/**
 * Search helper function
 * @param tree current BST node
 * @param key key to search for
 * @param return_data pointer to found data, NULL if not found
 * @return true if symbol found, false otherwise
 */
static bool bst_find(BSTNode* tree, const char* key, SymbolData** return_data) {
    if (tree == NULL) {
        *return_data = NULL;
        return false;
    }
    
    // Traverse left or right based on key comparison
    if (strcmp(key, tree->key) < 0) {
        return bst_find(tree->left, key, return_data);
    }
    if (strcmp(key, tree->key) > 0) {
        return bst_find(tree->right, key, return_data);
    }
    
    // Node found
    *return_data = tree->data;
    return true;
}

/**
 * Deletes a symbol from the symbol table
 * @param table symbol table to delete from
 * @param key key of the symbol to delete
 * @return true if symbol deleted, false if not found
 */
bool symtable_delete(SymTable *table, const char *key){
    if (table == NULL || table->root == NULL) {
        return false;
    }
    return bst_delete(&table->root, key);
}

/**
 * Delete helper function
 * @param tree current BST node
 * @param key key to delete
 * @return true if symbol deleted, false if not found
 */
static bool bst_delete(BSTNode** tree, const char* key){
    if (tree == NULL || *tree == NULL) {
        return false;
    }

    if (strcmp(key, (*tree)->key) < 0) {
        return bst_delete(&(*tree)->left, key);
    }
    if (strcmp(key, (*tree)->key) > 0) {
        return bst_delete(&(*tree)->right, key);
    }

    // Node found
    BSTNode* node_to_delete = *tree;

    // No children
    if (node_to_delete->left == NULL && node_to_delete->right == NULL) {
        symdata_free(node_to_delete->data);
        free(node_to_delete->key);
        free(node_to_delete);
        *tree = NULL;
        return true;
    }

    // One child (right)
    if (node_to_delete->left == NULL) {
        *tree = node_to_delete->right;
        symdata_free(node_to_delete->data);
        free(node_to_delete->key);
        free(node_to_delete);
        return true;
    }

    // One child (left)
    if (node_to_delete->right == NULL) { 
        *tree = node_to_delete->left;
        symdata_free(node_to_delete->data);
        free(node_to_delete->key);
        free(node_to_delete);
        return true;
    }

    // Two children
    return bst_replace_by_rightmost(node_to_delete, &node_to_delete->left);
}

/**
 * bst_delete helper function to free node with two children
 * @param target node to replace
 * @param tree subtree to find rightmost node
 * @return true on success
 */
static bool bst_replace_by_rightmost(BSTNode* target, BSTNode** tree){
    if (tree == NULL || *tree == NULL) {
        return false;
    }

    if ((*tree)->right != NULL) {
        return bst_replace_by_rightmost(target, &(*tree)->right);
    }

    // Rightmost node found
    // Transfer key and data
    free(target->key);
    symdata_free(target->data);
    target->key = (*tree)->key;
    target->data = (*tree)->data;
    
    // Remove rightmost node
    BSTNode* node_to_delete = *tree;
    *tree = (*tree)->left; // Move left child up
    free(node_to_delete);

    return true;
}

/**
 * Frees the entire symbol table
 * @param table symbol table to free
 */
void symtable_free(SymTable *table){
    if (table == NULL) {
        return;
    }
    // Free all nodes in the BST
    bst_free(table->root);
    table->root = NULL;
    free(table);
}
/**
 * Frees BST nodes recursively
 * @param tree current BST node
 */
static void bst_free(BSTNode* tree){
    if (tree == NULL) {
        return;
    }
    bst_free(tree->left);
    bst_free(tree->right);
    symdata_free(tree->data);
    free(tree->key);
    free(tree);
}

/**
 * Helper to create symbol data for variable
 * @param type variable type
 * @return pointer to created SymbolData, NULL on failure
 */
SymbolData* symdata_create_var(ifj25_type_t type){
    SymbolData *data = malloc(sizeof(SymbolData));
    if (data == NULL) {
        return NULL; // Memory allocation failure
    }
    data->kind = IFJ_SYMBOL_VAR;
    data->var = malloc(sizeof(VarData));
    if (data->var == NULL) {
        free(data);
        return NULL; // Memory allocation failure
    }
    data->var->type = type;
    data->func = NULL;
    return data;
}

/**
 * Helper to create symbol data for function/getter/setter
 * @param kind symbol kind (function/getter/setter)
 * @param arity number of parameters
 * @return pointer to created SymbolData, NULL on failure
 */
SymbolData* symdata_create_func(ifj25_symbol_kind_t kind, int arity){
    SymbolData *data = malloc(sizeof(SymbolData));
    if (data == NULL) {
        return NULL; // Memory allocation failure
    }
    data->kind = kind;
    data->func = malloc(sizeof(FuncData));
    if (data->func == NULL) {
        free(data);
        return NULL; // Memory allocation failure
    }
    data->func->arity = arity;
    data->func->params = NULL;
    data->var = NULL;
    return data;
}

/**
 * Frees symbol data
 * @param data symbol data to free
 */
static void symdata_free(SymbolData *data){
    if (data == NULL) {
        return;
    }
    if (data->kind == IFJ_SYMBOL_VAR) {
        // Free variable-specific data
        if (data->var != NULL) {
            if (data->var->type == IFJ_TYPE_STRING && data->var->value.str != NULL) {
                free(data->var->value.str);
            }
            free(data->var);
            data->var = NULL;
        }
    } else {
        // Free function-specific data
        if (data->func != NULL) {
            while(data->func->params != NULL) {
                Param *temp = data->func->params;
                data->func->params = data->func->params->next;
                free(temp->name);
                free(temp);
            }
            free(data->func);
            data->func = NULL;
        }
    }
}
