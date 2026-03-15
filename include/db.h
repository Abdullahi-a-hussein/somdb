#ifndef SOMDB_DB_H
#define SOMDB_DB_H

#include "table.h"

typedef struct
{
    Table *tables;
    size_t table_count;
    size_t capacity;
} Database;

SomDBStatus db_init(Database *db);
void db_free(Database *db);
SomDBStatus db_create_table(Database *db, const Schema *schema);
Table *db_find_table(Database *db, const char *name);
const Table *db_find_table_const(const Database *db, const char *name);
void db_list_tables(const Database *db);

#endif
