#ifndef SOMDB_VALUE_H
#define SOMDB_VALUE_H

#include "common.h"

typedef enum
{
    TYPE_INT = 0,
    TYPE_TEXT = 1
} ColumnType;

typedef struct
{
    int is_null;
    ColumnType type;
    union
    {
        int int_val;
        char *text_val;
    } as;
} Value;

Value value_make_int(int v);
Value value_make_text(const char *s);
Value value_make_null(ColumnType type);
void value_free(Value *value);
Value value_clone(const Value *value);
int value_equals(const Value *a, const Value *b);
void value_print(const Value *value);
const char *column_type_name(ColumnType type);
SomDBStatus column_type_from_string(const char *s, ColumnType *out);

#endif
