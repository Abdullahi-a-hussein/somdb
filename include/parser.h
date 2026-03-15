#ifndef SOMDB_PARSER_H
#define SOMDB_PARSER_H

#include "storage.h"

typedef struct
{
    char table_name[SOMDB_MAX_NAME_LEN];
    int column_count;
    Column columns[SOMDB_MAX_COLUMNS];
} CreateTableStmt;

typedef struct
{
    char table_name[SOMDB_MAX_NAME_LEN];
    int value_count;
    Value *values;
} InsertStmt;

typedef struct
{
    char table_name[SOMDB_MAX_NAME_LEN];
    int select_all;
    int selected_column_count;
    char selected_columns[SOMDB_MAX_COLUMNS][SOMDB_MAX_NAME_LEN];
    int has_where;
    char where_column[SOMDB_MAX_NAME_LEN];
    Value where_value;
} SelectStmt;

typedef enum
{
    STMT_NONE = 0,
    STMT_CREATE_TABLE,
    STMT_INSERT,
    STMT_SELECT,
    STMT_META_EXIT,
    STMT_META_TABLES,
    STMT_META_SCHEMA
} StatementType;

typedef struct
{
    StatementType type;
    union
    {
        CreateTableStmt create_table;
        InsertStmt insert;
        SelectStmt select;
        char meta_table_name[SOMDB_MAX_NAME_LEN];
    } as;
} Statement;

SomDBStatus parse_statement(const char *input, Statement *stmt);
void statement_free(Statement *stmt);
SomDBStatus execute_statement(Database *db, Statement *stmt);

#endif
