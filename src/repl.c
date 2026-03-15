#include <stdio.h>
#include "repl.h"

void repl_run(Database *db)
{
    char input[SOMDB_MAX_INPUT];

    while (1)
    {
        printf("somDB > ");
        if (!fgets(input, sizeof(input), stdin))
        {
            printf("\n");
            break;
        }

        Statement stmt;
        SomDBStatus parse_status = parse_statement(input, &stmt);
        if (parse_status != SOMDB_OK)
        {
            printf("error: %s\n", somdb_status_string(parse_status));
            continue;
        }

        if (stmt.type == STMT_META_EXIT)
        {
            statement_free(&stmt);
            break;
        }

        SomDBStatus exec_status = execute_statement(db, &stmt);
        if (exec_status == SOMDB_OK && stmt.type != STMT_SELECT && stmt.type != STMT_META_TABLES && stmt.type != STMT_META_SCHEMA)
        {
            printf("ok\n");
        }
        else if (exec_status != SOMDB_OK)
        {
            printf("error: %s\n", somdb_status_string(exec_status));
        }

        statement_free(&stmt);
    }
}
