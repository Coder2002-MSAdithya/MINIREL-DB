#!/bin/bash

echo "📁 Entering physical directory..."
cd physical # ensures script works no matter where you call it from

rm *.bpidx
echo "🚧 Compiling bptree_test.c..."

gcc -Wall -Wextra -g -I../include \
    bptree_test.c \
    ../build/physical/bptree.o \
    ../build/globals.o \
    ../build/helpers.o \
    ../build/physical/error.o \
    ../build/physical/freemap.o \
    -o ../bptree_test

echo "✅ Build completed successfully."

chmod +x bptree_test


echo "🏁 Done."