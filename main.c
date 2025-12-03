/**
 * Project: Implementace překladače jazyka IFJ25.
 *
 * main.c
 *
 * @author Jakub Gono - xgonoja00
 * @author Vojtěch Kabelka - xkabelv00
 * @author Filip Kachyňa - xkachyf00
 * @author Martin Metelka - xmetelm00
 */
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    // The compiler reads from stdin and writes to stdout
    FILE* source = stdin;
    FILE* output = stdout;
    
    // Initialize parser
    Parser* parser = parser_init(source, output);
    if (!parser) {
        fprintf(stderr, "Failed to initialize parser\n");
        return INTERNAL_ERROR;
    }
    
    // Parse the program
    int result = parse_program(parser);
    
    // Clean up
    parser_destroy(parser);
    
    return result;
}
