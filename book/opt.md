# Command Line Options

## Overview

You can supply options through two main functions: ca_opt() and ca_long_opt(). Since ca_long_opt() behaves identically to ca_opt() save having no short flag form, I will focus only on the former here.

The ca_opt() function is defined as follows:

```c
bool* ca_opt(
    char short_opt, 
    const char* long_opt, 
    const char* behavior,
    const char** result, 
    const char* description
);
```

Let's break down what these mean.

- `short_opt` is the short flag form of the option. For example, `-a` would be represented by a `short_opt` of `'a'`.

- `long_opt` is the long flag form of the option. For example, `--add` would be represented by a `long_opt` of `"add"`.

- `behavior` specifies how the option interacts with arguments and other options.

- `result` is a pointer to the resulting argument of an option after parsing. It will be set to `NULL` if the option was not passed. You can provide a custom argument name in the `--help` output by initializing it before creating the option. See the example on the main page for more. 

Be sure to check that the return value is `0`. If it's not, an error occured, and `errno` will have been set.

## Behavior

The behavior of an option is specified by a string parameter. Let's look at a few illustrative examples first.

Example `behavior` | Description
--- | ---
`""` | The empty string means that there is no additional behavior associated with this option.
`"."` | A dot means that the option takes an argument.
`".?"` | A question mark following the dot means that the option's argument is optional.
`"*"` | A star means that the option occurs in multiflag mode.
`". @abc"` | `@abc` means that this option can only be passed when __at least one__ of `-a`, `-b`, or `-c` is passed.
`".@abc"` | Whitespace isn't needed.
`". &abc"` | `&abc` means that this option can only be passed when __all__ of `-a`, `-b`, or `-c` are passed.
`". !@abc"` | `!&abc` means that this option can only be passed when __none__ of `-a`, `-b`, or `-c` are passed.
`"<abc"` | `<abc` means that this option can only be passed when only some of `-a`, `-b`, or `c` are passed, but not when any other flag is passed.

\warning
Note that you will not be able to specify referencing behavior for options that do not have a short flag form.

Here's what the symbols mean:

Symbol | Meaning
--- | ---
`.` | The option takes an argument.
`?` following `.` | The argument is optional.
`*` | The option occurs in multiflag mode.
`!` preceding `@` or `&` | The following quantifier is negated.
`@` | At least one of the following.
`&` | All of the following.
`<` | Only some of the following.

Essentially, the quantifiers `@`, `&`, and `<` enable you to express relationships between options:

- If you want an option `-a` that must be passed by itself, use `"<a"`. 
- If you want an option that is dependent on another option `-a`, use `"&a"`. 
- If you want to disable an option when the either `-a` and `-b` flags are passed, use `"!@ab"` (or `"!@ba"`).

The argument information (`"."`, `".?"`, or `"*"`) must be supplied before quantifiers, and arbitrary whitespace may be used to separate them for readability. It is possible to have quantifiers with no argument information, and vice versa.
