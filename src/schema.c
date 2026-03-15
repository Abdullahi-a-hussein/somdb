#include <stdio.h>
#include <string.h>
#include "schema.h"

void schema_init(Schema *schema, const char *table_name)
{
    memset(schema, 0, sizeof(*schema));
    strncpy(schema->table_name, table_name, SOMDB_MAX_NAME_LEN - 1);
    schema->table_name[SOMDB_MAX_NAME_LEN - 1] = '\0';
}

SomDBStatus schema_add_column(Schema *schema, const char *name, ColumnType type)
{
    if (schema->column_count >= SOMDB_MAX_COLUMNS)
    {
        return SOMDB_ERR_SCHEMA;
    }
    if (schema_find_column(schema, name) >= 0)
    {
        return SOMDB_ERR_DUPLICATE;
    }

    Column *col = &schema->columns[schema->column_count++];
    strncpy(col->name, name, SOMDB_MAX_NAME_LEN - 1);
    col->name[SOMDB_MAX_NAME_LEN - 1] = '\0';
    col->type = type;

    return SOMDB_OK;
}

int schema_find_column(const Schema *schema, const char *name)
{
    for (int i = 0; i < schema->column_count; i++)
    {
        if (strcmp(schema->columns[i].name, name) == 0)
        {
            return i;
        }
    }
    return -1;
}

void schema_print(const Schema *schema)
{
    printf("CREATE TABLE %s (", schema->table_name);
    for (int i = 0; i < schema->column_count; i++)
    {
        printf("%s %s", schema->columns[i].name, column_type_name(schema->columns[i].type));
        if (i + 1 < schema->column_count)
        {
            printf(", ");
        }
    }
    printf(");\n");
}
