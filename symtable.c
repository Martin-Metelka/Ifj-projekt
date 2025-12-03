#include "symtable.h"
#include <stdlib.h>
#include <string.h>

// Static helper function declarations
static BSTNode* bst_insert(BSTNode **tree, const char *key, SymbolData *data);
static bool bst_find(BSTNode *tree, const char *key, SymbolData **return_data);
static bool bst_delete(BSTNode **tree, const char *key);
static bool bst_replace_by_rightmost(BSTNode *target, BSTNode **tree);
static void bst_free(BSTNode *tree);
static char* my_strdup(const char* s);
static SymbolData* symdata_dup(SymbolData* data);
static int height(BSTNode *n);
static int max(int a, int b);
static int get_balance(BSTNode *n);
static BSTNode* rotate_right(BSTNode *y);
static BSTNode* rotate_left(BSTNode *y);
static BSTNode* balance_node(BSTNode *node);


/**
 * Duplicates a string
 * @param s string to duplicate
 * @return pointer to duplicated string, NULL on failure
 */
static char* my_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char *copy = malloc(len);
    if (copy) memcpy(copy, s, len);
    return copy;
}

/**
 * Duplicates SymbolData
 * @param data SymbolData to duplicate
 * @return pointer to duplicated SymbolData, NULL on failure
 */
static SymbolData* symdata_dup(SymbolData* data){
    if (!data) return NULL;

    SymbolData* copy = malloc(sizeof(SymbolData));
    if (!copy) return NULL;

    copy->kind = data->kind;
    if (data->kind == IFJ_SYMBOL_VAR) {
        copy->var = malloc(sizeof(VarData));
        if (!copy->var) { free(copy); return NULL; }
        copy->var->type = data->var->type;
        if (data->var->type == IFJ_TYPE_STRING && data->var->value.str) {
            copy->var->value.str = my_strdup(data->var->value.str);
        } else {
            copy->var->value = data->var->value;
        }
        copy->func = NULL;
    } else {
        copy->func = malloc(sizeof(FuncData));
        if (!copy->func) { free(copy); return NULL; }
        copy->func->arity = data->func->arity;
        copy->func->params = NULL;

        // Deep copy parameters
        Param* last = NULL;
        for (Param* p = data->func->params; p; p = p->next){
            Param* new_param = malloc(sizeof(Param));
            new_param->name = my_strdup(p->name);
            new_param->next = NULL;
            if (last) last->next = new_param;
            else copy->func->params = new_param;
            last = new_param;
        }
        copy->var = NULL;
    }

    return copy;
}

/**
 * Gets the height of a BST node
 * @param n BST node
 * @return height of the node, 0 if NULL
 */
static int height(BSTNode *n) {
    return n ? n->height : 0;
}

/**
 * Gets the maximum of two integers
 * @param a first integer
 * @param b second integer
 * @return maximum of a and b
 */
static int max(int a, int b) {
    return (a > b) ? a : b;
}

/**
 * Gets the balance factor of a BST node
 * @param n BST node
 * @return balance factor (height of left subtree - height of right subtree)
 */
static int get_balance(BSTNode *n) {
    return n ? height(n->left) - height(n->right) : 0;
}

/**
 * Performs a right rotation on the given BST node
 * @param y BST node to rotate
 * @return new root after rotation
 */
/*
     y              x
    / \            / \
   x   T3   ->    T1  y
  / \                / \
 T1 T2              T2 T3
*/
static BSTNode* rotate_right(BSTNode *y) {
    BSTNode *x = y->left;
    BSTNode *T2 = x->right;

    // Perform rotation
    x->right = y;
    y->left = T2;

    // Update heights
    y->height = max(height(y->left), height(y->right)) + 1;
    x->height = max(height(x->left), height(x->right)) + 1;

    return x; // new root
}

/**
 * Performs a left rotation on the given BST node
 * @param y BST node to rotate
 * @return new root after rotation
 */
/*
   y                x
  / \              / \
 T1  x     ->     y  T3
    / \          / \
   T2 T3        T1 T2
*/
static BSTNode* rotate_left(BSTNode *y) {
    BSTNode *x = y->right;
    BSTNode *T2 = x->left;

    // Perform rotation
    x->left = y;
    y->right = T2;

    // Update heights
    y->height = max(height(y->left), height(y->right)) + 1;
    x->height = max(height(x->left), height(x->right)) + 1;

    return x; // new root
}

