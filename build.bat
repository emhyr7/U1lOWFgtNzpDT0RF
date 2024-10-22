@echo off
setlocal

if not exist build mkdir build

set CFLAGS=-std=c23 -g -Wno-static-in-inline
set LFLAGS=-luser32.lib

clang %CFLAGS% -o build\code.exe code\main.c %LFLAGS%
