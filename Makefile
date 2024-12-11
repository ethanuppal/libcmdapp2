# Copyright (C) 2024 Ethan Uppal. All rights reserved.
#
# Purpose: builds and installs the libcmdapp library.

SRCDIR		:= src
INCLUDEDIR	:= src

CC			:= $(shell which gcc || which clang)
CFLAGS		:= -std=c99 -pedantic -Wall -Wextra -I $(INCLUDEDIR) -D _XOPEN_SOURCE -fPIC
CDEBUG		:= -g
CRELEASE	:= -O2
TARGET		:= cmdapp

CFLAGS 		+= $(CRELEASE)

SRC			:= $(shell find $(SRCDIR) -type f -name "*.c")
OBJ			:= $(SRC:.c=.o)

PUBHEA		:= src/cmdapp.h src/proj.h

STATICLIB	:= lib$(TARGET).a
DYNLIB		:= lib$(TARGET).so
LCLLIBS		:= $(STATICLIB) $(DYNLIB)

INSTLIB		:= /usr/local/lib
INSTHEA		:= /usr/local/include/$(TARGET)

# NOT MY CODE
# Customizes ar for macOS
ifeq ($(shell uname), Darwin)
AR 		:= /usr/bin/libtool
AR_OPT 	:= -static -o
else
AR 		:= ar
AR_OPT 	:= rcs -o
endif

all: static dynamic
static: $(STATICLIB)
dynamic: $(DYNLIB)

%.o: %.c
	@echo 'Compiling $@'
	$(CC) $(CFLAGS) $^ -c -o $@

clean:
	rm -rf $(LCLLIBS) $(OBJ) test/main docs

$(STATICLIB): $(OBJ)
	@echo 'Creating static $@'
	$(AR) $(AR_OPT) $@ $^

$(DYNLIB): $(OBJ)
	@echo 'Creating dynamic $@'
	$(CC) $(CFLAGS) -shared $^ -o $@

install: $(LCLLIBS)
	mkdir -p $(INSTLIB)
	mkdir -p $(INSTHEA)
	mv $(STATICLIB) $(INSTLIB)
	mv $(DYNLIB) $(INSTLIB)
	cp $(PUBHEA) $(INSTHEA)

uninstall:
	rm -rf $(INSTLIB)/$(STATICLIB) \
		$(INSTLIB)/$(DYNLIB) \
		$(INSTHEA)

reinstall:
	sudo make uninstall
	sudo make install

# Requires doxygen to be installed.
# Link: https://www.doxygen.nl
.PHONY: docs
docs:
	if ! command -v doxygen &> /dev/null; then \
	    echo "'doxygen' could not be found"; \
		echo "You can install it from https://www.doxygen.nl"; \
	    exit; \
	fi; \
	GIT_REVISION=$(shell git rev-parse --short HEAD) doxygen

.PHONY: test
test:
	@cd test; make test
