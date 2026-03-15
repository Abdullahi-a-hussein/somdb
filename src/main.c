#include <stdio.h>
#include "repl.h"

int main(void)
{
    Database db;
    SomDBStatus status = db_init(&db);
    if (status != SOMDB_OK)
    {
        fprintf(stderr, "failed to init db: %s\n", somdb_status_string(status));
        return 1;
    }

    status = storage_init_dir();
    if (status != SOMDB_OK)
    {
        fprintf(stderr, "failed to init storage dir: %s\n", somdb_status_string(status));
        db_free(&db);
        return 1;
    }

    status = storage_load_all(&db);
    if (status != SOMDB_OK)
    {
        fprintf(stderr, "failed to load tables: %s\n", somdb_status_string(status));
        db_free(&db);
        return 1;
    }

    repl_run(&db);
    db_free(&db);
    return 0;
}
