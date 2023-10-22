#include "inputBuffer.h"
#include "statements.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void printPrompt() { printf("Scratch > "); }

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Must give a filename for the DB.\n");
        exit(1);
    }

    table* table = dbOpen(argv[1]);
    inputBuffer* inputBfr = inputBufferCreate();
    while (1) {
        printPrompt();
        readInput(inputBfr);

        // Check if the command is a meta command. (preceded by a .)
        if (inputBfr->buffer[0] == '.') {
            switch (metaCommand(inputBfr, table)) {
            case META_COMMAND_SUCCESS:
                continue;
            case META_COMMAND_EXIT:
                exit(1);
            case META_COMMAND_UNKNOWN_COMMAND:
                printf("Unknown command %s.\n", inputBfr->buffer);
                continue;
            }
        }

        // See what statement the user has typed
        statement statement;
        switch (prepareStatement(inputBfr, &statement)) {
        case PREPARE_COMMAND_SUCCESS:
            break;
        case PREPARE_COMMAND_SYNTAX_ERROR:
            printf("Syntax error %s.\n", inputBfr->buffer);
            continue;
        case PREPARE_COMMAND_STRING_TOO_LONG:
            printf("String too long %s.\n", inputBfr->buffer);
            continue;
        case PREPARE_COMMAND_NEGATIVE_ID:
            printf("Number must be positive.");
            continue;
        case PREPARE_COMMAND_FAIL:
            printf("Command failed.");
            continue;
        case PREPARE_COMMAND_UNKNOWN_COMMAND:
            printf("Unknown command %s.\n", inputBfr->buffer);
            continue;
        }
        ExecuteResult er = executeStatement(&statement, table);
        switch (er) {
        case EXECUTE_SUCCESS:
            printf("Executed.\n");
            break;
        case EXECUTE_TABLE_FULL:
            printf("Table is full.\n");
            break;
        case EXECUTE_FAIL:
            printf("Execute Failed.\n");
            break;
        }
    }
    return 0;
}