/**
 * Balances the given BST node
 * @param node BST node to balance
 * @return new root after balancing
 */
static BSTNode* balance_node(BSTNode *node) {
    int balance = get_balance(node);

    // LL case
    if (balance > 1 && get_balance(node->left) >= 0) {
        return rotate_right(node);
    }

    // LR case
    if (balance > 1 && get_balance(node->left) < 0) {
        node->left = rotate_left(node->left);
        return rotate_right(node);
    }

    // RR case
    if (balance < -1 && get_balance(node->right) <= 0) {
        return rotate_left(node);
    }

    // RL case
    if (balance < -1 && get_balance(node->right) > 0) {
        node->right = rotate_right(node->right);
        return rotate_left(node);
    }

    return node; // no rebalance needed
}

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
    return bst_insert(&table->root, key, data) != NULL;
}

/**
 * Insert helper function
 * @param tree current BST node
 * @param key  key to insert
 * @param data data associated with the symbol
 * @return pointer to inserted node, NULL on failure
 */
static BSTNode* bst_insert(BSTNode** tree, const char* key, SymbolData* data){
    if (*tree == NULL) {
        // Create new node
        BSTNode* new_node = malloc(sizeof(BSTNode));
        if (new_node == NULL) {
            return NULL; // Memory allocation failure
        }
        // Allocate a copy of the key
        new_node->key = my_strdup(key);
        if (new_node->key == NULL) {
            free(new_node);
            return NULL; // Memory allocation failure
        }
        // Allocate a copy of SymbolData
        new_node->data = data;
        new_node->left = NULL;
        new_node->right = NULL;
        new_node->height = 1; // New node is initially added at leaf
        *tree = new_node;
        return *tree;
    }

    // Traverse left or right based on key comparison
    if (strcmp(key, (*tree)->key) < 0) {
        (*tree)->left = bst_insert(&(*tree)->left, key, data);
    } else if (strcmp(key, (*tree)->key) > 0) {
        (*tree)->right = bst_insert(&(*tree)->right, key, data);
    } else {
        // Node found, rewrite existing data
        symdata_free((*tree)->data); // Free old data
        (*tree)->data = data;
        return *tree;
    }
    // Update height
    (*tree)->height = 1 + max(height((*tree)->left), height((*tree)->right));
    *tree = balance_node(*tree);
    return *tree;
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
        bool deleted = bst_delete(&(*tree)->left, key);
        if (!deleted) return deleted;
    } else if (strcmp(key, (*tree)->key) > 0) {
        bool deleted = bst_delete(&(*tree)->right, key);
        if (!deleted) return deleted;
    } else {
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
        else if (node_to_delete->left == NULL) {
            *tree = node_to_delete->right;
            symdata_free(node_to_delete->data);
            free(node_to_delete->key);
            free(node_to_delete);
        }
        // One child (left)
        else if (node_to_delete->right == NULL) { 
            *tree = node_to_delete->left;
            symdata_free(node_to_delete->data);
            free(node_to_delete->key);
            free(node_to_delete);
        } 
        else {
            // Two children
            bool replaced = bst_replace_by_rightmost(node_to_delete, &node_to_delete->left);
            if (!replaced) {
                return false; // Failure in replacement
            }
        }
    }
    // Update height and balance
    if (*tree != NULL) {
        (*tree)->height = 1 + max(height((*tree)->left), height((*tree)->right));
        *tree = balance_node(*tree);    
    }

    return true;
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
    
    bool replaced = false;
    if ((*tree)->right != NULL) {
        replaced = bst_replace_by_rightmost(target, &(*tree)->right);
    } else {
        // Rightmost node found
        // Transfer key and data
        BSTNode* node_to_delete = *tree;
        free(target->key);
        symdata_free(target->data);
        target->key = my_strdup(node_to_delete->key);
        target->data = symdata_dup(node_to_delete->data);
    
        // Remove rightmost node
        *tree = node_to_delete->left;
        symdata_free(node_to_delete->data);
        free(node_to_delete->key);
        free(node_to_delete);
        replaced = true;
    }
    // Update height and balance
    if (*tree != NULL) {
        (*tree)->height = 1 + max(height((*tree)->left), height((*tree)->right));
        *tree = balance_node(*tree);    
    }
    return replaced;
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
void symdata_free(SymbolData *data){
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
