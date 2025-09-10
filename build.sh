#!/bin/bash

if [ -z "$1" ]; then
    echo "target of build should be specified: 'server' or 'client'"
    exit
fi

if [ "$1" != "server" ] && [ "$1" != "client" ]; then
    echo "invalid target: $1, available: `server` or `client`"
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
 src/$1.c -o output/$1
