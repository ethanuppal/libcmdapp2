# Copyright (C) 2024 Ethan Uppal. All rights reserved.
#
# Purpose:	builds the test program for the libcmdapp library.
# Requires:	the libcmdapp library to be installed.

CC		:= $(shell which gcc-13 || which gcc || which clang)
CFLAGS	:= -std=c99 -pedantic -Wall -Wextra \
		   -Wno-unused-parameter
SRC		:= main.c
EXEC	:= main
LIB		:= cmdapp

CFLAGS 	+= -I/usr/local/include/$(LIB)
LIBCONF := -L/usr/local/lib -l$(LIB)

test: build_test
	@./main

build_test: $(SRC)
	@$(CC) $(CFLAGS) $(LIBCONF) $^ -o $(EXEC)

clean:
	rm -rf $(EXEC)
