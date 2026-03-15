#ifndef SOMDB_STORAGE_H
#define SOMDB_STORAGE_H

#include "db.h"

SomDBStatus storage_init_dir(void);
SomDBStatus storage_save_table(const Table *table);
SomDBStatus storage_load_all(Database *db);

#endif
