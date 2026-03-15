#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include "storage.h"

typedef struct
{
    char magic[8];
    uint32_t version;
    uint32_t column_count;
    uint32_t row_count;
} TableFileHeader;

static void make_table_path(const char *table_name, char *out, size_t out_size)
{
    snprintf(out, out_size, "%s/%s.som", SOMDB_DATA_DIR, table_name);
}

static SomDBStatus write_u32(FILE *fp, uint32_t v)
{
    return fwrite(&v, sizeof(v), 1, fp) == 1 ? SOMDB_OK : SOMDB_ERR_IO;
}

static SomDBStatus read_u32(FILE *fp, uint32_t *out)
{
    return fread(out, sizeof(*out), 1, fp) == 1 ? SOMDB_OK : SOMDB_ERR_IO;
}

static SomDBStatus write_string(FILE *fp, const char *s)
{
    uint32_t len = (uint32_t)strlen(s);
    if (write_u32(fp, len) != SOMDB_OK)
    {
        return SOMDB_ERR_IO;
    }
    return fwrite(s, 1, len, fp) == len ? SOMDB_OK : SOMDB_ERR_IO;
}

static SomDBStatus read_string(FILE *fp, char **out)
{
    uint32_t len;
    if (read_u32(fp, &len) != SOMDB_OK)
    {
        return SOMDB_ERR_IO;
    }
    char *buf = malloc((size_t)len + 1);
    if (!buf)
    {
        return SOMDB_ERR_MEMORY;
    }
    if (fread(buf, 1, len, fp) != len)
    {
        free(buf);
        return SOMDB_ERR_IO;
    }
    buf[len] = '\0';
    *out = buf;
    return SOMDB_OK;
}

SomDBStatus storage_init_dir(void)
{
    struct stat st;
    if (stat(SOMDB_DATA_DIR, &st) == 0)
    {
        if (S_ISDIR(st.st_mode))
        {
            return SOMDB_OK;
        }
        return SOMDB_ERR_IO;
    }

    if (mkdir(SOMDB_DATA_DIR, 0755) != 0 && errno != EEXIST)
    {
        return SOMDB_ERR_IO;
    }
    return SOMDB_OK;
}

SomDBStatus storage_save_table(const Table *table)
{
    char path[256];
    make_table_path(table->schema.table_name, path, sizeof(path));

    FILE *fp = fopen(path, "wb");
    if (!fp)
    {
        return SOMDB_ERR_IO;
    }

    TableFileHeader header = {0};
    strncpy(header.magic, SOMDB_MAGIC, sizeof(header.magic) - 1);
    header.version = SOMDB_VERSION;
    header.column_count = (uint32_t)table->schema.column_count;
    header.row_count = (uint32_t)table->row_count;

    if (fwrite(&header, sizeof(header), 1, fp) != 1)
    {
        fclose(fp);
        return SOMDB_ERR_IO;
    }

    if (write_string(fp, table->schema.table_name) != SOMDB_OK)
    {
        fclose(fp);
        return SOMDB_ERR_IO;
    }

    for (int i = 0; i < table->schema.column_count; i++)
    {
        if (write_string(fp, table->schema.columns[i].name) != SOMDB_OK)
        {
            fclose(fp);
            return SOMDB_ERR_IO;
        }
        if (write_u32(fp, (uint32_t)table->schema.columns[i].type) != SOMDB_OK)
        {
            fclose(fp);
            return SOMDB_ERR_IO;
        }
    }

    for (size_t r = 0; r < table->row_count; r++)
    {
        for (int c = 0; c < table->schema.column_count; c++)
        {
            const Value *v = &table->rows[r].values[c];
            if (write_u32(fp, (uint32_t)v->is_null) != SOMDB_OK)
            {
                fclose(fp);
                return SOMDB_ERR_IO;
            }
            if (write_u32(fp, (uint32_t)v->type) != SOMDB_OK)
            {
                fclose(fp);
                return SOMDB_ERR_IO;
            }
            if (!v->is_null)
            {
                if (v->type == TYPE_INT)
                {
                    if (write_u32(fp, (uint32_t)v->as.int_val) != SOMDB_OK)
                    {
                        fclose(fp);
                        return SOMDB_ERR_IO;
                    }
                }
                else
                {
                    if (write_string(fp, v->as.text_val) != SOMDB_OK)
                    {
                        fclose(fp);
                        return SOMDB_ERR_IO;
                    }
                }
            }
        }
    }

    fclose(fp);
    return SOMDB_OK;
}

