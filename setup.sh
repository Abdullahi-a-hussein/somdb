#!/bin/bash

src_files=(
"main.c"
"repl.c"
"parser.c"
"lexer.c"
"engine.c"
"table.c"
"row.c"
"pager.c"
"storage.c"
"schema.c"
"util.c"
)

include_files=(
"somdb.h"
"parser.h"
"lexer.h"
"engine.h"
"table.h"
"row.h"
"pager.h"
"storage.h"
"schema.h"
"util.h"
)

extra_dirs=(
"data"
"tests"
)

create_files() {
    local folder="$1"
    shift

    # create folder only if it does not exist
    if [[ ! -d "$folder" ]]; then
        mkdir -p "$folder"
        echo "Created folder: $folder"
    fi

    for file in "$@"
    do
        local path="$folder/$file"

        # create file only if it does not exist
        if [[ ! -f "$path" ]]; then
            touch "$path"
            echo "Created file: $path"
        else
            echo "Skipped (exists): $path"
        fi
    done
}

create_files "src" "${src_files[@]}"
create_files "include" "${include_files[@]}"

for dir in "${extra_dirs[@]}"
do
    mkdir -p "$dir"
done

touch Makefile

echo "somDB structure created."