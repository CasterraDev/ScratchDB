#include "statements.h"
#include "inputBuffer.h"
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

void* findRowSpot(table* table, uint32_t rowNum) {
    uint32_t pageNum = rowNum / ROWS_PER_PAGE;
    void* page = getPage(table->pager, pageNum);
    uint32_t rowOffset = rowNum % ROWS_PER_PAGE;
    uint32_t byteOffset = rowOffset * ROW_SIZE;
    return page + byteOffset;
}

PrepareResult prepareStatement(inputBuffer* inputBfr, statement* smt) {
    if (!strncmp(inputBfr->buffer, "insert", 6)) {
        smt->type = STATEMENT_INSERT;

        char* keywords = strtok(inputBfr->buffer, " ");
        // TEMP
        char* idStr = strtok(NULL, " ");
        char* username = strtok(NULL, " ");
        char* email = strtok(NULL, " ");

        if (idStr == NULL || username == NULL || email == NULL) {
            return PREPARE_COMMAND_SYNTAX_ERROR;
        }

        int id = atoi(idStr);
        if (id < 0) {
            return PREPARE_COMMAND_NEGATIVE_ID;
        }
        if (strlen(username) > 254) {
            return PREPARE_COMMAND_STRING_TOO_LONG;
        }
        if (strlen(email) > 254) {
            return PREPARE_COMMAND_STRING_TOO_LONG;
        }

        smt->row.id = id;
        strcpy(smt->row.username, username);
        strcpy(smt->row.email, email);

        return PREPARE_COMMAND_SUCCESS;
    }
    if (!strncmp(inputBfr->buffer, "select", 6)) {
        smt->type = STATEMENT_SELECT;
        return PREPARE_COMMAND_SUCCESS;
    }
    if (!strncmp(inputBfr->buffer, "delete", 6)) {
        smt->type = STATEMENT_DELETE;
        return PREPARE_COMMAND_SUCCESS;
    }
    return PREPARE_COMMAND_UNKNOWN_COMMAND;
}

ExecuteResult executeStatement(statement* smt, table* table) {
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

MetaCommandResult metaCommand(inputBuffer* inputBfr, table* table) {
    if (!strcmp(inputBfr->buffer, ".exit")) {
        closeInputBuffer(inputBfr);
        dbClose(table);
        return META_COMMAND_EXIT;
    } else {
        return META_COMMAND_UNKNOWN_COMMAND;
    }
}

void serializeRow(Row* src, void* dest) {
    memcpy(dest + ID_OFFSET, &(src->id), ID_SIZE);
    strncpy(dest + USERNAME_OFFSET, src->username, USERNAME_SIZE);
    strncpy(dest + EMAIL_OFFSET, src->email, EMAIL_SIZE);
}

void deserializeRow(void* src, Row* dest) {
    memcpy(&dest->id, src + ID_OFFSET, ID_SIZE);
    memcpy(&dest->username, src + USERNAME_OFFSET, USERNAME_SIZE);
    memcpy(&dest->email, src + EMAIL_OFFSET, EMAIL_SIZE);
}

pager* pagerOpen(const char* filename) {
    // Open file
    int fd = open(
        filename,
        O_RDWR |
            O_CREAT, // Read/Write mode, Create the file if it doesn't exist
        S_IWUSR | S_IRUSR); // Let user have write/read permission

    if (fd == -1) {
        printf("Unable to open file %s.", filename);
        exit(1);
    }

    off_t fileLen = lseek(fd, 0, SEEK_END);

    pager* pg = malloc(sizeof(struct pager));
    pg->fileDescriptor = fd;
    pg->fileLen = fileLen;
    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        pg->pages[i] = NULL;
    }
    return pg;
}

void* getPage(pager* pager, uint32_t pageNum){
    if (pageNum > TABLE_MAX_PAGES){
        printf("Tried to get page out of table bounds.\n");
        exit(1);
    }

    if (pager->pages[pageNum] == NULL){
        //Cache miss. Allocate memory and retrieve from file
        void* page = malloc(PAGE_SIZE);
        uint32_t numPages = pager->fileLen / PAGE_SIZE;

        //If theres a partial page at the end. Add one
        if (pager->fileLen % PAGE_SIZE){
            numPages += 1;
        }

        if (pageNum <= numPages){
            lseek(pager->fileDescriptor, pageNum * PAGE_SIZE, SEEK_SET);
            ssize_t bytesRead = read(pager->fileDescriptor, page, PAGE_SIZE);
            if (bytesRead == -1){
                printf("Error reading file %d.\n", errno);
                exit(1);
            }
        }
        pager->pages[pageNum] = page;
    }
    return pager->pages[pageNum];
}

void pagerFlush(pager* pg, uint32_t idx, uint32_t size) {
    if (pg->pages[idx] == NULL) {
        printf("Tried to flush empty page\n");
        exit(1);
    }

    off_t offset = lseek(pg->fileDescriptor, idx * PAGE_SIZE, SEEK_SET);

    if (offset == -1) {
        printf("Error while seeking.\n");
        exit(1);
    }

    ssize_t bytesWritten = write(pg->fileDescriptor, pg->pages[idx], size);

    if (bytesWritten == -1) {
        printf("Error writing: %d\n", errno);
        exit(1);
    }
}

table* dbOpen(const char* filename) {
    pager* pg = pagerOpen(filename);
    uint32_t numRows = pg->fileLen / ROW_SIZE;
    table* table = malloc(sizeof(struct table));
    table->pager = pg;
    table->numRows = numRows;
    return table;
}

void dbClose(table* table) {
    pager* pgr = table->pager;
    uint32_t numFullPages = table->numRows / ROWS_PER_PAGE;

    for (int i = 0; i < numFullPages; i++) {
        if (pgr->pages[i] == NULL) {
            continue;
        }
        pagerFlush(pgr, i, PAGE_SIZE);
        free(pgr->pages[i]);
        pgr->pages[i] = NULL;
    }

    uint32_t leftOverRows = table->numRows % ROWS_PER_PAGE;
    if (leftOverRows > 0) {
        if (pgr->pages[numFullPages] != NULL) {
            pagerFlush(pgr, numFullPages, leftOverRows * ROW_SIZE);
            free(pgr->pages[numFullPages]);
            pgr->pages[numFullPages] = NULL;
        }
    }

    int res = close(pgr->fileDescriptor);
    if (res == -1){
        printf("Error closing file.\n");
        exit(1);
    }

    //Null all pages
    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++){
        void* p = pgr->pages[i];
        if (p){
            free(p);
            pgr->pages[i] = NULL;
        }
    }

    free(pgr);
    free(table);
}
