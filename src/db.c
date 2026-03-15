#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "db.h"

static SomDBStatus db_grow(Database *db)
{
    size_t new_capacity = db->capacity == 0 ? SOMDB_INITIAL_TABLE_CAPACITY : db->capacity * 2;
    Table *new_tables = realloc(db->tables, new_capacity * sizeof(Table));
    if (!new_tables)
    {
        return SOMDB_ERR_MEMORY;
    }
    db->tables = new_tables;
    db->capacity = new_capacity;
    return SOMDB_OK;
}

SomDBStatus db_init(Database *db)
{
    db->tables = NULL;
    db->table_count = 0;
    db->capacity = 0;
    return SOMDB_OK;
}

void db_free(Database *db)
{
    if (!db)
    {
        return;
    }
    for (size_t i = 0; i < db->table_count; i++)
    {
        table_free(&db->tables[i]);
    }
    free(db->tables);
    db->tables = NULL;
    db->table_count = 0;
    db->capacity = 0;
}

Table *db_find_table(Database *db, const char *name)
{
    for (size_t i = 0; i < db->table_count; i++)
    {
        if (strcmp(db->tables[i].schema.table_name, name) == 0)
        {
            return &db->tables[i];
        }
    }
    return NULL;
}

const Table *db_find_table_const(const Database *db, const char *name)
{
    for (size_t i = 0; i < db->table_count; i++)
    {
        if (strcmp(db->tables[i].schema.table_name, name) == 0)
        {
            return &db->tables[i];
        }
    }
    return NULL;
}

SomDBStatus db_create_table(Database *db, const Schema *schema)
{
    if (db_find_table(db, schema->table_name))
    {
        return SOMDB_ERR_DUPLICATE;
    }

    if (db->table_count == db->capacity)
    {
        SomDBStatus grow_status = db_grow(db);
        if (grow_status != SOMDB_OK)
        {
            return grow_status;
        }
    }

    SomDBStatus init_status = table_init(&db->tables[db->table_count], schema);
    if (init_status != SOMDB_OK)
    {
        return init_status;
    }

    db->table_count++;
    return SOMDB_OK;
}

void db_list_tables(const Database *db)
{
    if (db->table_count == 0)
    {
        printf("(no tables)\n");
        return;
    }
    for (size_t i = 0; i < db->table_count; i++)
    {
        printf("%s\n", db->tables[i].schema.table_name);
    }
}
