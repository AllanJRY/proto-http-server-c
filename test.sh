#!/bin/bash

if [ -z "$1" ]; then
    echo "target of test build should be specified, it could any c files in the tests/ folder"
    exit
fi

if [ ! -d "output/" ] ; then 
    mkdir "output" ; 
fi

clang -g3 -march=native -fsanitize=address -std=c99 \
 -Wall -Wextra -Wshadow -Wundef \
 -pedantic \
 -Iinclude -Isrc \
 -DDEBUG \
 tests/$1.c -o output/test_$1
