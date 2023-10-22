#pragma once
#include "inputBuffer.h"
#include <stdint.h>

#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)0)->Attribute)
#define TABLE_MAX_PAGES 100

typedef enum StatementType {
    STATEMENT_INSERT,
    STATEMENT_SELECT,
    STATEMENT_DELETE,
} StatementType;

typedef struct Row {
    int id;
    char username[255];
    char email[255];
} Row;

typedef struct statement {
    StatementType type;
    Row row;
} statement;

typedef struct pager {
    int fileDescriptor;
    uint32_t fileLen;
    void* pages[TABLE_MAX_PAGES];
} pager;

typedef struct table {
    int numRows;
    pager* pager;
} table;

typedef enum ExecuteResult {
    EXECUTE_SUCCESS,
    EXECUTE_TABLE_FULL,
    EXECUTE_FAIL
} ExecuteResult;

typedef enum {
    PREPARE_COMMAND_SUCCESS,
    PREPARE_COMMAND_STRING_TOO_LONG,
    PREPARE_COMMAND_NEGATIVE_ID,
    PREPARE_COMMAND_SYNTAX_ERROR,
    PREPARE_COMMAND_FAIL,
    PREPARE_COMMAND_UNKNOWN_COMMAND
} PrepareResult;

typedef enum {
    META_COMMAND_SUCCESS,
    META_COMMAND_EXIT,
    META_COMMAND_UNKNOWN_COMMAND
} MetaCommandResult;

void serializeRow(Row* src, void* dest);

void deserializeRow(void* src, Row* dest);
MetaCommandResult metaCommand(inputBuffer* inputBfr, table* table);

PrepareResult prepareStatement(inputBuffer* inputBfr, statement* smt);

ExecuteResult executeStatement(statement* smt, table* table);

pager* pagerOpen(const char* filename);
void* getPage(pager* pager, uint32_t pageNum);
void pagerFlush(pager* pg, uint32_t idx, uint32_t size);

void dbClose(table* table);

table* dbOpen(const char* filename);
