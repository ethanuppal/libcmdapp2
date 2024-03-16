// Copyright (C) 2024 Ethan Uppal. All rights reserved.

#include <stddef.h>
#include <stdio.h>
#include <cmdapp.h>
#include <stdlib.h>
#include <assert.h>

struct app {
    const char* expr;
    const char* filename;
};

void opt_callback(char short_opt, const char* long_opt, const char* arg,
    void* data) {
    printf("opt: short_opt=%c long_opt=%s arg=%s\n", short_opt, long_opt, arg);
}

void arg_callback(const char* arg, void* data) {
    printf("arg: arg=%s\n", arg);
}

int main(int argc, const char* argv[]) {
    struct app* app = malloc(sizeof(*app));
    if (!app) {
        perror("malloc");
        return 1;
    }

    if (ca_init(argc, argv) != 0) {
        perror("ca_init");
        return 1;
    }

    // program info
    ca_description("Serves as a useful example program for libcmdapp.");
    ca_author("First Author");
    ca_author("Second Author");
    ca_year(2024);
    ca_version(1, 0, 0);
    ca_versioning_info("All rights reserved.");

    // program usage
    ca_synopsis("subcommand [OPTION]...");
    ca_synopsis("[OPTION]... FILE");

    // prorgam options
    const char* a_arg = NULL;
    bool* a = ca_opt('a', "aa", ".LOL", &a_arg, "required arg");
    bool* A = ca_opt('A', "aa", ".?", &a_arg, "optional arg");
    bool* b = ca_opt('b', "bb", "*", NULL, "multiflag");
    bool* c = ca_opt('c', "cc", "*", NULL, "multiflag");
    bool* d = ca_opt('d', "dd", "!@bc", NULL, "incompatible with -b and -c");
    bool* O = ca_opt('O', "opt", "&ad", NULL, "depends on a and d");

    ca_opt('h', "help", "<h", NULL, "prints this info");
    ca_opt('v', "version", "<v", NULL, "prints version info");

    // parse
    ca_set_callbacks(opt_callback, arg_callback);
    if (ca_parse(app) != 0) {
        return 1;
    }

    printf("a was passed: %s (arg was %s)\n", (*a) ? "true" : "false", a_arg);

    free(app);
}