static SomDBStatus storage_load_file(Database *db, const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (!fp)
    {
        return SOMDB_ERR_IO;
    }

    TableFileHeader header;
    if (fread(&header, sizeof(header), 1, fp) != 1)
    {
        fclose(fp);
        return SOMDB_ERR_IO;
    }

    if (strncmp(header.magic, SOMDB_MAGIC, strlen(SOMDB_MAGIC)) != 0)
    {
        fclose(fp);
        return SOMDB_ERR_IO;
    }

    char *table_name = NULL;
    SomDBStatus status = read_string(fp, &table_name);
    if (status != SOMDB_OK)
    {
        fclose(fp);
        return status;
    }

    Schema schema;
    schema_init(&schema, table_name);
    free(table_name);

    for (uint32_t i = 0; i < header.column_count; i++)
    {
        char *col_name = NULL;
        uint32_t type_u32;

        status = read_string(fp, &col_name);
        if (status != SOMDB_OK)
        {
            fclose(fp);
            return status;
        }
        status = read_u32(fp, &type_u32);
        if (status != SOMDB_OK)
        {
            free(col_name);
            fclose(fp);
            return status;
        }

        status = schema_add_column(&schema, col_name, (ColumnType)type_u32);
        free(col_name);
        if (status != SOMDB_OK)
        {
            fclose(fp);
            return status;
        }
    }

    status = db_create_table(db, &schema);
    if (status != SOMDB_OK)
    {
        fclose(fp);
        return status;
    }

    Table *table = db_find_table(db, schema.table_name);
    if (!table)
    {
        fclose(fp);
        return SOMDB_ERR_INTERNAL;
    }

    for (uint32_t r = 0; r < header.row_count; r++)
    {
        Row row;
        status = row_init(&row, schema.column_count);
        if (status != SOMDB_OK)
        {
            fclose(fp);
            return status;
        }

        for (int c = 0; c < schema.column_count; c++)
        {
            uint32_t is_null_u32;
            uint32_t type_u32;
            if (read_u32(fp, &is_null_u32) != SOMDB_OK || read_u32(fp, &type_u32) != SOMDB_OK)
            {
                row_free(&row);
                fclose(fp);
                return SOMDB_ERR_IO;
            }

            if (is_null_u32)
            {
                row.values[c] = value_make_null((ColumnType)type_u32);
            }
            else if ((ColumnType)type_u32 == TYPE_INT)
            {
                uint32_t iv;
                if (read_u32(fp, &iv) != SOMDB_OK)
                {
                    row_free(&row);
                    fclose(fp);
                    return SOMDB_ERR_IO;
                }
                row.values[c] = value_make_int((int)iv);
            }
            else
            {
                char *text = NULL;
                if (read_string(fp, &text) != SOMDB_OK)
                {
                    row_free(&row);
                    fclose(fp);
                    return SOMDB_ERR_IO;
                }
                row.values[c] = value_make_text(text);
                free(text);
                if (!row.values[c].is_null && !row.values[c].as.text_val)
                {
                    row_free(&row);
                    fclose(fp);
                    return SOMDB_ERR_MEMORY;
                }
            }
        }

        status = table_insert(table, &row);
        row_free(&row);
        if (status != SOMDB_OK)
        {
            fclose(fp);
            return status;
        }
    }

    fclose(fp);
    return SOMDB_OK;
}

SomDBStatus storage_load_all(Database *db)
{
    DIR *dir = opendir(SOMDB_DATA_DIR);
    if (!dir)
    {
        return SOMDB_ERR_IO;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_name[0] == '.')
        {
            continue;
        }
        const char *ext = strrchr(entry->d_name, '.');
        if (!ext || strcmp(ext, ".som") != 0)
        {
            continue;
        }

        char path[512];
        snprintf(path, sizeof(path), "%s/%s", SOMDB_DATA_DIR, entry->d_name);
        SomDBStatus status = storage_load_file(db, path);
        if (status != SOMDB_OK)
        {
            closedir(dir);
            return status;
        }
    }

    closedir(dir);
    return SOMDB_OK;
}
