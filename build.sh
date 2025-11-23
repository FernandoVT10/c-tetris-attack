#!/bin/bash

FILES="src/main.c"
RAYLIB="-I./raylib-5.5/include -L./raylib-5.5/lib/ -l:libraylib.a"

gcc -Wall -Werror $FILES -o main $RAYLIB -lm
