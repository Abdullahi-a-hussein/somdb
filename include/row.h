#ifndef SOMDB_ROW_H
#define SOMDB_ROW_H

#include "schema.h"

typedef struct
{
    int value_count;
    Value *values;
} Row;

SomDBStatus row_init(Row *row, int value_count);
void row_free(Row *row);
SomDBStatus row_clone(Row *dest, const Row *src);
void row_print_projected(const Row *row, const Schema *schema, const int *indices, int count);

#endif
