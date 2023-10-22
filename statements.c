#include "statements.h"
#include "inputBuffer.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const uint32_t ID_SIZE = size_of_attribute(Row, id);
const uint32_t USERNAME_SIZE = size_of_attribute(Row, username);
const uint32_t EMAIL_SIZE = size_of_attribute(Row, email);
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

const uint32_t PAGE_SIZE = 4096;
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;

void* findRowSpot(Table* table, uint32_t rowNum){
    uint32_t pageNum = rowNum / ROWS_PER_PAGE;
    void* page = table->pages[pageNum];
    if (page == NULL){
        printf("Page was null. Mallocing space\n");
        table->pages[pageNum] = malloc(PAGE_SIZE);
        page = table->pages[pageNum];
    }
    uint32_t rowOffset = rowNum % ROWS_PER_PAGE;
    uint32_t byteOffset = rowOffset * ROW_SIZE;
    return page + byteOffset;
}

PrepareResult prepareStatement(inputBuffer* inputBfr, statement* smt) {
    if (!strncmp(inputBfr->buffer, "insert", 6)) {
        smt->type = STATEMENT_INSERT;

        char* keywords = strtok(inputBfr->buffer, " ");
        //TEMP
        char* idStr = strtok(NULL, " ");
        char* username = strtok(NULL, " ");
        char* email = strtok(NULL, " ");

        if (idStr == NULL || username == NULL || email == NULL){
            return PREPARE_COMMAND_SYNTAX_ERROR;
        }

        int id = atoi(idStr);
        if (id < 0){
            return PREPARE_COMMAND_NEGATIVE_ID;
        }
        if (strlen(username) > 254){
            return PREPARE_COMMAND_STRING_TOO_LONG;
        }
        if (strlen(email) > 254){
            return PREPARE_COMMAND_STRING_TOO_LONG;
        }

        smt->row.id = id;
        strcpy(smt->row.username, username);
        strcpy(smt->row.email, email);

        return PREPARE_COMMAND_SUCCESS;
    }
    if (!strncmp(inputBfr->buffer, "select", 6)){
        smt->type = STATEMENT_SELECT;
        return PREPARE_COMMAND_SUCCESS;
    }
    if (!strncmp(inputBfr->buffer, "delete", 6)) {
        smt->type = STATEMENT_DELETE;
        return PREPARE_COMMAND_SUCCESS;
    }
    return PREPARE_COMMAND_UNKNOWN_COMMAND;
}

ExecuteResult executeStatement(statement* smt, Table* table) {
    switch (smt->type) {
    case STATEMENT_INSERT:
        printf("Insert code.\n");
        Row* r = &smt->row;
        void* x = findRowSpot(table, table->numRows);
        serializeRow(r, x);
        table->numRows += 1;
        return EXECUTE_SUCCESS;
        break;
    case STATEMENT_SELECT:
        printf("Select code.\n");
        Row row;
        for (uint32_t i = 0; i < table->numRows; i++) {
            void* x = findRowSpot(table, i);
            deserializeRow(x, &row);
            printf("(%d, %s, %s)\n", row.id, row.username, row.email);
        }
        return EXECUTE_SUCCESS;
        break;
    case STATEMENT_DELETE:
        printf("Delete code.\n");
        return EXECUTE_SUCCESS;
        break;
    }
    return EXECUTE_FAIL;
}

MetaCommandResult metaCommand(inputBuffer* inputBfr, Table* table) {
    if (!strcmp(inputBfr->buffer, ".exit")) {
        closeInputBuffer(inputBfr);
        freeTable(table);
        return META_COMMAND_EXIT;
    } else {
        return META_COMMAND_UNKNOWN_COMMAND;
    }
}

void serializeRow(Row* src, void* dest){
    memcpy(dest + ID_OFFSET, &(src->id), ID_SIZE);
    memcpy(dest + USERNAME_OFFSET, &(src->username), USERNAME_SIZE);
    memcpy(dest + EMAIL_OFFSET, &(src->email), EMAIL_SIZE);
}

void deserializeRow(void* src, Row* dest){
    memcpy(&dest->id, src + ID_OFFSET, ID_SIZE);
    memcpy(&dest->username, src + USERNAME_OFFSET, USERNAME_SIZE);
    memcpy(&dest->email, src + EMAIL_OFFSET, EMAIL_SIZE);
}

Table* createTable(){
    Table* table = (Table*)malloc(sizeof(Table));
    table->numRows = 0;
    //Make all pages null
    for(uint32_t i = 0; i < TABLE_MAX_PAGES; i++){
        table->pages[i] = NULL;
    }
    return table;
}

void freeTable(Table* table){
    for (int i = 0; table->pages[i]; i++){
        free(table->pages[i]);
    }
    free(table);
}
