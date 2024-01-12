// Copyright (C) 2024 Ethan Uppal. All rights reserved.

#include <stddef.h>
#include <stdio.h>
#include <cmdapp.h>

int main(int argc, const char* argv[]) {
    if (ca_init(argc, argv) != 0) {
        perror("ca_init");
        return 1;
    }

    // program info
    ca_description("Serves as a useful example program for libcmdapp.");
    ca_author("Ethan Uppal");
    ca_author("Other Author");
    ca_year(2024);
    ca_version(1, 0, 0);
    ca_versioning_info("All rights reserved.");

    // program usage
    ca_synopsis("subcommand [OPTION]...");
    ca_synopsis("[OPTION]... FILE");

    // prorgam options
    ca_opt('a', "alert", "!@bef", NULL, "oh no!");
    ca_opt('b', "very-long-name", "", NULL,
        "this text has been put down a line");

    const char* expr = "EXPR";
    ca_opt('e', "expr", ".? !@f", &expr, "evaluates an expression");

    const char* filename = "FILE";
    ca_opt('f', "file", ". !@e", &filename, "processes a file");

    // parse
    // ca_parse(NULL);

    // ca_print_version();
    ca_print_help();
}
