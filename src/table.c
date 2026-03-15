#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "table.h"

static SomDBStatus table_grow(Table *table)
{
    size_t new_capacity = table->capacity == 0 ? SOMDB_INITIAL_ROW_CAPACITY : table->capacity * 2;
    Row *new_rows = realloc(table->rows, new_capacity * sizeof(Row));
    if (!new_rows)
    {
        return SOMDB_ERR_MEMORY;
    }
    table->rows = new_rows;
    table->capacity = new_capacity;
    return SOMDB_OK;
}

SomDBStatus table_init(Table *table, const Schema *schema)
{
    memset(table, 0, sizeof(*table));
    table->schema = *schema;
    return SOMDB_OK;
}

void table_free(Table *table)
{
    if (!table)
    {
        return;
    }
    for (size_t i = 0; i < table->row_count; i++)
    {
        row_free(&table->rows[i]);
    }
    free(table->rows);
    table->rows = NULL;
    table->row_count = 0;
    table->capacity = 0;
}

SomDBStatus table_insert(Table *table, const Row *row)
{
    if (row->value_count != table->schema.column_count)
    {
        return SOMDB_ERR_SCHEMA;
    }

    for (int i = 0; i < row->value_count; i++)
    {
        if (row->values[i].type != table->schema.columns[i].type)
        {
            return SOMDB_ERR_TYPE;
        }
    }

    if (table->row_count == table->capacity)
    {
        SomDBStatus grow_status = table_grow(table);
        if (grow_status != SOMDB_OK)
        {
            return grow_status;
        }
    }

    SomDBStatus clone_status = row_clone(&table->rows[table->row_count], row);
    if (clone_status != SOMDB_OK)
    {
        return clone_status;
    }

    table->row_count++;
    return SOMDB_OK;
}

void table_print_all(const Table *table)
{
    int indices[SOMDB_MAX_COLUMNS];
    for (int i = 0; i < table->schema.column_count; i++)
    {
        indices[i] = i;
    }
    table_print_projection(table, indices, table->schema.column_count, 0, -1, NULL);
}

void table_print_projection(const Table *table, const int *indices, int count, int has_where, int where_index, const Value *where_value)
{
    for (int i = 0; i < count; i++)
    {
        printf("%s", table->schema.columns[indices[i]].name);
        if (i + 1 < count)
        {
            printf(" | ");
        }
    }
    printf("\n");

    for (int i = 0; i < count; i++)
    {
        printf("----");
        if (i + 1 < count)
        {
            printf("+");
        }
    }
    printf("\n");

    for (size_t r = 0; r < table->row_count; r++)
    {
        if (has_where)
        {
            if (!value_equals(&table->rows[r].values[where_index], where_value))
            {
                continue;
            }
        }
        row_print_projected(&table->rows[r], &table->schema, indices, count);
    }
}
