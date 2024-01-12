/**
 * \file cmdapp.h
 * \brief Library for command line interfaces.
 * \copyright Copyright (C) 2024 Ethan Uppal. All rights reserved.
 * \author Ethan Uppal
 */

#pragma once

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
 * Sets additional versioning information for the program, overwriting previous
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

/**
 * Registers a command-line option `short_opt`/`long_opt`.
 *
 * @param short_opt The one-letter version of the option.
 * @param long_opt The long version of the option.
 * @param behavior Characteristics of the option, such as whether it takes an
 * argument.
 * @param result A pointer to a string initialized with the argument passed to
 * this option.
 */
void ca_opt(char short_opt, const char* long_opt, const char* behavior,
    const char** result);

/**
 * Registers a (strictly long) command-line option `long_opt`.
 *
 * @param behavior Characteristics of the option, such as whether it takes an
 * argument.
 * @param result A pointer to a string initialized with the argument passed to
 * this option.
 */
void ca_long_opt(const char* long_opt, const char* behavior,
    const char** result);

/**
 * Runs the parser on the command line arguments.
 *
 * After this function returns, all arguments will be initialized with
 * proper values. At each stage during the parsing, the user-provided
 * callback will be invoked with `user_data` as its parameter.
 *
 * @todo Let this function be called repeatedly.
 *
 * @pre ca_init() must have been called.
 */
void ca_parse(void* user_data);

/**
 * Prints versioning information to standard output.
 */
void ca_print_version(void);

#ifdef CA_PRIVATE_SRC

/**
 * Releases all resources allocated by the library at program termination.
 *
 * This function should not be called by a user.
 */
void ca_deinit(void);

#endif
