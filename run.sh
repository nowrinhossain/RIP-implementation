#!/bin/sh


gcc -pthread -o router router.c

sudo ./router 1 8088 &
sudo ./router 2 8089 &
sudo ./router 3 8090 &
