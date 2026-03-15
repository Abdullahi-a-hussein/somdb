#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "value.h"

static char *somdb_strdup(const char *s)
{
    size_t len = strlen(s);
    char *copy = malloc(len + 1);
    if (!copy)
    {
        return NULL;
    }
    memcpy(copy, s, len + 1);
    return copy;
}

const char *somdb_status_string(SomDBStatus status)
{
    switch (status)
    {
    case SOMDB_OK:
        return "ok";
    case SOMDB_ERR_IO:
        return "io error";
    case SOMDB_ERR_PARSE:
        return "parse error";
    case SOMDB_ERR_SCHEMA:
        return "schema error";
    case SOMDB_ERR_NOT_FOUND:
        return "not found";
    case SOMDB_ERR_TYPE:
        return "type error";
    case SOMDB_ERR_MEMORY:
        return "memory error";
    case SOMDB_ERR_DUPLICATE:
        return "duplicate error";
    case SOMDB_ERR_INTERNAL:
        return "internal error";
    default:
        return "unknown error";
    }
}

Value value_make_int(int v)
{
    Value value;
    value.is_null = 0;
    value.type = TYPE_INT;
    value.as.int_val = v;
    return value;
}

Value value_make_text(const char *s)
{
    Value value;
    value.is_null = 0;
    value.type = TYPE_TEXT;
    value.as.text_val = somdb_strdup(s ? s : "");
    if (!value.as.text_val)
    {
        value.is_null = 1;
    }
    return value;
}

Value value_make_null(ColumnType type)
{
    Value value;
    value.is_null = 1;
    value.type = type;
    value.as.text_val = NULL;
    return value;
}

void value_free(Value *value)
{
    if (!value)
    {
        return;
    }
    if (!value->is_null && value->type == TYPE_TEXT)
    {
        free(value->as.text_val);
        value->as.text_val = NULL;
    }
    value->is_null = 1;
}

Value value_clone(const Value *value)
{
    if (value->is_null)
    {
        return value_make_null(value->type);
    }
    if (value->type == TYPE_INT)
    {
        return value_make_int(value->as.int_val);
    }
    return value_make_text(value->as.text_val ? value->as.text_val : "");
}

int value_equals(const Value *a, const Value *b)
{
    if (a->type != b->type)
    {
        return 0;
    }
    if (a->is_null || b->is_null)
    {
        return a->is_null && b->is_null;
    }
    if (a->type == TYPE_INT)
    {
        return a->as.int_val == b->as.int_val;
    }
    return strcmp(a->as.text_val, b->as.text_val) == 0;
}

void value_print(const Value *value)
{
    if (value->is_null)
    {
        printf("NULL");
        return;
    }
    if (value->type == TYPE_INT)
    {
        printf("%d", value->as.int_val);
    }
    else
    {
        printf("%s", value->as.text_val);
    }
}

const char *column_type_name(ColumnType type)
{
    switch (type)
    {
    case TYPE_INT:
        return "INT";
    case TYPE_TEXT:
        return "TEXT";
    default:
        return "UNKNOWN";
    }
}

SomDBStatus column_type_from_string(const char *s, ColumnType *out)
{
    if (!s || !out)
    {
        return SOMDB_ERR_PARSE;
    }
    if (strcmp(s, "INT") == 0)
    {
        *out = TYPE_INT;
        return SOMDB_OK;
    }
    if (strcmp(s, "TEXT") == 0)
    {
        *out = TYPE_TEXT;
        return SOMDB_OK;
    }
    return SOMDB_ERR_TYPE;
}
