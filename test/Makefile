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
	@bash -c '\
	expect() { \
		printf "\033[33;1m ~ testing:\033[m $$2\n"; \
		$$2 1> /dev/null 2> /dev/null; \
		if [ $$? -ne $$1 ]; then \
			printf "\033[31;1m - test failed\033[m\n\n"; \
			return 1; \
		else \
			printf "\033[32;1m + test passed\033[m\n\n"; \
			return 0; \
		fi \
	}; \
	expect 0 "./main -bc"; \
	expect 0 "./main -abc"; \
	expect 0 "./main -acb"; \
	expect 1 "./main -bac"; \
	expect 1 "./main -bca"; \
	expect 1 "./main -cab"; \
	expect 1 "./main -cba"; \
	expect 1 "./main -a"; \
	expect 1 "./main -a -b"; \
	expect 0 "./main -a -"; \
	expect 0 "./main --aa -"; \
	expect 1 "./main -d -c"; \
	expect 1 "./main -b -d"; \
	expect 0 "./main -d"; \
	expect 0 "./main -ax -O -d"; \
	expect 1 "./main -O -d"; \
	expect 0 "./main --help"; \
	expect 1 "./main -h -ax"; \
	expect 0 "./main -Ax"; \
	expect 0 "./main -A"; \
	expect 0 "./main -A -b"; \
	'

build_test: $(SRC)
	@$(CC) $(CFLAGS) $(LIBCONF) $^ -o $(EXEC)

clean:
	rm -rf $(EXEC)
