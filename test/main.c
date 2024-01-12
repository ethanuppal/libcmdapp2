// Copyright (C) 2024 Ethan Uppal. All rights reserved.

#include <stddef.h>
#include <cmdapp.h>

int main(int argc, const char* argv[]) {
    ca_init(argc, argv);

    // program info
    ca_author("Ethan Uppal");
    ca_author("Eric Yachbes");
    ca_year(2024);
    ca_version(1, 0, 0);
    ca_versioning_info("All rights reserved.");

    // program usage
    ca_synopsis("subcommand [OPTION]...");
    ca_synopsis("[OPTION]... FILE");

    // const char* expr = NULL;
    // ca_opt('e', "expr", ".?", &expr);

    // ca_parse(NULL);
    ca_print_version();
}
