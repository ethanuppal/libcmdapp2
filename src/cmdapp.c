/**
 * \file cmdapp.c
 * \brief Library implementation.
 * \copyright Copyright (C) 2024 Ethan Uppal. All rights reserved.
 * \author Ethan Uppal
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#define CA_PRIVATE_SRC
#include "cmdapp.h"
#include "dynarr.h"
#undef CA_PRIVATE_SRC

#define HELLO_STRING "hello\n"
#define CA_NO_YEAR -1
#define CA_DESCRIPTION_OFFSET 20

/* Option information. */
enum ca_opt_flags {
    CA_OPT_ARG = 1 << 0,     ///< Takes an argument.
    CA_OPT_OPTARG = 1 << 1,  ///< Argument is optional.
};

/* Quantifiers for determining option compatibility. */
enum ca_opt_quantifier {
    CA_OPT_QUANTIFIER_NONE,  ///< Absence of a quantifier.
    CA_OPT_QUANTIFIER_ALL,   ///< All of the provided options must be passed.
    CA_OPT_QUANTIFIER_ANY    ///< At least one of the provided options must be
                             ///< passed.
};

/** A command line option. */
struct ca_opt {
    char short_opt;        ///< Short version of the command, or `'\0'` if none.
    const char* long_opt;  ///< Long version of the command.
    enum ca_opt_flags flags;            ///< Option flags.
    bool quantifier_is_negated;         ///< Whether `quantifier` is negated.
    enum ca_opt_quantifier quantifier;  ///< Conflict quantifier.
    const char* conflicts;    ///< A null-terminated list of option conflicts.
    const char** result;      ///< A pointer to where the passed arg should go.
    const char* arg_name;     ///< Name of the argument.
    const char* description;  ///< Option description.
};

/* Library data for the command line app. */
struct ca_app {
    const char* program;      ///< The name of the program as invoked.

    const char* description;  ///< A description of the program.

    size_t authors_length;
    size_t authors_capacity;
    const char** authors;  ///< Names of program authors.

    int year;       ///< The (valid non-negative) year when copyright began, or
                    ///< `CA_NO_YEAR` if no year was provided.

    int ver_major;  ///< The major version number.
    int ver_minor;  ///< The minor version number.
    int ver_patch;  ///< The patch version number.

    size_t synopses_length;
    size_t synopses_capacity;
    const char** synopses;    ///< Program synopses.

    const char* ver_info;     ///< Additional versioning information or `NULL`.

    bool use_end_of_options;  ///< see ca_use_end_of_options().

    size_t options_length;
    size_t options_capacity;
    struct ca_opt* options;  ///< Program options.

    // const char* description;
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

static bool is_short_flag(char flag) {
    return isalnum(flag);
}

/* Parses the given `behavior` string and initializes `opt`. Returns zero on
 * success, nonzero otherwise. */
static int ca_opt_parse_behavior(struct ca_opt* opt, const char* behavior) {
    // no behavior, fall back to defaults
    if (behavior[0] == 0) {
        return 0;
    }

    // otherwise we must specify that it takes an argument
    if (behavior[0] != '.') {
        return 1;
    }
    opt->flags |= CA_OPT_ARG;

    // we check whether the argument is optional
    if (behavior[1] == '?') {
        opt->flags |= CA_OPT_OPTARG;
    }

    // check if no quantifier provided
    if (behavior[2] == '\0') {
        return 0;
    }

    // find the first position after ?+whitespace where we encounter a
    // quantifier
    size_t i = 1;
    while (strchr("? \t", behavior[i]) != NULL
           && strchr("!@&", behavior[i]) == NULL) {
        i++;
    }

    // check whether quantifier is negated
    if (behavior[i] == '!') {
        opt->quantifier_is_negated = true;
        i++;
    }

    // determine quantifier
    switch (behavior[i]) {
        case '@':
            opt->quantifier = CA_OPT_QUANTIFIER_ANY;
            break;
        case '&':
            opt->quantifier = CA_OPT_QUANTIFIER_ALL;
            break;
        default:
            return 1;
    }

    // remaining input [i + 1, length) is list of flags
    opt->conflicts = behavior + i + 1;

    // validate that it is only a list of flags
    for (size_t i = 0; opt->conflicts[i]; i++) {
        if (!is_short_flag(opt->conflicts[i])) {
            return 1;
        }
    }

    return 0;
}

static void print_authors(void) {
    switch (app.authors_length) {
        case 1:
            printf("%s", app.authors[0]);
            break;
        case 2:
            printf("%s and %s", app.authors[0], app.authors[1]);
            break;
        default: {
            for (size_t i = 0; i < app.authors_length; i++) {
                if (i) {
                    printf(", ");
                }
                if (i + 1 == app.authors_length) {
                    printf("and ");
                }
                printf("%s", app.authors[i]);
            }
            break;
        }
    }
}

int ca_init(int argc, const char* argv[]) {
    // ensure inputs are safe to use
    if (!check_arg_consistency(argc, argv)) {
        errno = EINVAL;
        return 1;
    }

    // register deinitialization on program exit
    atexit(ca_deinit);

    // the program name is in the first element of argv
    app.program = argv[0];

    // no default description
    app.description = NULL;

    // initialize empty authors array
    app.authors = ca_dynamic_new(const char*, app.authors_length,
        app.authors_capacity);
    if (!app.authors) {
        errno = ENOMEM;
        return 1;
    }

    // no year provided initially
    app.year = CA_NO_YEAR;

    // v0.0.0
    app.ver_major = 0;
    app.ver_minor = 0;
    app.ver_patch = 0;

    // initialize empty synopses array
    app.synopses = ca_dynamic_new(const char*, app.synopses_length,
        app.synopses_capacity);
    if (!app.synopses) {
        errno = ENOMEM;
        return 1;
    }

    // default additional versioning information
    app.ver_info = "All rights reserved.";

    // default: -- ends the option list
    app.use_end_of_options = true;

    // initialize empty synopses array
    app.options = ca_dynamic_new(struct ca_opt, app.options_length,
        app.options_capacity);
    if (!app.options) {
        errno = ENOMEM;
        return 1;
    }

    return 0;
}

void ca_deinit() {
    free(app.authors);
    free(app.synopses);
    free(app.options);
}

void ca_description(const char* description) {
    if (description) {
        app.description = description;
    }
}

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
        ca_dynamic_push(&app.synopses, app.synopses_length,
            app.synopses_capacity, synopsis);
    }
}

