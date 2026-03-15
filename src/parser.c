#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "parser.h"

static void trim(char *s)
{
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1]))
    {
        s[--len] = '\0';
    }

    char *start = s;
    while (*start && isspace((unsigned char)*start))
    {
        start++;
    }
    if (start != s)
    {
        memmove(s, start, strlen(start) + 1);
    }
}

static int starts_with(const char *s, const char *prefix)
{
    return strncmp(s, prefix, strlen(prefix)) == 0;
}

static int read_identifier(const char **p, char *out)
{
    int i = 0;
    while (**p && (isalnum((unsigned char)**p) || **p == '_'))
    {
        if (i < SOMDB_MAX_NAME_LEN - 1)
        {
            out[i++] = **p;
        }
        (*p)++;
    }
    out[i] = '\0';
    return i > 0;
}

static void skip_spaces(const char **p)
{
    while (**p && isspace((unsigned char)**p))
    {
        (*p)++;
    }
}

static SomDBStatus parse_literal(const char **p, Value *out)
{
    skip_spaces(p);
    if (**p == '"')
    {
        (*p)++;
        char buffer[1024];
        int i = 0;
        while (**p && **p != '"')
        {
            if (i < (int)sizeof(buffer) - 1)
            {
                buffer[i++] = **p;
            }
            (*p)++;
        }
        if (**p != '"')
        {
            return SOMDB_ERR_PARSE;
        }
        (*p)++;
        buffer[i] = '\0';
        *out = value_make_text(buffer);
        if (!out->is_null && !out->as.text_val)
        {
            return SOMDB_ERR_MEMORY;
        }
        return SOMDB_OK;
    }

    char *endptr;
    long value = strtol(*p, &endptr, 10);
    if (endptr == *p)
    {
        return SOMDB_ERR_PARSE;
    }
    *out = value_make_int((int)value);
    *p = endptr;
    return SOMDB_OK;
}

static SomDBStatus parse_create(const char *input, Statement *stmt)
{
    const char *p = input + strlen("CREATE TABLE");
    skip_spaces(&p);

    char table_name[SOMDB_MAX_NAME_LEN];
    if (!read_identifier(&p, table_name))
    {
        return SOMDB_ERR_PARSE;
    }

    skip_spaces(&p);
    if (*p != '(')
    {
        return SOMDB_ERR_PARSE;
    }
    p++;

    stmt->type = STMT_CREATE_TABLE;
    memset(&stmt->as.create_table, 0, sizeof(stmt->as.create_table));
    strncpy(stmt->as.create_table.table_name, table_name, SOMDB_MAX_NAME_LEN - 1);

    while (*p)
    {
        skip_spaces(&p);
        if (*p == ')')
        {
            p++;
            break;
        }

        char col_name[SOMDB_MAX_NAME_LEN];
        char type_name[SOMDB_MAX_NAME_LEN];
        ColumnType type;

        if (!read_identifier(&p, col_name))
        {
            return SOMDB_ERR_PARSE;
        }
        skip_spaces(&p);
        if (!read_identifier(&p, type_name))
        {
            return SOMDB_ERR_PARSE;
        }
        if (column_type_from_string(type_name, &type) != SOMDB_OK)
        {
            return SOMDB_ERR_TYPE;
        }
        if (stmt->as.create_table.column_count >= SOMDB_MAX_COLUMNS)
        {
            return SOMDB_ERR_SCHEMA;
        }

        Column *col = &stmt->as.create_table.columns[stmt->as.create_table.column_count++];
        strncpy(col->name, col_name, SOMDB_MAX_NAME_LEN - 1);
        col->name[SOMDB_MAX_NAME_LEN - 1] = '\0';
        col->type = type;

        skip_spaces(&p);
        if (*p == ',')
        {
            p++;
            continue;
        }
        if (*p == ')')
        {
            p++;
            break;
        }
        return SOMDB_ERR_PARSE;
    }

    return SOMDB_OK;
}

