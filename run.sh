#!/bin/sh
gcc -pthread -o router router.c
./router 1 8088 &
gnome-terminal -- ./router 2 8089 &
gnome-terminal -- ./router 3 8090 &
gnome-terminal -- ./router 4 8091 &
