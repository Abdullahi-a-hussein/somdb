#include <stdio.h>
#include <stdlib.h>
#include "row.h"

SomDBStatus row_init(Row *row, int value_count)
{
    row->value_count = value_count;
    row->values = calloc((size_t)value_count, sizeof(Value));
    if (!row->values)
    {
        return SOMDB_ERR_MEMORY;
    }
    return SOMDB_OK;
}

void row_free(Row *row)
{
    if (!row || !row->values)
    {
        return;
    }
    for (int i = 0; i < row->value_count; i++)
    {
        value_free(&row->values[i]);
    }
    free(row->values);
    row->values = NULL;
    row->value_count = 0;
}

SomDBStatus row_clone(Row *dest, const Row *src)
{
    SomDBStatus status = row_init(dest, src->value_count);
    if (status != SOMDB_OK)
    {
        return status;
    }
    for (int i = 0; i < src->value_count; i++)
    {
        dest->values[i] = value_clone(&src->values[i]);
        if (!dest->values[i].is_null && dest->values[i].type == TYPE_TEXT && !dest->values[i].as.text_val)
        {
            row_free(dest);
            return SOMDB_ERR_MEMORY;
        }
    }
    return SOMDB_OK;
}

void row_print_projected(const Row *row, const Schema *schema, const int *indices, int count)
{
    (void)schema;
    for (int i = 0; i < count; i++)
    {
        value_print(&row->values[indices[i]]);
        if (i + 1 < count)
        {
            printf(" | ");
        }
    }
    printf("\n");
}
