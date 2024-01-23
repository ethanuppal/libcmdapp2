/**
 * \file cmdapp.h
 * \brief Library for command line interfaces.
 * \copyright Copyright (C) 2024 Ethan Uppal. All rights reserved.
 * \author Ethan Uppal
 */

#pragma once

#include <stdbool.h>

#ifdef CA_PUBLIC_SRC
"Error: this header should be the only place where CA_PUBLIC_SRC is defined.";
#endif

#define CA_PUBLIC_SRC
#include "proj.h"
#undef CA_PUBLIC_SRC

/**
 * Says hello.
 *
 * Prints `"Hello\n"` to the standard output stream. Prints an error on
 * failure.
 */
void ca_hello(void);

/**
 * Initializes the library with the `argc` and `argv` parameters from the `main`
 * function.
 *
 * @returns Zero on success, nonzero on failure.
 */
int ca_init(int argc, const char* argv[]);

/**
 * Writes a description, overwriting the previous description.
 *
 * If `NULL` is passed, this function has no effect.
 */
void ca_description(const char* description);

/**
 * Adds an author.
 *
 * If `NULL` is passed, this function has no effect.
 */
void ca_author(const char* author);

/**
 * Sets the year of the program when copyright began.
 *
 * @pre `year` is non-negative.
 */
void ca_year(int year);

/**
 * Sets the program version, overwriting the previous.
 *
 * The version is originally set to v0.0.0. See http://semver.org for more
 * information on semantic versioning.
 *
 * @pre `major`, `minor`, and `patch` are all non-negative.
 */
void ca_version(int major, int minor, int patch);

/**
 * Sets additional versioning information for the program, replacing previous
 * information. If `NULL` is passed, this function has no effect.
 *
 * @todo Figure out how to make multiple calls to this work. Could be
 * interesting handling sentences and newlines.
 */
void ca_versioning_info(const char* info);

/**
 * Registers a synopsis.
 *
 * A synopsis is a textual description of how the command should be run. Note
 * that whatever interpretations or scheme you intend for your synopsis is
 * entirely up to you. If `NULL` is passed, this function has no effect.
 *
 * \par Example
 * Here are some example synopses with possible interpretations.
 * - `"subcommand [OPTION]..."` means that the program can take a
 * subcommand followed by a series of options.
 * - `"[OPTION]... FILE"` means that the program can take a series of options
 * followed by a filename.
 */
void ca_synopsis(const char* synopsis);

/** Whether the argument `--` should be ignored and all subsequent arguments
 * treated verbatim. This is enabled by default. */
void ca_use_end_of_options(bool use);

/** Specifies whether `--help` and `--version` should be overriden from their
 * defaults. */
void ca_override_help_version(bool override_help, bool override_version);

/**
 * Registers a command-line option `short_opt`/`long_opt`. Sets `errno` on
 * failure.
 *
 * The `behavior` parameter is easily the most confusing. I have written a
 * \ref book/opt.md "comprehensive breakdown" of the parameter.
 *
 * @param short_opt The one-letter version of the option. If `\0` is passed,
 * this function behaves like ca_long_opt().
 * @param long_opt The long version of the option.
 * @param behavior Characteristics of the option, such as whether it takes an
 * argument.
 * @param result A pointer to a string initialized with the argument passed to
 * this option.
 * @param description A description of the option.
 *
 * @returns A handle to whether the flag was passed, or `NULL` on failure.
 */
bool* ca_opt(char short_opt, const char* long_opt, const char* behavior,
    const char** result, const char* description);

/**
 * Registers a (strictly long) command-line option `long_opt`.
 *
 * Behaves equivalently to ca_opt() but with the short option variant neglected.
 * Please see there for further information.
 */
bool* ca_long_opt(const char* long_opt, const char* behavior,
    const char** result, const char* description);

/**
 * Sets two on-line callbacks that will be invoked during parsing. The provided
 * callbacks replace previously set ones. If either callback provided is `NULL`
 * at the time of ca_parse(), it is not invoked.
 *
 * @param opt_callback Invoked when an option is parsed.
 * @param arg_callback Invoked when an argument is parsed.
 */