void ca_use_end_of_options(bool use) {
    app.use_end_of_options = use;
}

int ca_opt(char short_opt, const char* long_opt, const char* behavior,
    const char** result, const char* description) {
    // these parameters must be passed
    if (!long_opt || !behavior) {
        errno = EINVAL;
        return 1;
    }
    if (short_opt != '\0' && !is_short_flag(short_opt)) {
        errno = EINVAL;
        return 1;
    }

    // initialize option
    struct ca_opt opt;
    opt.short_opt = short_opt;
    opt.long_opt = long_opt;
    opt.result = result;
    opt.flags = 0;
    opt.quantifier_is_negated = false;
    opt.quantifier = CA_OPT_QUANTIFIER_NONE;
    opt.arg_name = NULL;
    opt.description = description;

    // parse behavior
    if (ca_opt_parse_behavior(&opt, behavior) != 0) {
        errno = EINVAL;
        return 1;
    }

    // if it takes an arg, it has to store it somewhere
    // the pointer will be to the name of the arg, or NULL if a default is to be
    // provided
    if (opt.flags & CA_OPT_ARG) {
        if (!result) {
            errno = EINVAL;
            return 1;
        } else {
            opt.arg_name = *result ? *result : "ARG";
            *result = NULL;  // reset back to NULL
        }
    }

    ca_dynamic_push(&app.options, app.options_length, app.options_capacity,
        opt);

    return 0;
}

int ca_long_opt(const char* long_opt, const char* behavior, const char** result,
    const char* description) {
    return ca_opt(0, long_opt, behavior, result, description);
}

void ca_parse(void* user_data __unused) {
    ca_abort("not implemented");
}

void ca_print_version() {
    // print program and version number
    printf("%s %d.%d.%d\n", app.program, app.ver_major, app.ver_minor,
        app.ver_patch);

    // rest of the prints use authors
    if (app.authors_length == 0) {
        return;
    }

    // print copyright
    printf("\nCopyright (C) ");
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
    print_authors();
    printf(".");

    // print additional versioning information
    if (app.ver_info) {
        printf(" %s", app.ver_info);
    }

    // print authorship
    printf("\n\nWritten by ");
    print_authors();
    printf(".\n");
}

void ca_print_help() {
    // keep track of whether a section has been printed so extra space can be
    // added for separation
    bool previous_print = false;

    // print description
    if (app.description) {
        printf("%s\n", app.description);
        previous_print = true;
    }

    // print synopses
    if (app.synopses_length > 0) {
        if (previous_print) printf("\n");
        printf("Usage: %s %s\n", app.program, app.synopses[0]);
        for (size_t i = 1; i < app.synopses_length; i++) {
            printf("   or: %s %s\n", app.program, app.synopses[i]);
        }
        previous_print = true;
    }

    // print options
    if (app.options_length > 0) {
        if (previous_print) printf("\n");
        printf("Options:\n");
        for (size_t i = 0; i < app.options_length; i++) {
            const struct ca_opt* opt = &app.options[i];

            // print the short option
            if (opt->short_opt != '\0') {
                printf(" -%c, ", opt->short_opt);
            } else {
                printf("      ");
            }

            // print the long option with arg if necessary
            int chars_printed = printf("--%s", opt->long_opt);
            if (opt->flags & CA_OPT_ARG) {
                chars_printed += printf("[=%s]", opt->arg_name);
            }

            // determine if past the offset for printing descriptions
            // if it is, put description on new line
            // otherwise print description afterward
            int padding = CA_DESCRIPTION_OFFSET
                          - (chars_printed + (sizeof("      ") - 1));
            if (padding < 1 /* min of 1 prevents right next to each other */) {
                printf("\n%*s%s\n", CA_DESCRIPTION_OFFSET - 1, "",
                    opt->description);
            } else {
                printf("%*s%s\n", padding, "", opt->description);
            }
        }
        previous_print = true;
    }
}
