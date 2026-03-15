#ifndef SOMDB_COMMON_H
#define SOMDB_COMMON_H

#include <stddef.h>
#include <stdint.h>

#define SOMDB_MAX_NAME_LEN 64
#define SOMDB_MAX_COLUMNS 32
#define SOMDB_INITIAL_ROW_CAPACITY 16
#define SOMDB_INITIAL_TABLE_CAPACITY 8
#define SOMDB_MAX_INPUT 2048
#define SOMDB_DATA_DIR "data"
#define SOMDB_MAGIC "SOMDB1"
#define SOMDB_VERSION 1u

typedef enum
{
    SOMDB_OK = 0,
    SOMDB_ERR_IO,
    SOMDB_ERR_PARSE,
    SOMDB_ERR_SCHEMA,
    SOMDB_ERR_NOT_FOUND,
    SOMDB_ERR_TYPE,
    SOMDB_ERR_MEMORY,
    SOMDB_ERR_DUPLICATE,
    SOMDB_ERR_INTERNAL
} SomDBStatus;

const char *somdb_status_string(SomDBStatus status);

#endif
