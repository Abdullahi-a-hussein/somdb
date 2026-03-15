#ifndef SOMDB_TABLE_H
#define SOMDB_TABLE_H

#include "row.h"

typedef struct
{
    Schema schema;
    Row *rows;
    size_t row_count;
    size_t capacity;
} Table;

SomDBStatus table_init(Table *table, const Schema *schema);
void table_free(Table *table);
SomDBStatus table_insert(Table *table, const Row *row);
void table_print_all(const Table *table);
void table_print_projection(const Table *table, const int *indices, int count, int has_where, int where_index, const Value *where_value);

#endif