static SomDBStatus parse_insert(const char *input, Statement *stmt)
{
    const char *p = input + strlen("INSERT INTO");
    skip_spaces(&p);

    stmt->type = STMT_INSERT;
    memset(&stmt->as.insert, 0, sizeof(stmt->as.insert));

    if (!read_identifier(&p, stmt->as.insert.table_name))
    {
        return SOMDB_ERR_PARSE;
    }

    skip_spaces(&p);
    if (!starts_with(p, "VALUES"))
    {
        return SOMDB_ERR_PARSE;
    }
    p += strlen("VALUES");
    skip_spaces(&p);
    if (*p != '(')
    {
        return SOMDB_ERR_PARSE;
    }
    p++;

    stmt->as.insert.values = calloc(SOMDB_MAX_COLUMNS, sizeof(Value));
    if (!stmt->as.insert.values)
    {
        return SOMDB_ERR_MEMORY;
    }

    while (*p)
    {
        skip_spaces(&p);
        if (*p == ')')
        {
            p++;
            break;
        }
        if (stmt->as.insert.value_count >= SOMDB_MAX_COLUMNS)
        {
            return SOMDB_ERR_SCHEMA;
        }

        SomDBStatus lit_status = parse_literal(&p, &stmt->as.insert.values[stmt->as.insert.value_count]);
        if (lit_status != SOMDB_OK)
        {
            return lit_status;
        }
        stmt->as.insert.value_count++;

        skip_spaces(&p);
        if (*p == ',')
        {
            p++;
            continue;
        }
        if (*p == ')')
        {
            p++;
            break;
        }
        return SOMDB_ERR_PARSE;
    }

    return SOMDB_OK;
}

static SomDBStatus parse_select(const char *input, Statement *stmt)
{
    const char *p = input + strlen("SELECT");
    skip_spaces(&p);

    stmt->type = STMT_SELECT;
    memset(&stmt->as.select, 0, sizeof(stmt->as.select));
    stmt->as.select.where_value = value_make_null(TYPE_INT);

    if (*p == '*')
    {
        stmt->as.select.select_all = 1;
        p++;
    }
    else
    {
        while (*p)
        {
            skip_spaces(&p);
            char name[SOMDB_MAX_NAME_LEN];
            if (!read_identifier(&p, name))
            {
                return SOMDB_ERR_PARSE;
            }
            if (stmt->as.select.selected_column_count >= SOMDB_MAX_COLUMNS)
            {
                return SOMDB_ERR_SCHEMA;
            }
            strncpy(stmt->as.select.selected_columns[stmt->as.select.selected_column_count++], name, SOMDB_MAX_NAME_LEN - 1);
            skip_spaces(&p);
            if (*p == ',')
            {
                p++;
                continue;
            }
            break;
        }
    }

    skip_spaces(&p);
    if (!starts_with(p, "FROM"))
    {
        return SOMDB_ERR_PARSE;
    }
    p += strlen("FROM");
    skip_spaces(&p);

    if (!read_identifier(&p, stmt->as.select.table_name))
    {
        return SOMDB_ERR_PARSE;
    }

    skip_spaces(&p);
    if (starts_with(p, "WHERE"))
    {
        p += strlen("WHERE");
        skip_spaces(&p);

        stmt->as.select.has_where = 1;
        if (!read_identifier(&p, stmt->as.select.where_column))
        {
            return SOMDB_ERR_PARSE;
        }
        skip_spaces(&p);
        if (*p != '=')
        {
            return SOMDB_ERR_PARSE;
        }
        p++;
        SomDBStatus lit_status = parse_literal(&p, &stmt->as.select.where_value);
        if (lit_status != SOMDB_OK)
        {
            return lit_status;
        }
    }

    return SOMDB_OK;
}

SomDBStatus parse_statement(const char *input, Statement *stmt)
{
    memset(stmt, 0, sizeof(*stmt));

    char buffer[SOMDB_MAX_INPUT];
    strncpy(buffer, input, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    trim(buffer);

    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == ';')
    {
        buffer[len - 1] = '\0';
        trim(buffer);
    }

    if (strcmp(buffer, ".exit") == 0)
    {
        stmt->type = STMT_META_EXIT;
        return SOMDB_OK;
    }
    if (strcmp(buffer, ".tables") == 0)
    {
        stmt->type = STMT_META_TABLES;
        return SOMDB_OK;
    }
    if (starts_with(buffer, ".schema"))
    {
        const char *p = buffer + strlen(".schema");
        skip_spaces(&p);
        if (!read_identifier(&p, stmt->as.meta_table_name))
        {
            return SOMDB_ERR_PARSE;
        }
        stmt->type = STMT_META_SCHEMA;
        return SOMDB_OK;
    }
    if (starts_with(buffer, "CREATE TABLE"))
    {
        return parse_create(buffer, stmt);
    }
    if (starts_with(buffer, "INSERT INTO"))
    {
        return parse_insert(buffer, stmt);
    }
    if (starts_with(buffer, "SELECT"))
    {
        return parse_select(buffer, stmt);
    }

    return SOMDB_ERR_PARSE;
}

