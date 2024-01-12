\mainpage Introduction

libcmdapp aims to be a simple and intuitive way to parse command line options and arguments.
It aims to provide flexible behavior with the least verbosity possible, intelligently filling any gaps.

Item | Location
------- | ---------
**Documentation** | cmdapp.h
**Repository** | https://github.com/ethanuppal/libcmdapp2

\section feature_sec Features

This library supports

- Intuitive and succinct interface
    ```c
    ca_author("My Name");
    ca_year(2024);
    ca_version(1, 0, 0);
    ```
- Long and short options
    ```c
    const char* expr = "EXPR";
    ca_opt('e', "expr", ". !@f", &expr, "evaluates an expression");
    ```
- Automatic `--help` and `--version` generation
    - The default implementation integrates with [`help2man`](https://www.gnu.org/software/help2man/) for __automatic man pages__
    - You can override and call ca_print_version() / ca_print_help() on your own
- Error handling and option conflicts

You can read more about supplying options [here](opt.md).

\section install_sec Install

\subsection install_unix_subsec Unix-like (MacOS, Linux)

The following bash commands will install the library at `/usr/local/lib/` and the header files at `/usr/local/include/cmdapp/`.

```bash
git clone https://github.com/ethanuppal/libcmdapp2.git
cd libcmdapp2
sudo make install
```

You can uninstall by running
```bash
sudo make uninstall
```
from the same directory.

\subsection install_windows_subsec Windows

The following bash commands will build the static and dynamic libraries in the `libcmdapp2/` directory.

```bash
git clone https://github.com/ethanuppal/libcmdapp2.git
cd libcmdapp2
make all
```

\section example_sec Example

We'll first initialize the library.

```c
if (ca_init(argc, argv) != 0) {
    perror("ca_init");
    return 1;
}
```

Then, we can tell it information about our program, such as the year it was written, the authors, etc.
```c
ca_description("Serves as a useful example program for libcmdapp.");
ca_author("Ethan Uppal");
ca_author("Other Author");
ca_year(2024);
ca_version(1, 0, 0);
ca_versioning_info("All rights reserved.");
```

We can also tell it how the program should be used in a series of synposes.
```c
ca_synopsis("subcommand [OPTION]...");
ca_synopsis("[OPTION]... FILE");
```

Finally, we supply the program options
```c
ca_opt('a', "alert", "", NULL, "oh no!");
ca_opt('b', "very-long-name", "", NULL,
    "this text has been put down a line");

const char* expr = "EXPR";
ca_opt('e', "expr", ". !@f", &expr, "evaluates an expression");

const char* filename = "FILE";
ca_opt('f', "file", ". !@e", &filename, "processes a file");
```

All you need now is to call ca_parse() or related functions!
