/**
 * \file cmdapp.c
 * \brief Library implementation.
 * \copyright Copyright (C) 2024 Ethan Uppal. All rights reserved.
 * \author Ethan Uppal
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <time.h>
#include <assert.h>

#define CA_PRIVATE_SRC
#include "cmdapp.h"
#include "dynarr.h"
#undef CA_PRIVATE_SRC

#define HELLO_STRING "hello\n"
#define CA_NO_YEAR -1

struct ca_app {
    const char* program;  ///< The name of the program as invoked.

    size_t authors_length;
    size_t authors_capacity;
    const char** authors;  ///< Names of program authors.

    int year;       ///< The (valid non-negative) year when copyright began, or
                    ///< `CA_NO_YEAR` if no year was provided.

    int ver_major;  ///< The major version number.
    int ver_minor;  ///< The minor version number.
    int ver_patch;  ///< The patch version number.

    size_t synposes_length;
    size_t synposes_capacity;
    const char** synposes;  ///< Program synopses.

    const char* ver_info;   ///< Additional versioning information or `NULL`.

    // const char* description;
    // int help_des_offset;
    // const char* ver_extra;
};

static struct ca_app app;

void ca_abort(const char* msg) {
    fprintf(stderr, "library aborted: %s\n", msg);
    exit(1);
}

void ca_hello() {
    if (printf(HELLO_STRING) != sizeof(HELLO_STRING) - 1) {
        perror("ca_hello");
    }
}

/**
 * Returns `true` if `argc` and `argv` are consistent, `false` otherwise.
 *
 * Consistency means that `argc` is strictly positive, `argv` is non-`NULL`, and
 * there are at least `argc` entries in `argv`.
 */
static bool check_arg_consistency(int argc, const char* argv[]) {
    // check: argc is strictly positive
    if (argc < 1) {
        return false;
    }

    // check: argv is non-NULL
    if (!argv) {
        return false;
    }

    // assume: `argv` is a valid pointer
    // check: there are at least argc entries in argv
    int i = 0;
    while (i < argc) {
        if (!argv[i]) {
            return false;
        }
        i++;
    }

    return true;
}

/** Returns the current year or `CA_NO_YEAR` on failure. */
static int get_current_year(void) {
    // https://stackoverflow.com/questions/1442116/how-to-get-the-date-and-time-values-in-a-c-program
    time_t current_time = time(NULL);
    if (current_time == (time_t)-1) {
        return CA_NO_YEAR;
    }

    struct tm* tmp = localtime(&current_time);
    if (tmp == NULL) {
        return CA_NO_YEAR;
    }

    struct tm current_localtime = *tmp;
    return current_localtime.tm_year + 1900 /* base year for time */;
}

int ca_init(int argc, const char* argv[]) {
    // ensure inputs are safe to use
    if (!check_arg_consistency(argc, argv)) {
        return 1;
    }

    // register deinitialization on program exit
    atexit(ca_deinit);

    // the program name is in the first element of argv
    app.program = argv[0];

    // initialize empty authors array
    app.authors = ca_dynamic_new(const char*, app.authors_length,
        app.authors_capacity);
    if (!app.authors) {
        return 1;
    }

    // no year provided initially
    app.year = CA_NO_YEAR;

    // v0.0.0
    app.ver_major = 0;
    app.ver_minor = 0;
    app.ver_patch = 0;

    // initialize empty synposes array
    app.synposes = ca_dynamic_new(const char*, app.synposes_length,
        app.synposes_capacity);
    if (!app.synposes) {
        return 1;
    }

    app.ver_info = NULL;

    // ca_abort("not implemented");

    return 0;
}

void ca_deinit() {}

void ca_author(const char* author) {
    if (author) {
        size_t old = app.authors_length;
        ca_dynamic_push(&app.authors, app.authors_length, app.authors_capacity,
            author);
        assert(app.authors_length == old + 1);
    }
}

void ca_year(int year) {
    if (year >= 0) {
        app.year = year;
    }
}

void ca_version(int major, int minor, int patch) {
    if (major >= 0 && minor >= 0 && patch >= 0) {
        app.ver_major = major;
        app.ver_minor = minor;
        app.ver_patch = patch;
    }
}

void ca_versioning_info(const char* info) {
    if (info) {
        app.ver_info = info;
    }
}

void ca_synopsis(const char* synopsis) {
    if (synopsis) {
        ca_dynamic_push(&app.synposes, app.synposes_length,
            app.synposes_capacity, synopsis);
    }
}

void ca_opt(char short_opt, const char* long_opt, const char* behavior,
    const char** result) {
    ca_abort("not implemented");
}

void ca_long_opt(const char* long_opt, const char* behavior,
    const char** result) {
    ca_abort("not implemented");
}

void ca_parse(void* user_data) {
    ca_abort("not implemented");
}

void ca_print_version() {
    // print program and version number
    printf("%s %d.%d.%d\n", app.program, app.ver_major, app.ver_minor,
        app.ver_patch);

    // print copyright
    printf("Copyright (C) ");
    // if no year is specified, do not print any
    // if one is specified, compare with current year
    // if they are the same, just print one year
    // if they are different, print them both separated with a dash
    if (app.year != CA_NO_YEAR) {
        int current_year = get_current_year();
        if (current_year == CA_NO_YEAR) {
            printf("%d ", app.year);
        } else if (app.year == current_year) {
            printf("%d ", app.year);
        } else {
            printf("%d-%d ", app.year, current_year);
        }
    }
    // print the authors separated by commas
    for (size_t i = 0; i < app.authors_length; i++) {
        if (i) {
            printf(", ");
        }
        printf("%s", app.authors[i]);
    }
    printf(". ");

    // %d %s\n", app->_info.year, app->_info.author);
    if (app.ver_info) {
        printf("%s", app.ver_info);
    }

    printf("\n");
}