void ca_set_callbacks(void (*opt_callback)(char, const char*, const char*,
                          void*),
    void (*arg_callback)(const char*, void*));

/**
 * Runs the parser on the command line arguments.
 *
 * After this function returns, all arguments will be initialized with
 * proper values. At each stage during the parsing, the user-provided
 * callbacks will be invoked with `user_data` as its parameter (see
 * ca_set_callbacks()).
 *
 * @pre ca_init() must have been called.
 *
 * @returns Zero on success, nonzero on failure.
 *
 * \par Time Complexity
 * This function runs in `O(nm)` time where `n` is the number of parsed
 * options and arguments and `m` is the number of options, assuming that each
 * long option is roughly constant time in comparison. In other words, if
 * options `a`, `b`, and `c` all support multiflag, then `-abc` would correspond
 * with `n=3`.
 */
int ca_parse(void* user_data);

/**
 * Prints versioning information to standard output.
 *
 * Compatible with `help2man` if run on `--version`.
 */
void ca_print_version(void);

/**
 * Prints help information to standard output.
 *
 * Compatible with `help2man` if run on `--help`.
 */
void ca_print_help(void);

#ifdef CA_PRIVATE_SRC

    #if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
        #define CA_ON_UNIX
        #include <unistd.h>
    #endif

    #define HELLO_STRING "hello\n"
    #define CA_NO_YEAR -1
    #define CA_DESCRIPTION_OFFSET 20

/** Option information. */
enum ca_opt_flags {
    CA_OPT_ARG = 1 << 0,     ///< Takes an argument.
    CA_OPT_OPTARG = 1 << 1,  ///< Argument is optional.
    CA_OPT_MFLAG = 1 << 2    ///< Occurs in multiflag.
};

/** Quantifiers for determining option compatibility. */
enum ca_opt_quantifier {
    CA_OPT_QUANTIFIER_NONE,  ///< Absence of a quantifier.
    CA_OPT_QUANTIFIER_ALL,   ///< All of the provided options must be passed.
    CA_OPT_QUANTIFIER_ANY,   ///< At least one of the provided options must be
                             ///< passed.
    CA_OPT_QUANTIFIER_ONLY   ///< Only the provided options can be passed.
};

/** A command line option. */
struct ca_opt {
    char short_opt;        ///< Short version of the command, or `'\0'` if none.
    const char* long_opt;  ///< Long version of the command.
    enum ca_opt_flags flags;            ///< Option flags.
    bool quantifier_is_negated;         ///< Whether `quantifier` is negated.
    enum ca_opt_quantifier quantifier;  ///< Conflict quantifier.
    const char* refs;         ///< A null-terminated list of option refs.
    const char** result;      ///< A pointer to where the passed arg should go.
    const char* arg_name;     ///< Name of the argument.
    const char* description;  ///< Option description.
    bool was_passed;  ///< Whether the option has been passed in the current
                      ///< run of prasing.
};

/**
 * An option or an argument, or a combination thereof.
 *
 * State | Meaning
 * --- | ---
 * `opt != NULL && arg != NULL` | Option with argument.
 * `opt == NULL && arg != NULL` | Option without argument.
 * `opt != NULL && arg == NULL` | Ordinary argument.
 * `opt == NULL && arg == NULL` | This state is disallowed.
 */
struct ca_parse_result {
    const struct ca_opt* opt;
    const char* arg;
};

/** Library data for the command line app. */
struct ca_app {
    int argc;
    const char** argv;

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

    size_t results_length;
    size_t results_capacity;
    size_t options_count;             ///< The number of options parsed.
    struct ca_parse_result* results;  ///< Results of most recent parse.

    void (*opt_callback)(char, const char*, const char*,
        void*);                       ///< Option callback.
    void (*arg_callback)(const char*, void*);  ///< Argument callback.

    bool override_help;     ///< Whether the user overrode `-h`/`--help`.
    bool override_version;  ///< Whether  the user overrode `-v`/`--version`.
};

/**
 * Releases all resources allocated by the library at program termination.
 *
 * This function should not be called by a user.
 */
void ca_deinit(void);

/** Prints a command line parsing error to standard error. */
void ca_print_error(const char* fmt, ...);

#endif
