#ifndef SOMDB_SCHEMA_H
#define SOMDB_SCHEMA_H

#include "value.h"

typedef struct
{
    char name[SOMDB_MAX_NAME_LEN];
    ColumnType type;
} Column;

typedef struct
{
    char table_name[SOMDB_MAX_NAME_LEN];
    int column_count;
    Column columns[SOMDB_MAX_COLUMNS];
} Schema;

void schema_init(Schema *schema, const char *table_name);
SomDBStatus schema_add_column(Schema *schema, const char *name, ColumnType type);
int schema_find_column(const Schema *schema, const char *name);
void schema_print(const Schema *schema);

#endif