void statement_free(Statement *stmt)
{
    if (!stmt)
    {
        return;
    }
    if (stmt->type == STMT_INSERT)
    {
        for (int i = 0; i < stmt->as.insert.value_count; i++)
        {
            value_free(&stmt->as.insert.values[i]);
        }
        free(stmt->as.insert.values);
        stmt->as.insert.values = NULL;
        stmt->as.insert.value_count = 0;
    }
    else if (stmt->type == STMT_SELECT && stmt->as.select.has_where)
    {
        value_free(&stmt->as.select.where_value);
    }
}

SomDBStatus execute_statement(Database *db, Statement *stmt)
{
    if (stmt->type == STMT_META_TABLES)
    {
        db_list_tables(db);
        return SOMDB_OK;
    }

    if (stmt->type == STMT_META_SCHEMA)
    {
        const Table *table = db_find_table_const(db, stmt->as.meta_table_name);
        if (!table)
        {
            return SOMDB_ERR_NOT_FOUND;
        }
        schema_print(&table->schema);
        return SOMDB_OK;
    }

    if (stmt->type == STMT_CREATE_TABLE)
    {
        Schema schema;
        schema_init(&schema, stmt->as.create_table.table_name);
        for (int i = 0; i < stmt->as.create_table.column_count; i++)
        {
            SomDBStatus status = schema_add_column(
                &schema,
                stmt->as.create_table.columns[i].name,
                stmt->as.create_table.columns[i].type);
            if (status != SOMDB_OK)
            {
                return status;
            }
        }
        SomDBStatus status = db_create_table(db, &schema);
        if (status != SOMDB_OK)
        {
            return status;
        }
        return storage_save_table(db_find_table(db, schema.table_name));
    }

    if (stmt->type == STMT_INSERT)
    {
        Table *table = db_find_table(db, stmt->as.insert.table_name);
        if (!table)
        {
            return SOMDB_ERR_NOT_FOUND;
        }

        Row row;
        SomDBStatus status = row_init(&row, stmt->as.insert.value_count);
        if (status != SOMDB_OK)
        {
            return status;
        }
        for (int i = 0; i < stmt->as.insert.value_count; i++)
        {
            row.values[i] = value_clone(&stmt->as.insert.values[i]);
            if (!row.values[i].is_null && row.values[i].type == TYPE_TEXT && !row.values[i].as.text_val)
            {
                row_free(&row);
                return SOMDB_ERR_MEMORY;
            }
        }

        status = table_insert(table, &row);
        row_free(&row);
        if (status != SOMDB_OK)
        {
            return status;
        }
        return storage_save_table(table);
    }

    if (stmt->type == STMT_SELECT)
    {
        const Table *table = db_find_table_const(db, stmt->as.select.table_name);
        if (!table)
        {
            return SOMDB_ERR_NOT_FOUND;
        }

        int indices[SOMDB_MAX_COLUMNS];
        int count = 0;

        if (stmt->as.select.select_all)
        {
            for (int i = 0; i < table->schema.column_count; i++)
            {
                indices[count++] = i;
            }
        }
        else
        {
            for (int i = 0; i < stmt->as.select.selected_column_count; i++)
            {
                int idx = schema_find_column(&table->schema, stmt->as.select.selected_columns[i]);
                if (idx < 0)
                {
                    return SOMDB_ERR_NOT_FOUND;
                }
                indices[count++] = idx;
            }
        }

        int where_index = -1;
        if (stmt->as.select.has_where)
        {
            where_index = schema_find_column(&table->schema, stmt->as.select.where_column);
            if (where_index < 0)
            {
                return SOMDB_ERR_NOT_FOUND;
            }
            if (table->schema.columns[where_index].type != stmt->as.select.where_value.type)
            {
                return SOMDB_ERR_TYPE;
            }
        }

        table_print_projection(
            table,
            indices,
            count,
            stmt->as.select.has_where,
            where_index,
            stmt->as.select.has_where ? &stmt->as.select.where_value : NULL);
        return SOMDB_OK;
    }

    return SOMDB_ERR_PARSE;
}
