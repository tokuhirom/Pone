#!/bin/sh
gcc -I 3rmaked/rockre/ -I src/ pone_generated.c -L 3rd/rockre/ -lm libpone.a -pthread 3rd/rockre/librockre.a -lstdc++
