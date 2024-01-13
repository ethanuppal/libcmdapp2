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
#include <stdarg.h>

#define CA_PRIVATE_SRC
#include "cmdapp.h"
#include "dynarr.h"
#undef CA_PRIVATE_SRC

/** Global library state. */
static struct ca_app app;

// static void _ca_abort(const char* msg) {
//     fprintf(stderr, "library aborted: %s\n", msg);
//     exit(1);
// }

void ca_hello() {
    if (printf("%s", HELLO_STRING) != sizeof(HELLO_STRING) - 1) {
        perror("ca_hello");
    }
}

/**
 * Returns `true` if `argc` and `argv` are consistent, `false` otherwise.
 *
 * Consistency means that `argc` is strictly positive, `argv` is non-`NULL`, and
 * there are at least `argc` entries in `argv`.
 */
static bool ca_check_arg_consistency(int argc, const char* argv[]) {
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
static int ca_get_current_year(void) {
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

static bool ca_is_short_flag(char flag) {
    return isalnum(flag);
}

/* Parses the given `behavior` string and initializes `opt`. Returns zero on
 * success, nonzero otherwise. */
static int ca_parse_opt_behavior(struct ca_opt* opt, const char* behavior) {
    // no behavior, fall back to defaults
    if (behavior[0] == 0) {
        return 0;
    }

    // otherwise we must specify that it takes an argument or is in multiflag
    if (behavior[0] == '.') {
        opt->flags |= CA_OPT_ARG;

        // we check whether the argument is optional
        if (behavior[1] == '?') {
            opt->flags |= CA_OPT_OPTARG;
        } else if (behavior[1] == '\0') {
            return 0;
        }

        // check if no quantifier provided
        if (behavior[2] == '\0') {
            return 0;
        }
    } else if (behavior[0] == '*') {
        opt->flags |= CA_OPT_MFLAG;

        // check if no quantifier provided
        if (behavior[1] == '\0') {
            return 0;
        }
    }

    // find the first position after . + ? + whitespace where we encounter a
    // quantifier
    size_t i = 0;
    while (strchr(".? \t", behavior[i]) != NULL
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
        case '<':
            opt->quantifier = CA_OPT_QUANTIFIER_ONLY;
            break;
        default:
            return 1;
    }

    // remaining input [i + 1, length) is list of flags
    opt->refs = behavior + i + 1;

    // validate that it is only a list of flags
    for (size_t i = 0; opt->refs[i]; i++) {
        if (!ca_is_short_flag(opt->refs[i])) {
            return 1;
        }
    }

    return 0;
}

/**
 * Finds the option associated with `short_opt` if it is non-`'\0'` or
 * `long_opt` if it is non-`NULL`.
 */
static struct ca_opt* ca_lookup_opt(char short_opt, const char* long_opt) {
    struct ca_opt* short_opt_cache[26 + 26 + 10] = {NULL};

    if (short_opt != '\0') {
        int map_index = islower(short_opt)   ? short_opt - 'a'
                        : isupper(short_opt) ? short_opt - 'A' + 26
                        : isdigit(short_opt) ? short_opt - '0' + 26 + 26
                                             : -1;
        if (short_opt_cache[map_index]) {
            return short_opt_cache[map_index];
        }
        for (size_t i = 0; i < app.options_length; i++) {
            if (app.options[i].short_opt == short_opt) {
                return short_opt_cache[map_index] = &app.options[i];
            }
        }
    } else if (long_opt != NULL) {
        for (size_t i = 0; i < app.options_length; i++) {
            if (strcmp(app.options[i].long_opt, long_opt) == 0) {
                return &app.options[i];
            }
        }
    }
    return NULL;
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
    if (!ca_check_arg_consistency(argc, argv)) {
        errno = EINVAL;
        return 1;
    }
    app.argc = argc;
    app.argv = argv;

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

    // initialize empty results array
    app.results = ca_dynamic_new(struct ca_parse_result, app.results_length,
        app.results_capacity);
    if (!app.results) {
        errno = ENOMEM;
        return 1;
    }

    // no callbacks by default
    app.opt_callback = NULL;
    app.arg_callback = NULL;

    // supply default --help and --version implementations
    app.override_help = false;
    app.override_version = false;

    // register deinitialization on program exit
    atexit(ca_deinit);

    return 0;
}

void ca_deinit() {
    free(app.authors);
    free(app.synopses);
    free(app.options);
    free(app.results);
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
    if (short_opt != '\0' && !ca_is_short_flag(short_opt)) {
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
    if (ca_parse_opt_behavior(&opt, behavior) != 0) {
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

void ca_set_callbacks(void (*opt_callback)(char, const char*, const char*,
                          void*),
    void (*arg_callback)(const char*, void*)) {
    app.opt_callback = opt_callback;
    app.arg_callback = arg_callback;
}

/** Adds an option to the results array. */
static void ca_parsed_opt(struct ca_opt* opt, const char* arg) {
    struct ca_parse_result result;
    result.opt = opt;
    result.arg = arg;
    opt->was_passed = true;
    ca_dynamic_push(&app.results, app.results_length, app.results_capacity,
        result);
    app.options_count++;
}

/** Adds an argument to the results array. */
static void ca_parsed_arg(struct ca_opt** last_opt, const char* arg) {
    struct ca_parse_result result;
    if (*last_opt) {
        result.opt = *last_opt;
        result.arg = arg;
        (*last_opt)->was_passed = true;
        *last_opt = NULL;
    } else {
        result.opt = NULL;
        result.arg = arg;
    }
    ca_dynamic_push(&app.results, app.results_length, app.results_capacity,
        result);
}

/** Iterate over the provided command line arguments and construct a resulting
 * array of options and arguments. */
static int ca_construct_results(void) {
    // when -- is passed and support for it is enabled, all subsequent arguments
    // are treated only as arguments
    bool only_args_now = false;

    // we start at 1 because argv[0] is already in app.program and it's useless
    // here
    int i = 1;

    // the previous option that takes an argument
    // this introduces code dup I need to figure out how to fix
    // specifically, anytime there is arg_callback
    struct ca_opt* last_opt = NULL;

    // go through the command line arguments
    while (i < app.argc) {
        const char* cur = app.argv[i];

        if (only_args_now) {
            // handle the case of after --
            ca_parsed_arg(&last_opt, cur);
            i++;
        } else if (cur[0] == '-') {
            // it could be a flag

            // handle '-' (common for stdin)
            if (cur[1] == '\0') {
                ca_parsed_arg(&last_opt, cur);
                i++;
                continue;
            }

            // handle '--'
            if (strcmp(cur, "--") == 0) {
                if (app.use_end_of_options) {
                    only_args_now = true;
                } else {
                    ca_parsed_arg(&last_opt, cur);
                }
                i++;
                continue;
            }

            // if last opt has not been resolved, we're at a flag now, so it
            // better have needed an _optional_ argument
            if (last_opt && !(last_opt->flags & CA_OPT_OPTARG)) {
                ca_print_error("--%s missing required argument\n",
                    last_opt->long_opt);
                return 1;
            }
            last_opt = NULL;

            // we now parse the option and argument (if there)
            struct ca_opt* opt = NULL;
            const char* arg = NULL;

            // determine whether it is a long or short option and search for the
            // corresponding option struct
            if (cur[1] != '-' /* short opt */) {
                char flag = cur[1];

                // the first character after '-' should always be a valid option
                opt = ca_lookup_opt(flag, NULL);
                if (!opt) {
                    ca_print_error("unknown flag -%c\n", flag);
                    return 1;
                }

                // we might have more characters though
                if (cur[2] != '\0') {
                    // we have a bunch of flags potentially after a short opt
                    // can only be multiflag if first is multiflag and then all
                    // others are too
                    if (opt->flags & CA_OPT_MFLAG) {
                        for (size_t j = 2; cur[j]; j++) {
                            opt = ca_lookup_opt(cur[j], NULL);
                            if (!opt) {
                                ca_print_error("unknown flag -%c\n", cur[j]);
                                return 1;
                            }
                            if (!(opt->flags & CA_OPT_MFLAG)) {
                                ca_print_error(
                                    "-%c must be passed separately from -%c\n",
                                    opt->short_opt, flag);
                                return 1;
                            }
                        }
                        for (size_t j = 1; cur[j]; j++) {
                            // this will work now because we tested it above
                            opt = ca_lookup_opt(cur[j], NULL);
                            ca_parsed_opt(opt, NULL);
                        }
                        i++;
                        continue;
                    } else {
                        // treat as connected option
                        // example: -I/usr/include is -I /usr/include
                        opt = ca_lookup_opt(flag, NULL);
                        if (!(opt->flags & CA_OPT_ARG)) {
                            ca_print_error("-%c does not take arguments\n",
                                flag);
                            return 1;
                        }
                        arg = cur + 2;
                    }
                }
            } else /* long opt */ {
                opt = ca_lookup_opt('\0', cur + 2);
                if (!opt) {
                    ca_print_error("unknown flag %s\n", cur);
                    return 1;
                }
            }
            if (!arg && opt->flags & CA_OPT_ARG) {
                // delay resolution of argument until next iteration or loop end
                last_opt = opt;
            } else {
                ca_parsed_opt(opt, arg);
            }
            i++;
        } else {
            // otherwise it must be an argument
            ca_parsed_arg(&last_opt, cur);
            i++;
        }
    }

    // more code dup
    if (last_opt && !(last_opt->flags & CA_OPT_OPTARG)) {
        ca_print_error("--%s missing required argument\n", last_opt->long_opt);
        return 1;
    }

    return 0;
}

// this function is a sin upon this earth
/** Determines whether the parsed results have any conflicts. */
static int ca_verify_results(void) {
    // check every option passed
    for (size_t i = 0; i < app.results_length; i++) {
        struct ca_parse_result result = app.results[i];
        if (result.opt) {
            const struct ca_opt* opt = result.opt;
            struct ca_opt* ref = NULL;

            // determine if the quantified proposition holds
            bool verdict = false;
            switch (opt->quantifier) {
                case CA_OPT_QUANTIFIER_ANY: {
                    // start out false, any success makes true
                    verdict = false;
                    for (size_t j = 0; opt->refs[j]; j++) {
                        ref = ca_lookup_opt(opt->refs[j], NULL);
                        if (!ref) {
                            ca_print_error(
                                "unknown flag -%c in definition of --%s\n",
                                opt->refs[j], opt->long_opt);
                            return 1;
                        }
                        if (ref->was_passed) {
                            verdict = true;
                            break;
                        }
                    }
                    break;
                }
                case CA_OPT_QUANTIFIER_ALL: {
                    // start out true, any failure makes false
                    verdict = true;
                    for (size_t j = 0; opt->refs[j]; j++) {
                        ref = ca_lookup_opt(opt->refs[j], NULL);
                        if (!ref) {
                            ca_print_error(
                                "unknown flag -%c in definition of --%s\n",
                                opt->refs[j], opt->long_opt);
                            return 1;
                        }
                        if (!ref->was_passed) {
                            verdict = false;
                            break;
                        }
                    }
                    break;
                }
                case CA_OPT_QUANTIFIER_ONLY: {
                    // 1. determine how many of the allowed arguments were
                    // passed
                    // 2. if more arguments were passed then allowed, there's a
                    // conflict
                    verdict = true;
                    size_t allowed_count = 0;
                    for (size_t j = 0; opt->refs[j]; j++) {
                        ref = ca_lookup_opt(opt->refs[j], NULL);
                        if (!ref) {
                            ca_print_error(
                                "unknown flag -%c in definition of --%s\n",
                                opt->refs[j], opt->long_opt);
                            return 1;
                        }
                        if (ref->was_passed) {
                            allowed_count++;
                        }
                        if (app.options_count > allowed_count) {
                            verdict = false;
                        }
                    }
                    break;
                }
                default: {
                    verdict = true;
                    break;
                }
            }
            // negation flips verdict
            if (opt->quantifier_is_negated) {
                verdict = !verdict;
            }
            // render verdict
            if (!verdict) {
                switch (opt->quantifier) {
                    case CA_OPT_QUANTIFIER_ANY: {
                        if (opt->quantifier_is_negated) {
                            ca_print_error("-%c conflicts with --%s\n",
                                ref->short_opt, opt->long_opt);
                        } else {
                            ca_print_error(
                                "at least one of the specified options for "
                                "--%s must be passed\n",
                                opt->long_opt);
                        }
                        break;
                    }
                    case CA_OPT_QUANTIFIER_ALL: {
                        if (opt->quantifier_is_negated) {
                            ca_print_error(
                                "only some of the specified options for --%s "
                                "should be passed\n",
                                opt->long_opt);
                        } else {
                            ca_print_error(
                                "all of the specified options for --%s must be "
                                "passed\n",
                                opt->long_opt);
                        }
                        break;
                    }
                    case CA_OPT_QUANTIFIER_ONLY: {
                        if (opt->quantifier_is_negated) {
                            ca_print_error(
                                "only other options besides those specified "
                                "for --%s should be passed\n",
                                opt->long_opt);
                        } else {
                            if (opt->short_opt != '\0'
                                && opt->refs[0] == opt->short_opt) {
                                ca_print_error(
                                    "--%s must be passed by itself\n",
                                    opt->long_opt);
                            } else {
                                ca_print_error(
                                    "--%s can only be passed with allowed "
                                    "options\n",
                                    opt->long_opt);
                            }
                        }
                        break;
                    }
                    default:
                        break;
                }
                return 1;
            }
        }
    }

    return 0;
}

int ca_parse(void* user_data) {
    // reset all options
    for (size_t i = 0; i < app.options_length; i++) {
        app.options[i].was_passed = false;
    }
    app.options_count = 0;
    // clear results array
    app.results_length = 0;

    // do bulk of the parsing
    if (ca_construct_results() != 0) {
        return 1;
    }

    // check for conflicts
    if (ca_verify_results() != 0) {
        return 1;
    }

    // run the callbacks
    for (size_t i = 0; i < app.results_length; i++) {
        struct ca_parse_result result = app.results[i];
        if (result.opt) {
            if (!app.override_help
                && strcmp(result.opt->long_opt, "help") == 0) {
                ca_print_help();
            } else if (!app.override_version
                       && strcmp(result.opt->long_opt, "version") == 0) {
                ca_print_version();
            } else {
                app.opt_callback(result.opt->short_opt, result.opt->long_opt,
                    result.arg, user_data);
            }
        } else {
            app.arg_callback(result.arg, user_data);
        }
    }

    return 0;
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
        int current_year = ca_get_current_year();
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

void ca_print_error(const char* fmt, ...) {
    const char* prefix = "error";
#ifdef CA_ON_UNIX
    if (getenv("NO_COLOR") == NULL && isatty(fileno(stderr))) {
        prefix = "\033[31merror\033[m";
    }
#endif
    fprintf(stderr, "%s: ", prefix);
    va_list l;
    va_start(l, fmt);
    vfprintf(stderr, fmt, l);
    va_end(l);
}
